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
        n1 = (la + 2) / 2; n2 = la + 1 - n1;
        box.assign ((size_t) n1, 1.0f);
        box2.assign ((size_t) n2, 1.0f);

        reset();
    }

    void reset()
    {
        std::fill (audio.begin(), audio.end(), 0.0f);
        std::fill (dqVal.begin(), dqVal.end(), 1.0f);
        std::fill (dqIdx.begin(), dqIdx.end(), 0);
        std::fill (box.begin(), box.end(), 1.0f);
        std::fill (box2.begin(), box2.end(), 1.0f);
        boxPos2 = 0; boxSum2 = (double) n2;
        for (auto& v : hist) std::fill (v.begin(), v.end(), 0.0f);
        histPos = 0;
        dqHead = dqCount = 0;
        t = 0;
        e = 1.0f;
        boxPos = 0;
        boxSum = (double) n1;
        audioPos = 0;
        grAvgDb = 0.0f;
        relCoeff = coeffForMs (relMin);
        sinceCoeff = 0;
        lastGain = 1.0f;
    }

    // Retardo total de la etapa en muestras
    int latency() const { return la - 1 + kDetDelay; }

    // Graves con release más lento (menos distorsión de modulación en graves).
    // fc: frecuencia de corte; ms: release del gain de graves (0 = desactivado).
    void setLowBandSlow (float fcHz, float ms)
    {
        lfOn = ms > 0.0f;
        if (! lfOn) return;
        const float K = std::tan (kPi * fcHz / (float) sr), Q = 0.70710678f;
        const float n = 1.0f / (1.0f + K / Q + K * K);
        lb0 = K * K * n; lb1 = 2.0f * lb0; lb2 = lb0;
        la1 = 2.0f * (K * K - 1.0f) * n; la2 = (1.0f - K / Q + K * K) * n;
        lfRel = coeffForMs (ms);
        for (auto& s : lz) s[0] = s[1] = 0.0f;
        gLF = 1.0f;
    }

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

        // 5. dos medias móviles en cascada (kernel triangular, ataque más suave)
        boxSum += (double) e - (double) box[(size_t) boxPos];
        box[(size_t) boxPos] = e;
        boxPos = (boxPos + 1) % n1;
        const float s1v = (float) (boxSum / (double) n1);
        boxSum2 += (double) s1v - (double) box2[(size_t) boxPos2];
        box2[(size_t) boxPos2] = s1v;
        boxPos2 = (boxPos2 + 1) % n2;
        const float a = (float) (boxSum2 / (double) n2);
        lastGain = a;

        // 6. audio retrasado: 'la - 1 + kDetDelay' muestras
        const int len = audioLenFrames;
        const int readPos = (audioPos + len - (la - 1 + kDetDelay)) % len;
        audio[(size_t) audioPos * 2] = inL;
        audio[(size_t) audioPos * 2 + 1] = inR;
        const float dl = audio[(size_t) readPos * 2];
        const float dr = audio[(size_t) readPos * 2 + 1];
        audioPos = (audioPos + 1) % len;

        if (lfOn)
        {
            // gain de graves: baja igual que 'a', pero recupera lento
            if (a < gLF) gLF = a; else gLF += lfRel * (a - gLF);
            const float d[2] = { dl, dr };
            float o[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                const float lp = lb0 * d[ch] + lz[ch][0];
                lz[ch][0] = lb1 * d[ch] - la1 * lp + lz[ch][1];
                lz[ch][1] = lb2 * d[ch] - la2 * lp;
                o[ch] = d[ch] * a - lp * (a - gLF);
            }
            outL = o[0]; outR = o[1];
        }
        else
        {
            outL = dl * a;
            outR = dr * a;
        }
        ++t;
    }

    float currentGain() const { return lastGain; }

private:
    static constexpr int kTaps = 24;
    static constexpr int kDetDelay = 12; // retardo del detector (centro del FIR)
    static constexpr int kHistMask = 31;

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
                    const float w = 0.5f * (1.0f + std::cos (kPi * dt / 12.5f));
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

    bool lfOn = false;
    float lb0 = 0, lb1 = 0, lb2 = 0, la1 = 0, la2 = 0, lfRel = 0, gLF = 1.0f, lz[2][2] = {};
    float e = 1.0f, lastGain = 1.0f;
    std::vector<float> box, box2;
    int boxPos = 0, boxPos2 = 0, n1 = 1, n2 = 1;
    double boxSum = 0.0, boxSum2 = 0.0;

    std::vector<float> hist[2] = { std::vector<float> (32, 0.0f), std::vector<float> (32, 0.0f) };
    int histPos = 0;
};

//==============================================================================
// Limitador de dos constantes de tiempo: la ganancia sigue de cerca el recorte necesario
// (release rápido, conserva el cuerpo de los transitorios) y una cola lenta suaviza.
class DualStage
{
public:
    // lookaheadMs : ventana de ataque; relMinMs/relMaxMs : rango del release.
    // Con relMin == relMax el release es fijo; con relMin < relMax se adapta al programa
    // (transitorios aislados -> rápido, limitación sostenida -> lento).
    void prepare (double sampleRate, float lookaheadMs, float relFastMs, float slowMs, float tailAmount)
    {
        sr = sampleRate;
        la = std::max (2, (int) std::lround (lookaheadMs * 0.001 * sampleRate));
        relMin = relMax = relFastMs;
        slowCoeffMs = slowMs; beta = tailAmount;
        avgCoeff = 1.0f - std::exp (-1.0f / (0.25f * (float) sampleRate)); // 250 ms

        const int audioLen = la - 1 + kDetDelay + 2;
        audio.assign ((size_t) audioLen * 2, 0.0f);
        audioLenFrames = audioLen;

        dqVal.assign ((size_t) la + 2, 1.0f);
        dqIdx.assign ((size_t) la + 2, 0);
        n1 = (la + 2) / 2; n2 = la + 1 - n1;
        box.assign ((size_t) n1, 1.0f);
        box2.assign ((size_t) n2, 1.0f);

        reset();
    }

    void reset()
    {
        std::fill (audio.begin(), audio.end(), 0.0f);
        std::fill (dqVal.begin(), dqVal.end(), 1.0f);
        std::fill (dqIdx.begin(), dqIdx.end(), 0);
        std::fill (box.begin(), box.end(), 1.0f);
        std::fill (box2.begin(), box2.end(), 1.0f);
        boxPos2 = 0; boxSum2 = (double) n2;
        for (auto& v : hist) std::fill (v.begin(), v.end(), 0.0f);
        histPos = 0;
        dqHead = dqCount = 0;
        t = 0;
        e = 1.0f;
        boxPos = 0;
        boxSum = (double) n1;
        audioPos = 0;
        grAvgDb = 0.0f;
        relCoeff = coeffForMs (relMin);
        slowCoeff = coeffForMs (slowCoeffMs); s = 1.0f;
        sinceCoeff = 0;
        lastGain = 1.0f;
    }

    // Retardo total de la etapa en muestras
    int latency() const { return la - 1 + kDetDelay; }

    // Graves con release más lento (menos distorsión de modulación en graves).
    // fc: frecuencia de corte; ms: release del gain de graves (0 = desactivado).
    void setLowBandSlow (float fcHz, float ms)
    {
        lfOn = ms > 0.0f;
        if (! lfOn) return;
        const float K = std::tan (kPi * fcHz / (float) sr), Q = 0.70710678f;
        const float n = 1.0f / (1.0f + K / Q + K * K);
        lb0 = K * K * n; lb1 = 2.0f * lb0; lb2 = lb0;
        la1 = 2.0f * (K * K - 1.0f) * n; la2 = (1.0f - K / Q + K * K) * n;
        lfRel = coeffForMs (ms);
        for (auto& s : lz) s[0] = s[1] = 0.0f;
        gLF = 1.0f;
    }

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

        // cola lenta: seguidor lento de la envolvente rápida; la ganancia final mezcla ambas (en dB)
        if (e < s) s = e; else s += slowCoeff * (e - s);
        const float g = std::pow (e, 1.0f - beta) * std::pow (s, beta);

        // 5. dos medias móviles en cascada (kernel triangular, ataque más suave)
        boxSum += (double) g - (double) box[(size_t) boxPos];
        box[(size_t) boxPos] = g;
        boxPos = (boxPos + 1) % n1;
        const float s1v = (float) (boxSum / (double) n1);
        boxSum2 += (double) s1v - (double) box2[(size_t) boxPos2];
        box2[(size_t) boxPos2] = s1v;
        boxPos2 = (boxPos2 + 1) % n2;
        const float a = (float) (boxSum2 / (double) n2);
        lastGain = a;

        // 6. audio retrasado: 'la - 1 + kDetDelay' muestras
        const int len = audioLenFrames;
        const int readPos = (audioPos + len - (la - 1 + kDetDelay)) % len;
        audio[(size_t) audioPos * 2] = inL;
        audio[(size_t) audioPos * 2 + 1] = inR;
        const float dl = audio[(size_t) readPos * 2];
        const float dr = audio[(size_t) readPos * 2 + 1];
        audioPos = (audioPos + 1) % len;

        if (lfOn)
        {
            // gain de graves: baja igual que 'a', pero recupera lento
            if (a < gLF) gLF = a; else gLF += lfRel * (a - gLF);
            const float d[2] = { dl, dr };
            float o[2];
            for (int ch = 0; ch < 2; ++ch)
            {
                const float lp = lb0 * d[ch] + lz[ch][0];
                lz[ch][0] = lb1 * d[ch] - la1 * lp + lz[ch][1];
                lz[ch][1] = lb2 * d[ch] - la2 * lp;
                o[ch] = d[ch] * a - lp * (a - gLF);
            }
            outL = o[0]; outR = o[1];
        }
        else
        {
            outL = dl * a;
            outR = dr * a;
        }
        ++t;
    }

    float currentGain() const { return lastGain; }

private:
    static constexpr int kTaps = 24;
    static constexpr int kDetDelay = 12; // retardo del detector (centro del FIR)
    static constexpr int kHistMask = 31;

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
                    const float w = 0.5f * (1.0f + std::cos (kPi * dt / 12.5f));
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
    float relMin = 40.0f, relMax = 40.0f, slowCoeffMs = 30.0f, beta = 0.2f, slowCoeff = 0.0f, s = 1.0f;
    float avgCoeff = 0.0f, relCoeff = 0.0f, grAvgDb = 0.0f;
    int sinceCoeff = 0;

    std::vector<float> audio;
    int audioLenFrames = 1, audioPos = 0;

    std::vector<float> dqVal;
    std::vector<int> dqIdx;
    int dqHead = 0, dqCount = 0;
    long t = 0;

    bool lfOn = false;
    float lb0 = 0, lb1 = 0, lb2 = 0, la1 = 0, la2 = 0, lfRel = 0, gLF = 1.0f, lz[2][2] = {};
    float e = 1.0f, lastGain = 1.0f;
    std::vector<float> box, box2;
    int boxPos = 0, boxPos2 = 0, n1 = 1, n2 = 1;
    double boxSum = 0.0, boxSum2 = 0.0;

    std::vector<float> hist[2] = { std::vector<float> (32, 0.0f), std::vector<float> (32, 0.0f) };
    int histPos = 0;
};

//==============================================================================
// Envuelve una etapa para ofrecer modo LINK (una ganancia para L y R) o UNLINK (cada canal
// con su propia ganancia, más estéreo). Las tres instancias corren siempre y se mezclan con
// un crossfade suave, así el cambio no hace clics (y la mezcla nunca supera el techo).
template <class Stage>
class LinkableStage
{
public:
    template <class... Args>
    void prepare (double sr, Args... args)
    {
        linked.prepare (sr, args...);
        left.prepare (sr, args...);
        right.prepare (sr, args...);
        mix = 0.0f;
        mixCoeff = 1.0f - std::exp (-1.0f / (0.01f * (float) sr));
    }
    int latency() const { return linked.latency(); }

    void process (float inL, float inR, float ceiling, bool truePeak, bool unlink, float& outL, float& outR)
    {
        float l1, r1, l2, r2, t1, t2;
        linked.process (inL, inR, ceiling, truePeak, l1, r1);
        left.process (inL, inL, ceiling, truePeak, l2, t1);
        right.process (inR, inR, ceiling, truePeak, t2, r2);
        mix += mixCoeff * ((unlink ? 1.0f : 0.0f) - mix);
        outL = l1 + mix * (l2 - l1);
        outR = r1 + mix * (r2 - r1);
    }
    float currentGain() const
    {
        return mix > 0.5f ? std::min (left.currentGain(), right.currentGain()) : linked.currentGain();
    }

private:
    Stage linked, left, right;
    float mix = 0.0f, mixCoeff = 0.01f;
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

//==============================================================================
// Crossover Linkwitz-Riley de 4º orden (2 x Butterworth de 2º orden)
class LR4Split
{
public:
    void prepare (double sampleRate, float freq)
    {
        const double K = std::tan (3.14159265358979323846 * (double) freq / sampleRate);
        const double q = 0.70710678118654752440;
        const double a0 = 1.0 + K / q + K * K;
        a1 = 2.0 * (K * K - 1.0) / a0;
        a2 = (1.0 - K / q + K * K) / a0;
        lb0 = K * K / a0; lb1 = 2.0 * lb0; lb2 = lb0;
        hb0 = 1.0 / a0;   hb1 = -2.0 * hb0; hb2 = hb0;
        reset();
    }
    void reset() { for (auto& s : st) s = State(); }

    // Separa x en grave (lo) y agudo (hi); ch = 0 / 1
    void split (int ch, float x, float& lo, float& hi)
    {
        auto& s = st[ch];
        const double l1 = tick (lb0, lb1, lb2, s.l1, (double) x);
        const double l2 = tick (lb0, lb1, lb2, s.l2, l1);
        const double h1 = tick (hb0, hb1, hb2, s.h1, (double) x);
        const double h2 = tick (hb0, hb1, hb2, s.h2, h1);
        lo = (float) l2;
        hi = (float) h2;
    }

private:
    struct Bq { double x1 = 0, x2 = 0, y1 = 0, y2 = 0; };
    struct State { Bq l1, l2, h1, h2; };

    double tick (double b0, double b1, double b2, Bq& z, double x) const
    {
        const double y = b0 * x + b1 * z.x1 + b2 * z.x2 - a1 * z.y1 - a2 * z.y2;
        z.x2 = z.x1; z.x1 = x; z.y2 = z.y1; z.y1 = y;
        return y;
    }

    double a1 = 0, a2 = 0, lb0 = 0, lb1 = 0, lb2 = 0, hb0 = 0, hb1 = 0, hb2 = 0;
    State st[2];
};

//==============================================================================
// Limitador multibanda (3 bandas) con un LookaheadStage por banda: los graves ya no
// "agachan" a los agudos (menos bombeo). Cada banda tiene su propio release.
class MultibandStage
{
public:
    struct BandCfg { float relMin, relMax; };

    void prepare (double sampleRate, float lookaheadMs, float xoverLow, float xoverHigh,
                  const BandCfg cfg[3])
    {
        x1.prepare (sampleRate, xoverLow);
        x2.prepare (sampleRate, xoverHigh);
        x2lowComp.prepare (sampleRate, xoverHigh); // compensa la fase de la banda grave
        for (int b = 0; b < 3; ++b)
            band[b].prepare (sampleRate, lookaheadMs, cfg[b].relMin, cfg[b].relMax);
    }

    void reset()
    {
        x1.reset(); x2.reset(); x2lowComp.reset();
        for (auto& b : band) b.reset();
    }

    int latency() const { return band[0].latency(); }

    // ceilings: techo de cada banda (lineal)
    void process (float inL, float inR, const float ceilings[3], float& outL, float& outR)
    {
        float sig[3][2];
        const float in[2] = { inL, inR };
        for (int ch = 0; ch < 2; ++ch)
        {
            float lo, rest, mid, hi, lA, lB;
            x1.split (ch, in[ch], lo, rest);
            x2.split (ch, rest, mid, hi);
            x2lowComp.split (ch, lo, lA, lB); // lA + lB = lo con la fase de x2
            sig[0][ch] = lA + lB;
            sig[1][ch] = mid;
            sig[2][ch] = hi;
        }
        outL = outR = 0.0f;
        gain = 1.0f;
        for (int b = 0; b < 3; ++b)
        {
            float l, r;
            band[b].process (sig[b][0], sig[b][1], ceilings[b], false, l, r);
            outL += l;
            outR += r;
            minGain = b == 0 ? band[b].currentGain() : std::min (minGain, band[b].currentGain());
        }
        gain = minGain;
    }

    // Reducción "visible": la mayor de las tres bandas
    float currentGain() const { return gain; }

private:
    LR4Split x1, x2, x2lowComp;
    LookaheadStage band[3];
    float gain = 1.0f, minGain = 1.0f;
};

//==============================================================================
// Clipper suave con sobremuestreo 4x (FIR lineal de 65 taps). Recorta los picos
// "pasando" por una curva suave en lugar de bajar la ganancia: conserva la pegada de los
// transitorios y reduce el bombeo. Latencia: 16 muestras.
class SoftClipper4x
{
public:
    static constexpr int kLatency = 16;

    SoftClipper4x()
    {
        // sinc con corte en 1/4 de la frecuencia de muestreo sobremuestreada (= Nyquist original)
        for (int i = 0; i < kN; ++i)
        {
            const float m = (float) (i - kC);
            const float x = kPi * m * 0.25f;
            const float sinc = std::abs (x) < 1.0e-6f ? 1.0f : std::sin (x) / x;
            const float w = 0.5f * (1.0f + std::cos (kPi * m / (float) (kC + 1)));
            h[i] = 0.25f * sinc * w;
        }
        reset();
    }

    void reset()
    {
        for (auto& v : xin) std::fill (v, v + kXLen, 0.0f);
        for (auto& v : cbuf) std::fill (v, v + kCLen, 0.0f);
        xPos = 0; cPos = 0;
    }

    // ceiling lineal; knee 0..1 (fracción del techo donde empieza la curva)
    void process (float inL, float inR, float ceiling, float kneeFrac, float& outL, float& outR)
    {
        const float in[2] = { inL, inR };
        float out[2];
        const float T = ceiling * kneeFrac;
        for (int ch = 0; ch < 2; ++ch)
        {
            // entrada en el historial (polifásico: x[n-j])
            xin[ch][xPos] = in[ch];
            float up[4];
            for (int p = 0; p < 4; ++p)
            {
                float y = 0.0f;
                // y_up[4n+p] = 4 * sum_j h[4j+p] x[n-j]
                for (int j = 0, i = p; i < kN; ++j, i += 4)
                    y += h[i] * xin[ch][(xPos - j + kXLen) % kXLen];
                up[p] = 4.0f * y;
            }
            // clip suave a 4x
            for (int p = 0; p < 4; ++p)
            {
                const float a = std::abs (up[p]);
                float v = up[p];
                if (a > T)
                {
                    const float r = ceiling - T;
                    v = (up[p] > 0.0f ? 1.0f : -1.0f) * (T + r * std::tanh ((a - T) / r));
                }
                cbuf[ch][(cPos + p) % kCLen] = v;
            }
            // decimación: filtra a 4x y toma la muestra 4n
            float y = 0.0f;
            const int m = (cPos + 0) % kCLen; // posición de la muestra 4n
            for (int i = 0; i < kN; ++i)
                y += h[i] * cbuf[ch][((m - i) % kCLen + kCLen) % kCLen];
            out[ch] = y;
        }
        xPos = (xPos + 1) % kXLen;
        cPos = (cPos + 4) % kCLen;
        outL = out[0];
        outR = out[1];
    }

private:
    static constexpr int kN = 65, kC = 32;
    static constexpr int kXLen = 32;       // >= 17 muestras de historial
    static constexpr int kCLen = 128;      // >= 65 + 4
    float h[kN];
    float xin[2][kXLen];
    float cbuf[2][kCLen];
    int xPos = 0, cPos = 0;
};
} // namespace limdsp
