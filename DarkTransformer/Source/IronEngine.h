#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <cmath>

//==============================================================================
// Motor DSP de Dark Transformer: modelos de circuito (no capturas) de
// transformadores de línea y preamps clásicos.
//
// Cadena por canal:
//   [frecuencia base]   corte de graves del transformador de entrada
//   [oversampling 4x]   núcleo de entrada (satura más en graves, como el flujo
//                       magnético) -> etapa de ganancia (clase A / discreta /
//                       válvula) -> núcleo de salida -> resonancia de agudos
//                       -> bloqueo de DC
//   [frecuencia base]   EQ de la "personalidad" del equipo
//
// Todo está normalizado para tener ganancia unitaria a nivel bajo: el DRIVE
// solo cambia el color, no el volumen.
//==============================================================================
namespace iron
{
    enum Voice
    {
        nickel = 0, // transformador de núcleo de níquel: rápido, abierto, sutil
        steel,      // núcleo de acero: graves gordos, agudos suaves
        vintage,    // línea vintage americana: pares tempranos, luego 3er armónico
        britA,      // preamp británico clase A con transformadores
        usDiscrete, // preamp americano discreto (op-amp), medios al frente
        tube,       // preamp a válvula: 2º armónico y compresión suave
        numVoices
    };

    juce::StringArray voiceNames();

    struct VoiceParams
    {
        float lowCutHz;      // pérdida de graves del transformador
        float coreHz;        // dónde empieza a dominar el flujo (graves)
        float coreAmount;    // cuánto del núcleo satura (0..1)
        float coreDrive;     // qué tan temprano satura el núcleo
        float coreBias;      // asimetría del núcleo (armónicos pares)
        int   stage;         // 0 ninguna, 1 clase A, 2 discreta, 3 válvula
        float stageDrive;
        float stageBias;
        float outCoreAmount; // transformador de salida (0 = no tiene)
        float resHz;         // resonancia de agudos (en la tasa sobremuestreada)
        float resQ;
        float lowShelfHz, lowShelfDb;
        float bellHz, bellDb, bellQ;
        float highShelfHz, highShelfDb;
    };

    const VoiceParams& paramsFor (int voice);

    //==========================================================================
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

    //==========================================================================
    // Un canal del motor
    class Channel
    {
    public:
        void prepare (double baseRate, double osRate);
        void setVoice (int voice);
        void reset();

        // Antes del oversampling (frecuencia base)
        inline float processPre (float x) noexcept { return inHp.hp (x); }
        // Dentro del oversampling
        float processOversampled (float x) noexcept;
        // Después del oversampling (frecuencia base)
        inline float processPost (float x) noexcept { return highShelf.process (bell.process (lowShelf.process (x))); }

    private:
        double baseRate = 48000.0, osRate = 192000.0;
        int voice = nickel;
        VoiceParams p {};

        OnePole inHp, coreLp, outCoreLp, envelope, dcBlock;
        Biquad res, lowShelf, bell, highShelf;
        float envCoeff = 0.0f;
    };
}
