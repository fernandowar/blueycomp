#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr float kStage1Over = 1.2589f; // +2 dB: la etapa 1 deja pasar 2 dB de picos al clipper
    constexpr float kClipKnee = 0.7f;      // el clipper empieza a curvar al 70% del techo
}

juce::AudioProcessorValueTreeState::ParameterLayout DarkLimiterAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    // Threshold (entrada): bajarlo sube la ganancia que entra al limitador
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "threshold", 1 }, "Threshold",
        juce::NormalisableRange<float> (-20.0f, 0.0f, 0.1f), -4.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    // Output = techo (ceiling)
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "ceiling", 1 }, "Output",
        juce::NormalisableRange<float> (-12.0f, 0.0f, 0.1f), -0.3f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "truepeak", 1 }, "True Peak", true));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

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

    // Etapa 1: limitador de 3 bandas (cruces 120 Hz y 2.5 kHz), lookahead 10 ms (ataque lento y
    //          suave), release dependiente del programa por banda (los graves lo tienen más largo).
    // Clipper suave (4x): se lleva los últimos 2 dB de picos sin bajar la ganancia, así se
    //          conserva la pegada de los transitorios y se reduce el bombeo.
    // Etapa 2: lookahead 1 ms, release 40 ms, True Peak: atrapa lo que quede y fija el techo.
    const limdsp::MultibandStage::BandCfg cfg[3] = { { 120.0f, 1000.0f }, { 60.0f, 500.0f }, { 42.0f, 350.0f } };
    stage1.prepare (sr, 10.0f, 120.0f, 2500.0f, cfg);
    clipper.reset();
    stage2.prepare (sr, 1.0f, 40.0f, 40.0f);
    lufs.prepare (sr);

    latencySamples = stage1.latency() + limdsp::SoftClipper4x::kLatency + stage2.latency();
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

    float* chL = buffer.getWritePointer (0);
    float* chR = numCh > 1 ? buffer.getWritePointer (1) : nullptr;

    float maxGr = 0.0f, maxPeak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float g = inGain.getNextValue();
        const float c = ceilGain.getNextValue();
        const float m = wetMix.getNextValue();

        const float dryL = chL[i];
        const float dryR = chR != nullptr ? chR[i] : dryL;

        // copia seca retrasada por la latencia total
        const int rp = (dryPos + dryLen - latencySamples) % dryLen;
        const float dl = dryRing[0][(size_t) rp];
        const float dr = dryRing[1][(size_t) rp];
        dryRing[0][(size_t) dryPos] = dryL;
        dryRing[1][(size_t) dryPos] = dryR;
        dryPos = (dryPos + 1) % dryLen;

        // limitador de dos etapas
        const float c1 = c * kStage1Over;
        const float bandCeil[3] = { c1, c1, c1 };
        float a, b, a2, b2, wl, wr;
        stage1.process (dryL * g, dryR * g, bandCeil, a, b);
        clipper.process (a, b, c, kClipKnee, a2, b2);
        stage2.process (a2, b2, c, truePeak, wl, wr);

        // seguro final: nunca por encima del techo
        wl = juce::jlimit (-c, c, wl);
        wr = juce::jlimit (-c, c, wr);

        const float grLin = stage1.currentGain() * stage2.currentGain();
        maxGr = juce::jmax (maxGr, -juce::Decibels::gainToDecibels (grLin, -60.0f));

        const float outL = dl + m * (wl - dl);
        const float outR = dr + m * (wr - dr);
        chL[i] = outL;
        if (chR != nullptr) chR[i] = outR;

        maxPeak = juce::jmax (maxPeak, std::abs (outL), std::abs (outR));
        lufs.process (outL, outR);
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
