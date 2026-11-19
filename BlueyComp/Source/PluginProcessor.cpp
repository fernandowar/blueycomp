#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Etapa de salida clase A + transformador, estilo 1176 "Bluey":
    //  - término cuadrático asimétrico  -> 2º armónico
    //  - saturación tanh                -> 3º y armónicos impares
    // 'amt' (0..1) sube con el knob Compress.
    inline float shape (float x, float amt)
    {
        const float k = 1.0f + 1.4f * amt;
        float v = k * x;
        v += (0.20f * amt) * v * v;
        const float y = std::tanh (v) / k;
        return x + amt * (y - x);
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout BlueyCompAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "compress", 1 }, "Compress",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

BlueyCompAudioProcessor::BlueyCompAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    compressParam = apvts.getRawParameterValue ("compress");
    outputParam   = apvts.getRawParameterValue ("output");
    bypassParam   = apvts.getRawParameterValue ("bypass");
}

bool BlueyCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void BlueyCompAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    sampleRate = sr;
    attackCoeff  = 1.0f - std::exp (-1.0f / (0.001f * kAttackMs  * (float) sr));
    releaseCoeff = 1.0f - std::exp (-1.0f / (0.001f * kReleaseMs * (float) sr));
    envDb = 0.0f;

    dcCoeff = 1.0f - (juce::MathConstants<float>::twoPi * 8.0f / (float) sr);
    dcX.fill (0.0f);
    dcY.fill (0.0f);

    oversampler.initProcessing ((size_t) samplesPerBlock);
    oversampler.reset();
    const int latency = (int) std::ceil (oversampler.getLatencyInSamples());
    setLatencySamples (latency);

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) samplesPerBlock, 2 };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);
    dryDelay.reset();
    dryBuffer.setSize (2, samplesPerBlock);

    driveDb.reset (sr, 0.03);
    driveDb.setCurrentAndTargetValue (compressParam->load() * kMaxDriveDb);
    outGain.reset (sr, 0.03);
    outGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    wetMix.reset (sr, 0.02);
    wetMix.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
}

void BlueyCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0) return;

    const float compress = compressParam->load();

    driveDb.setTargetValue (compress * kMaxDriveDb);
    outGain.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    wetMix.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);

    // Copia seca retrasada por la latencia del oversampling (para el bypass sin clics)
    if (dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (2, numSamples, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dryBuffer.copyFrom (ch, 0, buffer, ch, 0, numSamples);
    {
        juce::dsp::AudioBlock<float> dryBlock (dryBuffer);
        auto sub = dryBlock.getSubBlock (0, (size_t) numSamples).getSubsetChannelBlock (0, (size_t) numCh);
        juce::dsp::ProcessContextReplacing<float> ctx (sub);
        dryDelay.process (ctx);
    }

    // --- Compresión (enlace estéreo, ataque/release fijos, ratio 4:1) ---
    const float slope = 1.0f - 1.0f / kRatio;
    float peakGr = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dDb = driveDb.getNextValue();
        const float drive = juce::Decibels::decibelsToGain (dDb);

        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[i]));

        const float levelDb = juce::Decibels::gainToDecibels (peak * drive, -120.0f);
        const float over = levelDb - kThreshDb;

        float targetGr = 0.0f;
        if (2.0f * over > kKneeDb)
            targetGr = slope * over;
        else if (2.0f * over > -kKneeDb)
        {
            const float x = over + kKneeDb * 0.5f;
            targetGr = slope * x * x / (2.0f * kKneeDb);
        }

        const float coeff = targetGr > envDb ? attackCoeff : releaseCoeff;
        envDb += coeff * (targetGr - envDb);
        peakGr = juce::jmax (peakGr, envDb);

        const float makeupDb = -0.65f * dDb;
        const float totalGain = juce::Decibels::decibelsToGain (dDb - envDb + makeupDb);

        for (int ch = 0; ch < numCh; ++ch)
            buffer.getWritePointer (ch)[i] *= totalGain;
    }

    // --- Armónicos tipo 1176 azul (2º + 3º), con 4x oversampling ---
    const float amt = 0.2f + 0.8f * compress;
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto sub = block.getSubsetChannelBlock (0, (size_t) numCh);
        auto up = oversampler.processSamplesUp (sub);
        for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
        {
            float* d = up.getChannelPointer (ch);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = shape (d[i], amt);
        }
        oversampler.processSamplesDown (sub);
    }

    // --- Filtro de DC (el término asimétrico genera offset), output y mezcla ---
    for (int i = 0; i < numSamples; ++i)
    {
        const float m  = wetMix.getNextValue();
        const float og = outGain.getNextValue();

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            const float x = d[i];
            const float y = x - dcX[(size_t) ch] + dcCoeff * dcY[(size_t) ch];
            dcX[(size_t) ch] = x;
            dcY[(size_t) ch] = y;

            const float wet = y * og;
            const float dry = dryBuffer.getReadPointer (ch)[i];
            d[i] = dry + m * (wet - dry);
        }
    }

    gainReductionDb.store (wetMix.getCurrentValue() > 0.5f ? peakGr : 0.0f);
}

juce::AudioProcessorEditor* BlueyCompAudioProcessor::createEditor()
{
    return new BlueyCompAudioProcessorEditor (*this);
}

void BlueyCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BlueyCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueyCompAudioProcessor();
}
