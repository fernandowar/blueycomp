#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include "IronEngine.h"

//==============================================================================
// Dark Transformer: saturación de transformadores + preamp clase A en DSP,
// un solo color. DRIVE empuja la señal (con compensación automática de
// nivel), MIX mezcla en paralelo con el seco alineado en latencia y OUTPUT
// ajusta el volumen final. Oversampling 4x con filtros de fase lineal.
//==============================================================================
class DarkTransformerAudioProcessor : public juce::AudioProcessor
{
public:
    DarkTransformerAudioProcessor();
    ~DarkTransformerAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark Transformer"; }
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

    // Pico (dBFS) que entra al transformador después del DRIVE, para el VU
    std::atomic<float> ironLevelDb { -100.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    static constexpr int kOversamplingOrder = 2; // 2^2 = 4x
    std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
    std::array<iron::Channel, 2> channels;

    int blockSize = 0;
    int latency = 0;
    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> driveGains;
    std::array<std::vector<float>, 2> dryLine;
    std::vector<float> gainLine; // la compensación del DRIVE también va retrasada
    int linePos = 0;

    juce::SmoothedValue<float> driveGain, outputGain, mixAmt, activeAmt;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTransformerAudioProcessor)
};
