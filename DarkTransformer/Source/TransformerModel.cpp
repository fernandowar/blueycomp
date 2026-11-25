#include "TransformerModel.h"

#include "NAM/dsp.h"
#include "NAM/get_dsp.h"
#include "NAM/wavenet/model.h"
#include "NAM/wavenet/slimmable.h"
#include "NAM/lstm.h"
#include "NAM/convnet.h"
#include "NAM/linear.h"
#include "NAM/container.h"
#include "NAM/sequential.h"
// El resampler viene de iPlug2 y espera estas dos definiciones del framework
namespace iplug { static constexpr double PI = 3.14159265358979323846; }
#ifndef DEFAULT_BLOCK_SIZE
 #define DEFAULT_BLOCK_SIZE 512
#endif
#include "ResamplingContainer/ResamplingContainer.h"

#include <cmath>

//==============================================================================
namespace
{
    constexpr float kTwoPi = 6.283185307f;

    // Cada arquitectura de NAM se registra sola con un objeto estático en su .cpp.
    // Dentro de un plugin el enlazador puede descartar esos archivos si nadie los
    // usa; referenciarlos aquí asegura que WaveNet, LSTM, etc. queden incluidos.
    using CreateFn = std::unique_ptr<nam::ModelConfig> (*) (const nlohmann::json&, double);
    void ensureArchitecturesLinked()
    {
        static volatile CreateFn fns[] = {
            &nam::wavenet::create_config, &nam::slimmable_wavenet::create_config,
            &nam::lstm::create_config,    &nam::convnet::create_config,
            &nam::linear::create_config,  &nam::container::create_config,
            &nam::sequential::create_config
        };
        juce::ignoreUnused (fns);
    }

    // tanh con un sesgo pequeño (aporta 2º armónico) y ganancia unitaria en cero
    inline float biasedSat (float x, float bias)
    {
        const float tb = std::tanh (bias);
        return (std::tanh (x + bias) - tb) / (1.0f - tb * tb);
    }
}

void FallbackTransformer::prepare (double sr)
{
    const auto onePole = [sr] (float hz) { return 1.0f - std::exp (-kTwoPi * hz / (float) sr); };
    lowCoeff = onePole (110.0f);
    hfCoeff  = onePole (juce::jmin (21000.0f, 0.45f * (float) sr));
    dcCoeff  = 1.0f - kTwoPi * 8.0f / (float) sr;
    reset();
}

void FallbackTransformer::reset()
{
    low = hf = dcX = dcY = 0.0f;
}

float FallbackTransformer::processSample (float x)
{
    // El flujo magnético del núcleo crece en graves: se satura la parte baja
    low += lowCoeff * (x - low);
    const float high = x - low;
    float y = 0.5f * biasedSat (2.0f * low, 0.08f) + high;

    // Saturación general muy suave (cola de 3º armónico)
    y = biasedSat (y * 0.7f, 0.03f) / 0.7f;

    // Pequeña pérdida de agudos extremos
    hf += hfCoeff * (y - hf);
    y = hf;

    // Bloqueador de DC (por el sesgo)
    const float out = y - dcX + dcCoeff * dcY;
    dcX = y;
    dcY = out;
    return out;
}

//==============================================================================
namespace
{
    // La latencia que informa el resampler no es exacta (difiere en 1-2 muestras).
    // Se mide con ruido de banda limitada pasando por una copia con proceso
    // identidad: el desfase con mayor correlación es la latencia real, así el
    // seco del MIX queda alineado y no aparece filtro peine.
    int measureResamplerLatency (double hostRate, double modelRate, int maxBlock)
    {
        dsp::ResamplingContainer<float, 2, 12> probe (modelRate);
        probe.Reset (hostRate, maxBlock);
        const int maxLag = 2 * probe.GetLatency() + 64;
        const int len = (int) (hostRate * 0.25) + maxLag;

        // Ruido filtrado muy por debajo de la Nyquist más baja
        std::vector<float> in ((size_t) len), out ((size_t) len, 0.0f);
        juce::Random rng (1234);
        const float cutoff = 0.15f * (float) juce::jmin (hostRate, modelRate);
        const float c = 1.0f - std::exp (-kTwoPi * cutoff / (float) hostRate);
        float s1 = 0.0f, s2 = 0.0f, s3 = 0.0f;
        for (auto& v : in)
        {
            s1 += c * (rng.nextFloat() * 2.0f - 1.0f - s1);
            s2 += c * (s1 - s2);
            s3 += c * (s2 - s3);
            v = s3;
        }

        auto identity = [] (float** i, float** o, int n)
        {
            for (int ch = 0; ch < 2; ++ch)
                std::copy (i[ch], i[ch] + n, o[ch]);
        };

        std::vector<float> dummyIn ((size_t) maxBlock, 0.0f), dummyOut ((size_t) maxBlock);
        for (int pos = 0; pos < len; pos += maxBlock)
        {
            const int n = juce::jmin (maxBlock, len - pos);
            float* ins[2]  = { in.data() + pos, dummyIn.data() };
            float* outs[2] = { out.data() + pos, dummyOut.data() };
            probe.ProcessBlock (ins, outs, n, identity);
        }

        int best = 0;
        double bestCorr = -1.0e30;
        for (int lag = 0; lag <= maxLag; ++lag)
        {
            double corr = 0.0;
            for (int i = maxLag; i < len; ++i)
                corr += (double) out[(size_t) i] * in[(size_t) (i - lag)];
            if (corr > bestCorr) { bestCorr = corr; best = lag; }
        }
        return best;
    }
}

TransformerModel::~TransformerModel() = default;

std::unique_ptr<TransformerModel> TransformerModel::makeFallback()
{
    std::unique_ptr<TransformerModel> m (new TransformerModel());
    m->neural = false;
    m->name = "DSP (sin captura)";
    return m;
}

std::unique_ptr<TransformerModel> TransformerModel::fromNam (const juce::String& jsonText,
                                                              const juce::String& displayName,
                                                              juce::String& error)
{
    ensureArchitecturesLinked();
    std::unique_ptr<TransformerModel> m (new TransformerModel());
    try
    {
        const auto json = nlohmann::json::parse (jsonText.toStdString());

        nam::DspLoadOptions opts;
        opts.prewarm = false;
        for (auto& net : m->nets)
            net = nam::get_dsp (json, opts);

        auto& net = *m->nets[0];
        if (net.NumInputChannels() != 1 || net.NumOutputChannels() != 1)
        {
            error = "La captura debe ser mono (1 entrada, 1 salida).";
            return nullptr;
        }

        m->arbitraryRate = net.SupportsArbitrarySampleRate();
       #ifdef DT_TEST_FORCE_RESAMPLE
        m->arbitraryRate = false; // solo para las pruebas del resampler
       #endif
        const double expected = net.GetExpectedSampleRate();
        m->modelRate = expected > 0.0 ? expected : 48000.0;

        // Calibración: seno de 1 kHz a -24 dBFS. Se mide la ganancia del modelo
        // a nivel bajo para que DRIVE = 0 dB suene al mismo volumen que el seco.
        const double rate = m->arbitraryRate ? 48000.0 : m->modelRate;
        constexpr int block = 512;
        net.Reset (rate, block);

        const int total = (int) (rate * 0.6);
        const int measureFrom = (int) (rate * 0.3);
        const float amp = std::pow (10.0f, -24.0f / 20.0f);
        std::vector<float> in (block), out (block);
        double sumIn = 0.0, sumOut = 0.0;
        for (int done = 0; done < total; done += block)
        {
            const int n = juce::jmin (block, total - done);
            for (int i = 0; i < n; ++i)
                in[(size_t) i] = amp * std::sin (kTwoPi * 1000.0f * (float) (done + i) / (float) rate);
            float* ip = in.data();
            float* op = out.data();
            net.process (&ip, &op, n);
            for (int i = 0; i < n; ++i)
                if (done + i >= measureFrom)
                {
                    sumIn  += (double) in[(size_t) i] * in[(size_t) i];
                    sumOut += (double) out[(size_t) i] * out[(size_t) i];
                }
        }

        if (! (sumOut > 1.0e-12) || ! std::isfinite (sumOut))
        {
            error = "La captura no devuelve señal (¿archivo dañado?).";
            return nullptr;
        }
        const float calib = (float) std::sqrt (sumIn / sumOut);
        m->calibration = juce::jlimit (0.01f, 100.0f, calib); // ±40 dB máx.
    }
    catch (const std::exception& e)
    {
        error = juce::String ("No se pudo cargar la captura: ") + e.what();
        return nullptr;
    }

    m->neural = true;
    m->name = displayName;
    return m;
}

//==============================================================================
void TransformerModel::prepare (double sr, int maxBlockSize)
{
    hostRate = sr;
    maxBlock = juce::jmax (1, maxBlockSize);
    latency = 0;
    resampler.reset();

    if (neural)
    {
        const bool needsResampling = ! arbitraryRate && std::abs (modelRate - hostRate) > 0.5;
        const double netRate = needsResampling ? modelRate : hostRate;
        const int netBlock = (int) std::ceil ((double) maxBlock * netRate / hostRate) + 32;

        for (auto& net : nets)
            net->Reset (netRate, netBlock);

        if (needsResampling)
        {
            resampler = std::make_unique<Resampler> (modelRate);
            resampler->Reset (hostRate, maxBlock);
            latency = measureResamplerLatency (hostRate, modelRate, maxBlock);
            netFunc = [this] (float** in, float** out, int n) { runNetworks (in, out, n); };
        }
    }
    else
    {
        for (auto& f : fallback)
            f.prepare (hostRate);
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        inBuf[(size_t) ch].assign ((size_t) maxBlock, 0.0f);
        outBuf[(size_t) ch].assign ((size_t) maxBlock, 0.0f);
        inPtr[(size_t) ch] = inBuf[(size_t) ch].data();
        outPtr[(size_t) ch] = outBuf[(size_t) ch].data();
        dryLine[(size_t) ch].assign ((size_t) latency + 1, 0.0f);
    }
    dryPos = 0;
}

void TransformerModel::runNetworks (float** in, float** out, int n)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        if (ch < activeChannels)
        {
            float* ip = in[ch];
            float* op = out[ch];
            nets[(size_t) ch]->process (&ip, &op, n);
        }
        else
        {
            std::fill (out[ch], out[ch] + n, 0.0f);
        }
    }
}

void TransformerModel::process (float* const* io, int numChannels, int n)
{
    jassert (n <= maxBlock);
    numChannels = juce::jmin (numChannels, 2);

    if (! neural)
    {
        for (int ch = 0; ch < numChannels; ++ch)
            for (int i = 0; i < n; ++i)
                io[ch][i] = fallback[(size_t) ch].processSample (io[ch][i]);
        return;
    }

    activeChannels = numChannels;
    for (int ch = 0; ch < 2; ++ch)
    {
        if (ch < numChannels)
            std::copy (io[ch], io[ch] + n, inPtr[(size_t) ch]);
        else
            std::fill (inPtr[(size_t) ch], inPtr[(size_t) ch] + n, 0.0f);
    }

    if (resampler != nullptr)
        resampler->ProcessBlock (inPtr.data(), outPtr.data(), n, netFunc);
    else
        runNetworks (inPtr.data(), outPtr.data(), n);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* src = outPtr[(size_t) ch];
        for (int i = 0; i < n; ++i)
        {
            const float v = src[i] * calibration;
            io[ch][i] = std::isfinite (v) ? v : 0.0f;
        }
    }
}

void TransformerModel::delayDry (float* const* io, int numChannels, int n)
{
    if (latency <= 0)
        return;

    const int len = latency + 1;
    numChannels = juce::jmin (numChannels, 2);
    int pos = dryPos;
    for (int i = 0; i < n; ++i)
    {
        const int readPos = (pos + 1) % len; // la muestra escrita hace `latency` pasos
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto& line = dryLine[(size_t) ch];
            line[(size_t) pos] = io[ch][i];
            io[ch][i] = line[(size_t) readPos];
        }
        pos = readPos;
    }
    dryPos = pos;
}
