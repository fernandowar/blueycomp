#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <atomic>
#include "TransformerModel.h"

//==============================================================================
// Dark Transformer: saturación de transformador basada en una captura NAM.
// DRIVE empuja la señal contra el "hierro" (con compensación automática de
// nivel), MIX mezcla en paralelo (con el seco alineado en latencia) y
// OUTPUT ajusta el volumen final.
//==============================================================================
class DarkTransformerAudioProcessor : public juce::AudioProcessor,
                                      private juce::Timer
{
public:
    DarkTransformerAudioProcessor();
    ~DarkTransformerAudioProcessor() override;

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

    //==========================================================================
    // Modelos (llamar solo desde el hilo de mensajes)
    bool loadModelFile (const juce::File& file);  // captura .nam externa
    void loadDefaultModel();                       // la incluida en el plugin (o el respaldo DSP)
    const juce::String& getModelName() const { return modelName; }
    bool isModelNeural() const { return modelNeural; }
    const juce::String& getLastError() const { return lastError; }

    juce::AudioProcessorValueTreeState apvts;

    // Pico (dBFS) que entra al transformador después del DRIVE, para los LEDs
    std::atomic<float> ironLevelDb { -100.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void installModel (std::unique_ptr<TransformerModel> model);
    void timerCallback() override;

    double currentRate = 0.0;
    int currentBlock = 0;

    // Intercambio sin bloqueos entre hilos: el de mensajes deja el modelo nuevo
    // en `pending`, el de audio lo toma y deja el viejo en `retired`, y el
    // timer lo borra fuera del hilo de audio.
    TransformerModel* active = nullptr;
    std::atomic<TransformerModel*> pending { nullptr };
    std::atomic<TransformerModel*> retired { nullptr };

    juce::String modelName, lastError;
    bool modelNeural = false;

    juce::AudioBuffer<float> dryBuffer;
    std::vector<float> driveGains;
    juce::SmoothedValue<float> driveGain, outputGain, mixAmt, activeAmt;

    std::atomic<float>* driveParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* outputParam = nullptr;
    std::atomic<float>* bypassParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTransformerAudioProcessor)
};
