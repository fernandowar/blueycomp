#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class BlueyCompAudioProcessor : public juce::AudioProcessor
{
public:
    BlueyCompAudioProcessor();
    ~BlueyCompAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Bluey Comp"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Reducción de ganancia actual en dB (positivo), para el LED del editor
    std::atomic<float> gainReductionDb { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Ajustes fijos estilo 1176: Attack 3, Release 7, Ratio 4:1
    static constexpr float kAttackMs   = 0.4f;   // posición 3 del attack (~400 us)
    static constexpr float kReleaseMs  = 60.0f;  // posición 7 del release (rápido)
    static constexpr float kRatio      = 4.0f;
    static constexpr float kThreshDb   = -4.0f;  // umbral fijo (el knob sube el nivel de entrada)
    static constexpr float kKneeDb     = 6.0f;
    static constexpr float kMaxDriveDb = 32.0f;

    double sampleRate = 44100.0;
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;
    float envDb = 0.0f; // reducción suavizada (dB, positiva)

    juce::SmoothedValue<float> driveDb;
    juce::SmoothedValue<float> mixPower; // 1 = activo, 0 = bypass (sin clics)

    std::atomic<float>* compressParam = nullptr;
    std::atomic<float>* powerParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueyCompAudioProcessor)
};
