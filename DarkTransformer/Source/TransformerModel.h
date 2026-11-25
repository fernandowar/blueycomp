#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <functional>
#include <memory>
#include <vector>

namespace nam { class DSP; }
namespace dsp { template <typename T, int NCHANS, size_t A> class ResamplingContainer; }

//==============================================================================
// Transformador de respaldo (DSP clásico) para cuando no hay captura .nam.
// Satura más los graves que los agudos (como el núcleo de un transformador),
// con un poco de 2º armónico y una caída muy suave arriba de 20 kHz.
//==============================================================================
class FallbackTransformer
{
public:
    void prepare (double sampleRate);
    void reset();
    float processSample (float x);

private:
    float lowCoeff = 0.0f, hfCoeff = 1.0f, dcCoeff = 0.999f;
    float low = 0.0f, hf = 0.0f, dcX = 0.0f, dcY = 0.0f;
};

//==============================================================================
// Un "modelo de transformador" listo para sonar: o bien una captura NAM
// (una red por canal + resampling a la frecuencia del modelo), o bien el
// respaldo DSP. Se construye y prepara en el hilo de mensajes; process() y
// delayDry() corren en el hilo de audio y no reservan memoria.
//==============================================================================
class TransformerModel
{
public:
    ~TransformerModel();

    // Carga desde el texto JSON de un archivo .nam. Devuelve nullptr y llena
    // `error` si el archivo no sirve.
    static std::unique_ptr<TransformerModel> fromNam (const juce::String& jsonText,
                                                      const juce::String& displayName,
                                                      juce::String& error);
    static std::unique_ptr<TransformerModel> makeFallback();

    // Prepara para la frecuencia del host y el bloque máximo (nunca en el hilo de audio)
    void prepare (double hostSampleRate, int maxBlockSize);

    // Procesa en el lugar la señal húmeda (1 o 2 canales, n <= maxBlockSize)
    void process (float* const* io, int numChannels, int numSamples);

    // Retrasa la señal seca la misma latencia que el camino húmedo (para MIX)
    void delayDry (float* const* io, int numChannels, int numSamples);

    int getLatencySamples() const { return latency; }
    bool isNeural() const { return neural; }
    const juce::String& getName() const { return name; }
    double getModelSampleRate() const { return modelRate; }

private:
    TransformerModel() = default;
    void runNetworks (float** in, float** out, int n);

    using Resampler = dsp::ResamplingContainer<float, 2, 12>;

    bool neural = false;
    juce::String name;
    std::array<std::unique_ptr<nam::DSP>, 2> nets;
    double modelRate = 48000.0;
    bool arbitraryRate = false;
    float calibration = 1.0f; // deja la ganancia a nivel bajo en 0 dB

    double hostRate = 0.0;
    int maxBlock = 0;
    int activeChannels = 2;
    std::unique_ptr<Resampler> resampler;
    std::function<void (float**, float**, int)> netFunc;

    std::array<std::vector<float>, 2> inBuf, outBuf;
    std::array<float*, 2> inPtr { nullptr, nullptr }, outPtr { nullptr, nullptr };

    std::array<FallbackTransformer, 2> fallback;

    int latency = 0;
    std::array<std::vector<float>, 2> dryLine;
    int dryPos = 0;
};
