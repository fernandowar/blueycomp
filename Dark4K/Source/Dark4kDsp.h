#pragma once
// DSP sin dependencias de JUCE (solo std): biquads para el EQ/filtros estilo SSL 4000 y puerta básica.
#include <cmath>
#include <algorithm>

namespace d4k
{
constexpr float kPi = 3.14159265358979f;

struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] { 0, 0 }, z2[2] { 0, 0 };

    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0.0f; }
    void set (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
        a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
    }
    void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }

    inline float process (int ch, float x)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

    // Respuesta en magnitud (dB) para pruebas
    double magDb (double f, double sr) const
    {
        const double w = 2.0 * kPi * f / sr;
        const double c1 = std::cos (w), s1 = std::sin (w), c2 = std::cos (2 * w), s2 = std::sin (2 * w);
        const double nr = b0 + b1 * c1 + b2 * c2, ni = -(b1 * s1 + b2 * s2);
        const double dr = 1 + a1 * c1 + a2 * c2, di = -(a1 * s1 + a2 * s2);
        return 10.0 * std::log10 ((nr * nr + ni * ni) / (dr * dr + di * di));
    }

    // --- diseños (RBJ) ---
    void peak (double sr, double f, double gainDb, double Q)
    {
        f = std::min (f, sr * 0.45);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / sr, al = std::sin (w) / (2 * Q), c = std::cos (w);
        set (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void highShelf (double sr, double f, double gainDb, double Q)
    {
        f = std::min (f, sr * 0.45);
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / sr, al = std::sin (w) / (2 * Q), c = std::cos (w);
        const double t = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) + (A - 1) * c + t), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - t),
             (A + 1) - (A - 1) * c + t, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - t);
    }
    void lowShelf (double sr, double f, double gainDb, double Q)
    {
        const double A = std::pow (10.0, gainDb / 40.0), w = 2 * kPi * f / sr, al = std::sin (w) / (2 * Q), c = std::cos (w);
        const double t = 2 * std::sqrt (A) * al;
        set (A * ((A + 1) - (A - 1) * c + t), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - t),
             (A + 1) + (A - 1) * c + t, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - t);
    }
    void highPass2 (double sr, double f, double Q)
    {
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * Q), c = std::cos (w);
        set ((1 + c) / 2, -(1 + c), (1 + c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void lowPass2 (double sr, double f, double Q)
    {
        f = std::min (f, sr * 0.45);
        const double w = 2 * kPi * f / sr, al = std::sin (w) / (2 * Q), c = std::cos (w);
        set ((1 - c) / 2, 1 - c, (1 - c) / 2, 1 + al, -2 * c, 1 - al);
    }
    void highPass1 (double sr, double f)
    {
        const double K = std::tan (kPi * f / sr);
        set (1.0, -1.0, 0.0, 1.0 + K, K - 1.0, 0.0);
        // normalizado: b0 = 1/(1+K)
    }
};

// Características de los dos "colores" de EQ del 4000: Brown (02) y Black (242)
// Voicing medido sobre curvas de referencia SSL 4000 E (Black):
//  - Shelf de agudos: el "8 kHz" de la perilla corresponde a un punto de media ganancia cerca de 2 kHz.
//  - Rangos de ganancia: shelf +-16.5 dB, campana LF/HF +-18 dB, HMF/LMF de +-15 (Q 0.1) a +-18 dB (Q 3.5).
struct EqVoicing
{
    bool black = true;
    // Q: la perilla va de 3 (angosto, izquierda) a .5 (ancho, derecha). Puntos medidos sobre curvas de referencia
    // (HMF: Q .1 -> 0.175, 3.5 -> 1.42; LMF es ~1.22x mas angosta), interpolados en escala logaritmica.
    static double qMap (double label, bool lmf = false)
    {
        static const double x[4] { 0.5, 1.5, 2.0, 3.0 }, q[4] { 0.175, 0.55, 0.84, 1.42 };
        const double l = std::clamp (label, 0.5, 3.0);
        double r = q[3];
        for (int i = 0; i < 3; ++i)
            if (l <= x[i + 1]) { r = q[i] * std::pow (q[i + 1] / q[i], (l - x[i]) / (x[i + 1] - x[i])); break; }
        return lmf ? r * 1.22 : r;
    }
    double bellQ (double label, bool lmf = false) const { return qMap (label, lmf); }
    double bellQFixed() const { return 0.84; }
    double shelfQHf() const { return 0.42; }
    double shelfQLf() const { return 0.45; }
    double shelfQ() const { return 0.42; }
    static double hfShelfFreq (double knobHz) { return knobHz * 0.26; }   // 8k -> ~2.07k
    static double lfShelfFreq (double knobHz) { return knobHz * 2.45; }   // 60 Hz -> ~147 Hz
    static double hmfFreq (double knobHz) { return knobHz * 1.334; }      // 3.5k -> ~4.67k
    static double lmfFreq (double knobHz) { return knobHz * 0.806; }      // 500 -> ~403 Hz
    static double shelfGain (double g) { return g * 1.1; }
    static double bellGain (double g) { return g * 1.2; }
    static double midGain (double g, double label)
    {
        static const double x[4] { 0.5, 1.5, 2.0, 3.0 }, m[4] { 0.982, 1.02, 1.157, 1.2 };
        const double l = std::clamp (label, 0.5, 3.0);
        for (int i = 0; i < 3; ++i)
            if (l <= x[i + 1]) return g * (m[i] + (m[i + 1] - m[i]) * (l - x[i]) / (x[i + 1] - x[i]));
        return g * m[3];
    }
};
} // namespace d4k
