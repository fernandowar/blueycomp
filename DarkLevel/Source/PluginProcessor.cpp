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
    gain.reset (sr, 0.02);
    gain.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 1.0f : juce::Decibels::decibelsToGain (gainParam->load()));
}

void DarkLevelAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numCh = buffer.getNumChannels(), n = buffer.getNumSamples();

    gain.setTargetValue (bypassParam->load() > 0.5f ? 1.0f : juce::Decibels::decibelsToGain (gainParam->load()));

    float peak = 0.0f;
    for (int i = 0; i < n; ++i)
    {
        const float g = gain.getNextValue();
        for (int ch = 0; ch < numCh; ++ch)
        {
            float* d = buffer.getWritePointer (ch);
            d[i] *= g;
            peak = juce::jmax (peak, std::abs (d[i]));
        }
    }

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
