#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "Dark4kDsp.h"

// Dark 4K: canal estilo SSL 4000 E. Filtros + EQ de 4 bandas (Brown/Black) + Dark VCA (compresor con
// saturación y profundidad) + puerta básica + trim de entrada y fader de salida.
// Cada instancia es un "canal" distinto: pequeñas tolerancias analógicas (ganancias, frecuencias,
// umbral, ataque, saturación) derivadas de un número de canal que se guarda con el proyecto.
class Dark4kAudioProcessor : public juce::AudioProcessor
{
public:
    Dark4kAudioProcessor();
    ~Dark4kAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>& b, juce::MidiBuffer& m) override { processBlock (b, m); }

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Dark 4K"; }
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

    // Medidores (pico lineal acumulado desde la última lectura de la interfaz) y GR
    std::atomic<float> inPeak[2] { { 0.0f }, { 0.0f } };
    std::atomic<float> outPeak[2] { { 0.0f }, { 0.0f } };
    std::atomic<float> gainReductionDb { 0.0f };

    // Número de canal (1..999): define las tolerancias de este "canal de consola"
    int getChannel() const { return channel.load(); }
    void rerollChannel();

    static constexpr float kAttackMs[6] = { 0.1f, 0.3f, 1.0f, 3.0f, 10.0f, 30.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void setChannel (int ch);
    void updateFilters();

    // Variación de canal
    struct Variation
    {
        float eqGain[4] {}, eqFreq[4] {}, hpf = 1, lpf = 1, thrDb = 0, atk = 1, makeupDb = 0,
              satBias = 0, trimDb = 0, lrGain = 0, outDb = 0;
    } var;
    std::atomic<int> channel { 1 };
    std::atomic<bool> varDirty { true };

    // Ajustes fijos del VCA (los mismos del Dark VCA)
    static constexpr float kRatio = 3.0f, kKneeDb = 3.0f, kSatDrive = 1.25f, kDepth = 0.4f;
    static constexpr float kRelFixedMs = 100.0f, kRelFastMs = 100.0f, kRelSlowMs = 1200.0f, kAtkSlowMs = 300.0f;

    double sampleRate = 44100.0;

    // Filtros y EQ
    d4k::Biquad hpf1, hpf2, lpf, hf, hmf, lmf, lf;
    bool hpfOn = false, lpfOn = false, hfOn = false, hmfOn = false, lmfOn = false, lfOn = false;

    // Dinámica
    float relFixedCoeff = 0, relFastCoeff = 0, relSlowCoeff = 0, atkSlowCoeff = 0;
    float envFast = 0, envSlow = 0;
    float dcX[2] {}, dcY[2] {}, dcCoeff = 0.999f;
    float exLp[2] {}, sideLo = 0, sideHi = 0, exCoeff = 0.3f, sideLoCoeff = 0.02f, sideHiCoeff = 0.2f;
    float eqGritLp[2] {};
    // Puerta básica
    float gateLevel = 0, gateGainDb = 0, gateHold = 0;

    juce::SmoothedValue<float> thresholdDb, makeupGain, inGain, outGain, wetMix, gateThrDb, gateRangeDb;
    juce::SmoothedValue<float> eqMix, dynMix;

    std::atomic<float>* prm[40] {};
    enum P { pInTrim, pPhase, pLevel, pBypass, pEqIn, pEqBlack, pFiltIn, pHpf, pLpf,
             pHfG, pHfF, pHfBell, pHmfG, pHmfF, pHmfQ, pLmfG, pLmfF, pLmfQ, pLfG, pLfF, pLfBell,
             pDynIn, pThr, pAtk, pRel, pMakeup, pGateThr, pGateRange, pCut, pCount };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dark4kAudioProcessor)
};
