#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

// Dark Level: gain staging. Una perilla de ganancia y un medidor de pico con zona objetivo -12..-6 dBFS.
class DarkLevelAudioProcessor : public juce::AudioProcessor
{
public:
    DarkLevelAudioProcessor();
    ~DarkLevelAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark Level"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Pico (lineal) acumulado desde la última lectura de la interfaz
    std::atomic<float> peakLin { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    juce::SmoothedValue<float> gain;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLevelAudioProcessor)
};
