#pragma once

#include <juce_core/juce_core.h>
#include <cmath>

//==============================================================================
// Motor DSP de Dark Transformer: un solo color, pensado para dar peso,
// profundidad y contenido armónico. Es un modelo de circuito (no una captura).
//
// Cadena por canal:
//   [frecuencia base]   pérdida de sub-graves del transformador de entrada
//   [oversampling 4x]   transformador de entrada (el núcleo satura más en
//                       graves, como el flujo magnético real)
//                       -> etapa clase A con sesgo que se mueve con la señal
//                          (2º armónico dinámico)
//                       -> segunda etapa simétrica (3er armónico al empujar)
//                       -> transformador de salida
//                       -> resonancia de agudos y bloqueo de DC
//   [frecuencia base]   curva de "profundidad": peso abajo, menos barro en
//                       medios-bajos y aire arriba
//
// A nivel bajo la ganancia es unitaria: el DRIVE cambia el color, no el volumen.
//
// Como un equipo real: L y R tienen tolerancias distintas de componentes y
// cada canal deriva lento y por su cuenta (punto de trabajo, núcleo, ganancia),
// así el color se mueve todo el tiempo y nunca queda estático.
//==============================================================================
namespace iron
{
    struct Biquad
    {
        float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
        float z1 = 0, z2 = 0;

        void reset() { z1 = z2 = 0.0f; }
        void setIdentity() { b0 = 1; b1 = b2 = a1 = a2 = 0; }
        void setLowpass (double sr, double f, double q);
        void setLowShelf (double sr, double f, double db);
        void setHighShelf (double sr, double f, double db);
        void setBell (double sr, double f, double q, double db);

        inline float process (float x) noexcept
        {
            const float y = b0 * x + z1;
            z1 = b1 * x - a1 * y + z2;
            z2 = b2 * x - a2 * y;
            return y;
        }
    };

    struct OnePole
    {
        float c = 1.0f, s = 0.0f;
        void setLowpass (double sr, double f) { c = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * f / sr)); }
        void reset() { s = 0.0f; }
        inline float lp (float x) noexcept { s += c * (x - s); return s; }
        inline float hp (float x) noexcept { return x - lp (x); }
    };

    // Fuente aleatoria suave (random walk filtrado) para la deriva analógica
    struct Drift
    {
        void prepare (double controlRate, float hz, juce::int64 seed);
        float next() noexcept;

        juce::Random rng;
        float target = 0.0f, smooth1 = 0.0f, smooth2 = 0.0f, coeff = 0.0f;
        int period = 1, counter = 0;
    };

    // Un canal. channelIndex 0 = L, 1 = R: cada uno tiene sus propias
    // tolerancias de componentes y su propia deriva lenta.
    class Channel
    {
    public:
        void prepare (double baseRate, double osRate, int channelIndex);
        void reset();

        inline float processPre (float x) noexcept { return inHp.hp (x); }
        float processOversampled (float x) noexcept;
        inline float processPost (float x) noexcept { return air.process (mud.process (weight.process (x))); }

    private:
        void updateControl() noexcept;

        static constexpr int kControlStep = 32;

        OnePole inHp, inCore, outCore, envelope, dcBlock;
        Biquad res, weight, mud, air;

        int tol = 0;
        double os = 192000.0;
        float coreBaseHz = 110.0f, resBaseHz = 32000.0f;
        Drift driftA, driftB, driftC;
        int controlCounter = 0;

        struct Current
        {
            float inDrive = 2.0f, inBias = 0.15f, classABias = 0.42f, classADrive = 0.75f,
                  outAmount = 0.9f, gain = 1.0f;
        } cur;
    };
}
