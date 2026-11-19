#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>

class DarkVcaAudioProcessor : public juce::AudioProcessor
{
public:
    DarkVcaAudioProcessor();
    ~DarkVcaAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark VCA"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    // El botón de bypass del host se enlaza con este parámetro
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Reducción de ganancia actual en dB (positivo), para la escalera de LEDs
    std::atomic<float> gainReductionDb { 0.0f };

    static constexpr float kAttackMs[6] = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // Ajustes fijos
    static constexpr float kRatio    = 3.0f;
    static constexpr float kKneeDb   = 3.0f;
    static constexpr float kRelFixedMs = 100.0f;  // release fijo
    static constexpr float kRelFastMs  = 100.0f;  // etapa rápida del modo Auto
    static constexpr float kRelSlowMs  = 1200.0f; // etapa lenta del modo Auto
    static constexpr float kAtkSlowMs  = 300.0f;  // la etapa lenta "se entera" más despacio

    double sampleRate = 44100.0;
    float relFixedCoeff = 0.0f, relFastCoeff = 0.0f, relSlowCoeff = 0.0f, atkSlowCoeff = 0.0f;
    float envFast = 0.0f, envSlow = 0.0f; // reducción suavizada (dB, positiva)
    float dcX[2] { 0.0f, 0.0f }, dcY[2] { 0.0f, 0.0f }; // bloqueador de DC tras la saturación
    float dcCoeff = 0.999f;
    // DEPTH: filtros de un polo para excitador de presencia y ancho estéreo
    float exLp[2] { 0.0f, 0.0f };
    float sideLo = 0.0f, sideHi = 0.0f;
    float exCoeff = 0.3f, sideLoCoeff = 0.02f, sideHiCoeff = 0.2f;
    juce::SmoothedValue<float> driveAmt, depthAmt;

    juce::SmoothedValue<float> thresholdDb;
    juce::SmoothedValue<float> makeupGain;
    juce::SmoothedValue<float> wetMix; // 1 = activo, 0 = bypass (sin clics)

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* attackParam = nullptr;
    std::atomic<float>* releaseParam = nullptr;
    std::atomic<float>* makeupParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;
    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* depthParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkVcaAudioProcessor)
};
