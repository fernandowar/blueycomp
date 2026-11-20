#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include <vector>
#include "LimiterDsp.h"

class DarkLimiterAudioProcessor : public juce::AudioProcessor
{
public:
    DarkLimiterAudioProcessor();
    ~DarkLimiterAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark Limiter"; }
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

    // Para los medidores de la interfaz
    std::atomic<float> gainReductionDb { 0.0f };   // dB positivos (máximo del bloque)
    std::atomic<float> outputPeakDb { -100.0f };   // pico de salida (dBFS, máximo del bloque)
    std::atomic<float> lufsShortTerm { -100.0f };  // LUFS de corto plazo (3 s)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double sampleRate = 44100.0;
    int latencySamples = 0;

    limdsp::DualStage stage1;          // lookahead 1.5 ms, release rápido + cola lenta
    limdsp::LookaheadStage stage2;     // seguridad: 1 ms, True Peak, fija el techo
    limdsp::LufsShortTerm lufs;

    // Copia seca retrasada (bypass sin clics y alineado con la latencia)
    std::vector<float> dryRing[2];
    int dryLen = 1, dryPos = 0;

    juce::SmoothedValue<float> inGain, ceilGain, wetMix;

    std::atomic<float>* thresholdParam = nullptr;
    std::atomic<float>* ceilingParam = nullptr;
    std::atomic<float>* truePeakParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLimiterAudioProcessor)
};
