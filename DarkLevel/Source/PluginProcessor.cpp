#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DarkLevelAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "gain", 1 }, "Gain",
        juce::NormalisableRange<float> (-24.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "auto", 1 }, "Auto", false));
    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));
    return layout;
}

DarkLevelAudioProcessor::DarkLevelAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    gainParam   = apvts.getRawParameterValue ("gain");
    autoParam   = apvts.getRawParameterValue ("auto");
    bypassParam = apvts.getRawParameterValue ("bypass");
}

bool DarkLevelAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void DarkLevelAudioProcessor::prepareToPlay (double sr, int)
{
    W = juce::jmax (2, juce::roundToInt (0.002 * sr)); // 2 ms
    setLatencySamples (W - 1);
    for (auto& d : delay) d.assign ((size_t) W, 0.0f);
    gdRing.assign ((size_t) W, 1.0f);
    mRing.assign ((size_t) W, 1.0f);
    pos = 0;
    mSum = (double) W;
    env = 0.0f;
    gHold = 1.0f;
    envDecay = std::exp (-1.0f / (4.0f * (float) sr)); // la ganancia sube despacio (~2 dB/s)

    gain.reset (sr, 0.02);
    gain.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 1.0f : juce::Decibels::decibelsToGain (gainParam->load()));
    autoMix.reset (sr, 0.05);
    autoMix.setCurrentAndTargetValue (autoParam->load() > 0.5f && bypassParam->load() < 0.5f ? 1.0f : 0.0f);
}

void DarkLevelAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = juce::jmin (buffer.getNumChannels(), 2), n = buffer.getNumSamples();
    if (numCh == 0) return;

    const bool bypass = bypassParam->load() > 0.5f;
    gain.setTargetValue (bypass ? 1.0f : juce::Decibels::decibelsToGain (gainParam->load()));
    autoMix.setTargetValue (autoParam->load() > 0.5f && ! bypass ? 1.0f : 0.0f);
    const float targetLin = juce::Decibels::decibelsToGain (kAutoTargetDb);
    const float maxG = juce::Decibels::decibelsToGain (kAutoMaxDb);

    float* ch[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };
    float peak = 0.0f, lastAutoG = 1.0f;

    for (int i = 0; i < n; ++i)
    {
        // pico de entrada (enlazado: máximo de los canales, no cambia la imagen estéreo)
        float pk = 0.0f;
        for (int c = 0; c < numCh; ++c) pk = juce::jmax (pk, std::abs (ch[c][i]));

        // ganancia deseada para llevar el pico a -9 dBFS (sube lento, baja de inmediato)
        env = juce::jmax (pk, env * envDecay);
        float gd = gHold;
        if (env > 1.0e-4f) gd = juce::jlimit (1.0f / maxG, maxG, targetLin / env);
        gHold = gd;

        // mínimo en ventana + media móvil: baja suave justo antes del pico (lookahead)
        gdRing[(size_t) pos] = gd;
        float m = gd;
        for (int k = 0; k < W; ++k) m = juce::jmin (m, gdRing[(size_t) k]);
        mSum += (double) m - (double) mRing[(size_t) pos];
        mRing[(size_t) pos] = m;
        const float gAuto = (float) (mSum / (double) W);
        lastAutoG = gAuto;

        const float gm = gain.getNextValue();
        const float mix = autoMix.getNextValue();
        const float g = gm + mix * (gAuto - gm);

        const int rp = (pos + 1) % W; // muestra más antigua = retraso de W-1
        for (int c = 0; c < numCh; ++c)
        {
            const float in = ch[c][i];
            const float dl = (W > 1) ? delay[c][(size_t) rp] : in;
            delay[c][(size_t) pos] = in;
            const float o = dl * g;
            ch[c][i] = o;
            peak = juce::jmax (peak, std::abs (o));
        }
        pos = (pos + 1) % W;
    }

    autoGainDb.store (juce::Decibels::gainToDecibels (lastAutoG, -60.0f));
    float cur = peakLin.load();
    while (peak > cur && ! peakLin.compare_exchange_weak (cur, peak)) {}
}

juce::AudioProcessorEditor* DarkLevelAudioProcessor::createEditor()
{
    return new DarkLevelAudioProcessorEditor (*this);
}

void DarkLevelAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DarkLevelAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkLevelAudioProcessor();
}
