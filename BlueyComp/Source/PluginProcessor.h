#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <array>

class BlueyCompAudioProcessor : public juce::AudioProcessor
{
public:
    BlueyCompAudioProcessor();
    ~BlueyCompAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

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

    // El botón de bypass del host (Studio One, etc.) se enlaza con este parámetro
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Reducción de ganancia actual en dB (positivo), para el medidor del editor
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

    // Saturación con 4x oversampling para que los armónicos no generen aliasing
    juce::dsp::Oversampling<float> oversampler { 2, 2,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
    juce::AudioBuffer<float> dryBuffer;

    std::array<float, 2> dcX { 0.0f, 0.0f }, dcY { 0.0f, 0.0f };
    float dcCoeff = 0.999f;

    juce::SmoothedValue<float> driveDb;
    juce::SmoothedValue<float> outGain;
    juce::SmoothedValue<float> wetMix; // 1 = procesado, 0 = bypass (sin clics)

    std::atomic<float>* compressParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueyCompAudioProcessor)
};
