#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>

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
    // Ganancia que aplica el modo AUTO (dB), para mostrarla en la interfaz
    std::atomic<float> autoGainDb { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    // AUTO: sigue el pico de entrada y lleva la señal a -9 dBFS. Con lookahead de 2 ms la ganancia
    // baja ANTES de que llegue un pico, así que nunca clipea. Es solo ganancia (no colorea).
    static constexpr float kAutoTargetDb = -9.0f;
    static constexpr float kAutoMaxDb = 24.0f;
    int W = 2;                                   // ventana de lookahead (muestras)
    std::vector<float> delay[2];                 // audio retrasado W-1 muestras
    std::vector<float> gdRing, mRing;            // ganancia deseada y su mínimo deslizante
    int pos = 0;
    double mSum = 0.0;
    float env = 0.0f, envDecay = 0.9999f, gHold = 1.0f;

    juce::SmoothedValue<float> gain, autoMix;
    std::atomic<float>* gainParam = nullptr;
    std::atomic<float>* autoParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLevelAudioProcessor)
};
