#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr float DarkVcaAudioProcessor::kAttackMs[6];

juce::AudioProcessorValueTreeState::ParameterLayout DarkVcaAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Threshold estilo SSL: la perilla va de +10 dB (izquierda) a -20 dB (derecha)
    juce::NormalisableRange<float> thr (
        -30.0f, 10.0f,
        [] (float s, float e, float p) { return e - p * (e - s); },
        [] (float s, float e, float v) { return (e - v) / (e - s); },
        [] (float s, float e, float v) { return juce::jlimit (s, e, std::round (v * 10.0f) / 10.0f); });

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold", thr, -10.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::StringArray { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 3));

    layout.add (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::StringArray { "100 ms", "Auto" }, 0));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "makeup", 1 }, "Makeup",
        juce::NormalisableRange<float> (0.0f, 15.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

DarkVcaAudioProcessor::DarkVcaAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    thresholdParam = apvts.getRawParameterValue ("threshold");
    attackParam    = apvts.getRawParameterValue ("attack");
    releaseParam   = apvts.getRawParameterValue ("release");
    makeupParam    = apvts.getRawParameterValue ("makeup");
    bypassParam    = apvts.getRawParameterValue ("bypass");
}

bool DarkVcaAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DarkVcaAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    const auto coeffFor = [sr] (float ms) { return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sr)); };
    relFixedCoeff = coeffFor (kRelFixedMs);
    relFastCoeff  = coeffFor (kRelFastMs);
    relSlowCoeff  = coeffFor (kRelSlowMs);
    atkSlowCoeff  = coeffFor (kAtkSlowMs);
    envFast = envSlow = 0.0f;
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
    dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 15.0f / (float) sr;

    thresholdDb.reset (sr, 0.02);
    thresholdDb.setCurrentAndTargetValue (thresholdParam->load());
    makeupGain.reset (sr, 0.03);
    makeupGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (makeupParam->load()));
    wetMix.reset (sr, 0.02);
    wetMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
}

void DarkVcaAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0) return;

    const int atkIdx = juce::jlimit (0, 5, (int) std::round (attackParam->load()));
    const bool autoRelease = releaseParam->load() > 0.5f;
    const float attackCoeff = 1.0f - std::exp (-1.0f / (0.001f * kAttackMs[atkIdx] * (float) sampleRate));

    thresholdDb.setTargetValue (thresholdParam->load());
    makeupGain.setTargetValue (juce::Decibels::decibelsToGain (makeupParam->load()));
    wetMix.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);

    const float slope = 1.0f - 1.0f / kRatio;
    float peakGr = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float thr = thresholdDb.getNextValue();
        const float mk = makeupGain.getNextValue();
        const float m = wetMix.getNextValue();

        // Detector enlazado (stereo link): el canal más fuerte manda
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[i]));

        const float levelDb = juce::Decibels::gainToDecibels (peak, -120.0f);
        const float over = levelDb - thr;

        // Curva de ganancia ratio 3:1 con rodilla corta (3 dB)
        float targetGr = 0.0f;
        if (2.0f * over > kKneeDb)
            targetGr = slope * over;
        else if (2.0f * over > -kKneeDb)
        {
            const float x = over + kKneeDb * 0.5f;
            targetGr = slope * x * x / (2.0f * kKneeDb);
        }

        float gr;
        if (autoRelease)
        {
            // Release dependiente del programa: etapa rápida + etapa lenta que
            // solo "se carga" con compresión sostenida
            envFast += (targetGr > envFast ? attackCoeff : relFastCoeff) * (targetGr - envFast);
            envSlow += (targetGr > envSlow ? atkSlowCoeff : relSlowCoeff) * (targetGr - envSlow);
            gr = juce::jmax (envFast, envSlow);
        }
        else
        {
            envFast += (targetGr > envFast ? attackCoeff : relFixedCoeff) * (targetGr - envFast);
            envSlow = envFast;
            gr = envFast;
        }
        peakGr = juce::jmax (peakGr, gr);

        const float gain = juce::Decibels::decibelsToGain (-gr) * mk;
        // Saturación analógica: waveshaper tanh asimétrico (armónicos pares e impares),
        // siempre presente y más marcado con más compresión. Cada canal tiene una
        // pequeña "tolerancia" distinta, como componentes reales.
        const float drive = 1.6f + 0.07f * gr;
        constexpr float kSatMix = 0.6f;
        static const float biasCh[2] = { 0.12f, 0.16f };
        static const float gainCh[2] = { 1.0f, 0.994f };

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            const float dry = d[i];
            float wet = dry * gain * gainCh[ch];

            const float bias = biasCh[ch];
            const float tb = std::tanh (bias);
            const float shaped = (std::tanh (drive * wet + bias) - tb) / (drive * (1.0f - tb * tb));
            wet += kSatMix * (shaped - wet);

            // quita el DC que genera la asimetría
            const float y = wet - dcX[ch] + dcCoeff * dcY[ch];
            dcX[ch] = wet;
            dcY[ch] = y;
            wet = y;

            d[i] = dry + m * (wet - dry);
        }
    }

    gainReductionDb.store (wetMix.getCurrentValue() > 0.5f ? peakGr : 0.0f);
}

juce::AudioProcessorEditor* DarkVcaAudioProcessor::createEditor()
{
    return new DarkVcaAudioProcessorEditor (*this);
}

void DarkVcaAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DarkVcaAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkVcaAudioProcessor();
}
