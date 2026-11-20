#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DarkLimiterAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Threshold (entrada): bajarlo sube la ganancia que entra al limitador
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-20.0f, 0.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Output = techo (ceiling)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ceiling", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.2f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "truepeak", 1 }, "True Peak", true));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "unlink", 1 }, "Unlink", false));

    return layout;
}

DarkLimiterAudioProcessor::DarkLimiterAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    thresholdParam = apvts.getRawParameterValue ("threshold");
    ceilingParam   = apvts.getRawParameterValue ("ceiling");
    truePeakParam  = apvts.getRawParameterValue ("truepeak");
    bypassParam    = apvts.getRawParameterValue ("bypass");
    unlinkParam    = apvts.getRawParameterValue ("unlink");
}

bool DarkLimiterAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DarkLimiterAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;

    // Etapa 1: limitador de dos constantes de tiempo (lookahead 2 ms, release rápido 1.5 ms que sigue el
    //          recorte necesario y conserva el cuerpo de los transitorios + cola lenta de 40 ms).
    //          Ajustado contra el Ozone IRC II. Sin clipper, con True Peak y al techo final.
    // Etapa 2: lookahead 1 ms, release 30 ms: red de seguridad que casi no actúa.
    stage1.prepare (sr, 2.0f, 1.5f, 40.0f, 0.12f);
    stage2.prepare (sr, 1.0f, 30.0f, 30.0f);
    lufs.prepare (sr);

    oversampler.initProcessing ((size_t) kChunk);
    oversampler.reset();
    satBuf.setSize (2, kChunk);
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
    dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 15.0f / (float) sr;

    const int osLatency = juce::roundToInt (oversampler.getLatencyInSamples());
    latencySamples = osLatency + stage1.latency() + stage2.latency();
    setLatencySamples (latencySamples);

    dryLen = latencySamples + 2;
    for (auto& r : dryRing) r.assign ((size_t) dryLen, 0.0f);
    dryPos = 0;

    inGain.reset (sr, 0.02);
    inGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (-thresholdParam->load()));
    ceilGain.reset (sr, 0.02);
    ceilGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (ceilingParam->load()));
    wetMix.reset (sr, 0.02);
    wetMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
}

void DarkLimiterAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0) return;

    inGain.setTargetValue (juce::Decibels::decibelsToGain (-thresholdParam->load()));
    ceilGain.setTargetValue (juce::Decibels::decibelsToGain (ceilingParam->load()));
    wetMix.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
    const bool truePeak = truePeakParam->load() > 0.5f;

    const bool unlink = unlinkParam->load() > 0.5f;

    float* chL = buffer.getWritePointer (0);
    float* chR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float maxGr = 0.0f, maxPeak = 0.0f;

    // Saturación del Dark VCA (tanh casi simétrico, ligera asimetría y tolerancia entre
    // canales), a 4x para evitar aliasing, ANTES de la ganancia de entrada y el limitador.
    constexpr float kSatDrive = 1.25f;
    static const float biasCh[2] = { 0.004f, 0.006f };
    static const float gainCh[2] = { 1.0f, 0.994f };

    for (int start = 0; start < numSamples; start += kChunk)
    {
        const int n = juce::jmin (kChunk, numSamples - start);

        for (int i = 0; i < n; ++i)
        {
            satBuf.setSample (0, i, chL[start + i]);
            satBuf.setSample (1, i, chR != nullptr ? chR[start + i] : chL[start + i]);
        }
        juce::dsp::AudioBlock<float> block (satBuf);
        auto sub = block.getSubBlock (0, (size_t) n);
        auto up = oversampler.processSamplesUp (sub);
        for (size_t ch = 0; ch < 2; ++ch)
        {
            float* d = up.getChannelPointer (ch);
            const float bias = biasCh[ch], tb = std::tanh (bias);
            const float norm = 1.0f / (kSatDrive * (1.0f - tb * tb));
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = (std::tanh (kSatDrive * d[i] * gainCh[ch] + bias) - tb) * norm;
        }
        oversampler.processSamplesDown (sub);

        for (int i = 0; i < n; ++i)
        {
            const float g = inGain.getNextValue();
            const float c = ceilGain.getNextValue();
            const float m = wetMix.getNextValue();

            const float dryL = chL[start + i];
            const float dryR = chR != nullptr ? chR[start + i] : dryL;

            // quita el DC que genera la asimetría de la saturación
            float s[2] = { satBuf.getSample (0, i), satBuf.getSample (1, i) };
            for (int ch = 0; ch < 2; ++ch)
            {
                const float y = s[ch] - dcX[ch] + dcCoeff * dcY[ch];
                dcX[ch] = s[ch];
                dcY[ch] = y;
                s[ch] = y;
            }

            // copia seca retrasada por la latencia total
            const int rp = (dryPos + dryLen - latencySamples) % dryLen;
            const float dl = dryRing[0][(size_t) rp];
            const float dr = dryRing[1][(size_t) rp];
            dryRing[0][(size_t) dryPos] = dryL;
            dryRing[1][(size_t) dryPos] = dryR;
            dryPos = (dryPos + 1) % dryLen;

            // limitador de dos etapas
            float a, b, wl, wr;
            stage1.process (s[0] * g, s[1] * g, c, truePeak, unlink, a, b);
            stage2.process (a, b, c, truePeak, unlink, wl, wr);

            // seguro final: nunca por encima del techo
            wl = juce::jlimit (-c, c, wl);
            wr = juce::jlimit (-c, c, wr);

            const float grLin = stage1.currentGain() * stage2.currentGain();
            maxGr = juce::jmax (maxGr, -juce::Decibels::gainToDecibels (grLin, -60.0f));

            const float outL = dl + m * (wl - dl);
            const float outR = dr + m * (wr - dr);
            chL[start + i] = outL;
            if (chR != nullptr) chR[start + i] = outR;

            maxPeak = juce::jmax (maxPeak, std::abs (outL), std::abs (outR));
            lufs.process (outL, outR);
        }
    }

    gainReductionDb.store (wetMix.getCurrentValue() > 0.5f ? maxGr : 0.0f);
    outputPeakDb.store (juce::Decibels::gainToDecibels (maxPeak, -100.0f));
    lufsShortTerm.store (lufs.value());
}

juce::AudioProcessorEditor* DarkLimiterAudioProcessor::createEditor()
{
    return new DarkLimiterAudioProcessorEditor (*this);
}

void DarkLimiterAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DarkLimiterAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkLimiterAudioProcessor();
}
