#pragma once

// DSP del limitador, sin dependencias de JUCE (solo la librería estándar) para poder
// probarlo por separado.
//
//  * LookaheadStage : limitador "brickwall" con lookahead (mínimo deslizante + media
//    móvil), release dependiente del programa y detector opcional de True Peak.
//  * LufsShortTerm  : medidor LUFS de corto plazo (3 s) con filtro K (ITU-R BS.1770).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace limdsp
{
constexpr float kPi = 3.14159265358979323846f;

//==============================================================================
class LookaheadStage
{
public:
    // lookaheadMs : ventana de ataque; relMinMs/relMaxMs : rango del release.
    // Con relMin == relMax el release es fijo; con relMin < relMax se adapta al programa
    // (transitorios aislados -> rápido, limitación sostenida -> lento).
    void prepare (double sampleRate, float lookaheadMs, float relMinMs, float relMaxMs)
    {
        sr = sampleRate;
        la = std::max (2, (int) std::lround (lookaheadMs * 0.001 * sampleRate));
        relMin = relMinMs;
        relMax = relMaxMs;
        avgCoeff = 1.0f - std::exp (-1.0f / (0.25f * (float) sampleRate)); // 250 ms

        const int audioLen = la - 1 + kDetDelay + 2;
        audio.assign ((size_t) audioLen * 2, 0.0f);
        audioLenFrames = audioLen;

        dqVal.assign ((size_t) la + 2, 1.0f);
        dqIdx.assign ((size_t) la + 2, 0);
        box.assign ((size_t) la, 1.0f);

        reset();
    }

    void reset()
    {
        std::fill (audio.begin(), audio.end(), 0.0f);
        std::fill (dqVal.begin(), dqVal.end(), 1.0f);
        std::fill (dqIdx.begin(), dqIdx.end(), 0);
        std::fill (box.begin(), box.end(), 1.0f);
        for (auto& v : hist) std::fill (v.begin(), v.end(), 0.0f);
        histPos = 0;
        dqHead = dqCount = 0;
        t = 0;
        e = 1.0f;
        boxPos = 0;
        boxSum = (double) la;
        audioPos = 0;
        grAvgDb = 0.0f;
        relCoeff = coeffForMs (relMin);
        sinceCoeff = 0;
        lastGain = 1.0f;
    }

    // Retardo total de la etapa en muestras
    int latency() const { return la - 1 + kDetDelay; }

    // Procesa un frame estéreo. 'ceiling' es lineal. Devuelve la salida retrasada y limitada.
    void process (float inL, float inR, float ceiling, bool truePeak, float& outL, float& outR)
    {
        // 1. historial para el detector
        histPos = (histPos + 1) & kHistMask;
        hist[0][(size_t) histPos] = inL;
        hist[1][(size_t) histPos] = inR;

        // 2. detector: pico (muestral o True Peak aproximado con interpolación 4x)
        float det = 0.0f;
        for (int ch = 0; ch < 2; ++ch)
        {
            det = std::max (det, std::abs (h (ch, kDetDelay)));
            if (truePeak)
            {
                for (int k = 0; k < 3; ++k)
                {
                    float y = 0.0f;
                    for (int j = 0; j < kTaps; ++j)
                        y += h (ch, j) * kInterp[k][j];
                    det = std::max (det, std::abs (y));
                }
            }
        }

        // pequeño margen (~0.18 dB) con True Peak: la interpolación es una estimación
        const float cl = truePeak ? ceiling * 0.98f : ceiling;
        const float gTarget = det > cl ? cl / det : 1.0f;

        // 3. mínimo deslizante de las últimas 'la' muestras
        while (dqCount > 0 && dqVal[(size_t) backIdx()] >= gTarget)
            --dqCount;
        const int ins = (dqHead + dqCount) % (int) dqVal.size();
        dqVal[(size_t) ins] = gTarget;
        dqIdx[(size_t) ins] = t;
        ++dqCount;
        while (dqIdx[(size_t) dqHead] <= t - la)
        {
            dqHead = (dqHead + 1) % (int) dqVal.size();
            --dqCount;
        }
        const float m = dqVal[(size_t) dqHead];

        // 4. release (ataque instantáneo hacia m, recuperación exponencial)
        if (m < e)
            e = m;
        else
            e += relCoeff * (m - e);

        // release dependiente del programa
        const float grDb = -20.0f * std::log10 (std::max (e, 1.0e-6f));
        grAvgDb += avgCoeff * (grDb - grAvgDb);
        if (++sinceCoeff >= 16)
        {
            sinceCoeff = 0;
            const float adapt = std::min (1.0f, std::max (0.0f, grAvgDb / 3.0f));
            const float ms = relMin * std::pow (relMax / relMin, adapt);
            relCoeff = coeffForMs (ms);
        }

        // 5. media móvil de longitud 'la' (suaviza el ataque)
        boxSum += (double) e - (double) box[(size_t) boxPos];
        box[(size_t) boxPos] = e;
        boxPos = (boxPos + 1) % la;
        const float a = (float) (boxSum / (double) la);
        lastGain = a;

        // 6. audio retrasado: 'la - 1 + kDetDelay' muestras
        const int len = audioLenFrames;
        const int readPos = (audioPos + len - (la - 1 + kDetDelay)) % len;
        audio[(size_t) audioPos * 2] = inL;
        audio[(size_t) audioPos * 2 + 1] = inR;
        const float dl = audio[(size_t) readPos * 2];
        const float dr = audio[(size_t) readPos * 2 + 1];
        audioPos = (audioPos + 1) % len;

        outL = dl * a;
        outR = dr * a;
        ++t;
    }

    float currentGain() const { return lastGain; }

private:
    static constexpr int kTaps = 12;
    static constexpr int kDetDelay = 6; // retardo del detector (centro del FIR)
    static constexpr int kHistMask = 15;

    struct InterpTable
    {
        float v[3][kTaps];
        InterpTable()
        {
            for (int k = 1; k <= 3; ++k)
                for (int j = 0; j < kTaps; ++j)
                {
                    const float dt = (float) (kDetDelay - j) - 0.25f * (float) k;
                    const float x = kPi * dt;
                    const float sinc = std::abs (x) < 1.0e-6f ? 1.0f : std::sin (x) / x;
                    const float w = 0.5f * (1.0f + std::cos (kPi * dt / 6.5f));
                    v[k - 1][j] = sinc * w;
                }
        }
        const float* operator[] (int k) const { return v[k]; }
    };
    static const InterpTable& interpTable()
    {
        static const InterpTable tbl;
        return tbl;
    }

    float h (int ch, int back) const { return hist[(size_t) ch][(size_t) ((histPos - back) & kHistMask)]; }
    int backIdx() const { return (dqHead + dqCount - 1) % (int) dqVal.size(); }
    float coeffForMs (float ms) const { return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sr)); }

    const InterpTable& kInterp = interpTable();

    double sr = 44100.0;
    int la = 64;
    float relMin = 40.0f, relMax = 40.0f;
    float avgCoeff = 0.0f, relCoeff = 0.0f, grAvgDb = 0.0f;
    int sinceCoeff = 0;

    std::vector<float> audio;
    int audioLenFrames = 1, audioPos = 0;

    std::vector<float> dqVal;
    std::vector<int> dqIdx;
    int dqHead = 0, dqCount = 0;
    long t = 0;

    float e = 1.0f, lastGain = 1.0f;
    std::vector<float> box;
    int boxPos = 0;
    double boxSum = 0.0;

    std::vector<float> hist[2] = { std::vector<float> (16, 0.0f), std::vector<float> (16, 0.0f) };
    int histPos = 0;
};

//==============================================================================
// LUFS de corto plazo (ventana de 3 s) con ponderación K, canales L/R con peso 1.
class LufsShortTerm
{
public:
    void prepare (double sampleRate)
    {
        const double fs = sampleRate;

        // Etapa 1: shelf agudo
        {
            const double f0 = 1681.974450955533, G = 3.999843853973347, Q = 0.7071752369554196;
            const double K = std::tan (3.14159265358979323846 * f0 / fs);
            const double Vh = std::pow (10.0, G / 20.0), Vb = std::pow (Vh, 0.4996667741545416);
            const double a0 = 1.0 + K / Q + K * K;
            s1.b0 = (Vh + Vb * K / Q + K * K) / a0;
            s1.b1 = 2.0 * (K * K - Vh) / a0;
            s1.b2 = (Vh - Vb * K / Q + K * K) / a0;
            s1.a1 = 2.0 * (K * K - 1.0) / a0;
            s1.a2 = (1.0 - K / Q + K * K) / a0;
        }
        // Etapa 2: pasa-altos (RLB)
        {
            const double f0 = 38.13547087602444, Q = 0.5003270373238773;
            const double K = std::tan (3.14159265358979323846 * f0 / fs);
            const double a0 = 1.0 + K / Q + K * K;
            s2.b0 = 1.0; s2.b1 = -2.0; s2.b2 = 1.0;
            s2.a1 = 2.0 * (K * K - 1.0) / a0;
            s2.a2 = (1.0 - K / Q + K * K) / a0;
        }
        blockLen = std::max (1, (int) std::lround (0.1 * fs)); // bloques de 100 ms
        reset();
    }

    void reset()
    {
        for (auto& c : z1) c = Bq();
        for (auto& c : z2) c = Bq();
        sumSq = 0.0;
        count = 0;
        ring.assign (30, 0.0);
        ringPos = 0;
        ringFill = 0;
        lufs = -100.0f;
    }

    void process (float l, float r)
    {
        const float in[2] = { l, r };
        for (int ch = 0; ch < 2; ++ch)
        {
            const double y1 = run (s1, z1[ch], (double) in[ch]);
            const double y2 = run (s2, z2[ch], y1);
            sumSq += y2 * y2;
        }
        if (++count >= blockLen)
        {
            ring[(size_t) ringPos] = sumSq / (double) count;
            ringPos = (ringPos + 1) % (int) ring.size();
            ringFill = std::min (ringFill + 1, (int) ring.size());
            sumSq = 0.0;
            count = 0;

            double p = 0.0;
            for (int i = 0; i < ringFill; ++i) p += ring[(size_t) i];
            p /= (double) ringFill;
            lufs = p > 1.0e-10 ? (float) (-0.691 + 10.0 * std::log10 (p)) : -100.0f;
        }
    }

    float value() const { return lufs; }

private:
    struct Coef { double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0; };
    struct Bq { double x1 = 0, x2 = 0, y1 = 0, y2 = 0; };

    static double run (const Coef& c, Bq& z, double x)
    {
        const double y = c.b0 * x + c.b1 * z.x1 + c.b2 * z.x2 - c.a1 * z.y1 - c.a2 * z.y2;
        z.x2 = z.x1; z.x1 = x;
        z.y2 = z.y1; z.y1 = y;
        return y;
    }

    Coef s1, s2;
    Bq z1[2], z2[2];
    double sumSq = 0.0;
    int count = 0, blockLen = 4410;
    std::vector<double> ring = std::vector<double> (30, 0.0);
    int ringPos = 0, ringFill = 0;
    float lufs = -100.0f;
};
} // namespace limdsp
