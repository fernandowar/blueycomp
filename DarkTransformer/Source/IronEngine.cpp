#include "IronEngine.h"

namespace iron
{
    //==========================================================================
    // RBJ cookbook, normalizado a a0 = 1
    void Biquad::setLowpass (double sr, double f, double q)
    {
        const double w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        const double a0 = 1.0 + alpha;
        b0 = (float) ((1.0 - cw) * 0.5 / a0);
        b1 = (float) ((1.0 - cw) / a0);
        b2 = b0;
        a1 = (float) (-2.0 * cw / a0);
        a2 = (float) ((1.0 - alpha) / a0);
    }

    void Biquad::setLowShelf (double sr, double f, double db)
    {
        if (std::abs (db) < 0.01) { setIdentity(); return; }
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / 2.0 * std::sqrt (2.0); // S = 1
        const double sA = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1) + (A - 1) * cw + sA;
        b0 = (float) (A * ((A + 1) - (A - 1) * cw + sA) / a0);
        b1 = (float) (2 * A * ((A - 1) - (A + 1) * cw) / a0);
        b2 = (float) (A * ((A + 1) - (A - 1) * cw - sA) / a0);
        a1 = (float) (-2 * ((A - 1) + (A + 1) * cw) / a0);
        a2 = (float) (((A + 1) + (A - 1) * cw - sA) / a0);
    }

    void Biquad::setHighShelf (double sr, double f, double db)
    {
        if (std::abs (db) < 0.01) { setIdentity(); return; }
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / 2.0 * std::sqrt (2.0);
        const double sA = 2.0 * std::sqrt (A) * alpha;
        const double a0 = (A + 1) - (A - 1) * cw + sA;
        b0 = (float) (A * ((A + 1) + (A - 1) * cw + sA) / a0);
        b1 = (float) (-2 * A * ((A - 1) + (A + 1) * cw) / a0);
        b2 = (float) (A * ((A + 1) + (A - 1) * cw - sA) / a0);
        a1 = (float) (2 * ((A - 1) - (A + 1) * cw) / a0);
        a2 = (float) (((A + 1) - (A - 1) * cw - sA) / a0);
    }

    void Biquad::setBell (double sr, double f, double q, double db)
    {
        if (std::abs (db) < 0.01) { setIdentity(); return; }
        const double A = std::pow (10.0, db / 40.0), w = juce::MathConstants<double>::twoPi * f / sr;
        const double cw = std::cos (w), alpha = std::sin (w) / (2.0 * q);
        const double a0 = 1 + alpha / A;
        b0 = (float) ((1 + alpha * A) / a0);
        b1 = (float) (-2 * cw / a0);
        b2 = (float) ((1 - alpha * A) / a0);
        a1 = b1;
        a2 = (float) ((1 - alpha / A) / a0);
    }

    //==========================================================================
    namespace
    {
        // tanh con sesgo, pendiente 1 y salida 0 en el origen
        inline float biasedTanh (float x, float k, float b) noexcept
        {
            const float tb = std::tanh (b);
            return (std::tanh (k * x + b) - tb) / (k * (1.0f - tb * tb));
        }

    }

    //==========================================================================
    // Constantes del color (afinadas midiendo armónicos y respuesta)
    namespace sig
    {
        constexpr float inCoreHz   = 110.0f, inCoreAmount = 1.0f, inCoreDrive = 2.0f, inCoreBias = 0.15f;
        constexpr float broadDrive = 0.6f,   broadBias = 0.08f;
        constexpr float classADrive = 0.75f, classABias = 0.42f, classAMove = 0.20f;
        constexpr float pushDrive  = 0.45f;
        constexpr float outCoreHz  = 140.0f, outCoreAmount = 0.9f, outCoreDrive = 1.8f, outCoreBias = 0.12f;
        constexpr float resHz = 32000.0f, resQ = 0.9f;
    }

    //==========================================================================
    // Tolerancias entre canales: como en un equipo real, L y R no son iguales.
    // Diferencias chicas (para no mover la imagen estéreo) pero audibles en el
    // "ancho" y la textura, igual que la tolerancia del Dark VCA.
    namespace
    {
        struct Tolerance
        {
            float gain, inDrive, inBias, coreHz, classABias, classADrive, outAmount, resHz;
            juce::int64 seed;
        };

        constexpr Tolerance tolerances[2] = {
            // L
            { 1.000f, 1.00f, 0.000f, 1.00f, 0.000f, 1.00f, 1.00f, 1.00f, 0x5EED1 },
            // R: -0.12 dB, núcleo un poco más "duro" y más grave, otro punto de trabajo
            { 0.986f, 1.06f, 0.030f, 0.93f, 0.060f, 0.96f, 1.06f, 1.08f, 0x5EED2 },
        };

        // Amplitudes de la deriva continua (alrededor del valor nominal)
        constexpr float driftBias  = 0.16f;  // punto de trabajo clase A
        constexpr float driftDrive = 0.12f;  // ±12 % de saturación del núcleo
        constexpr float driftGain  = 0.012f; // ±0.1 dB
        constexpr float driftCore  = 0.15f;  // ±15 % de la frecuencia del núcleo
        constexpr float driftRes   = 0.10f;  // ±10 % de la resonancia de agudos
    }

    void Drift::prepare (double controlRate, float hz, juce::int64 seed)
    {
        rng.setSeed (seed);
        // nuevo destino aleatorio unas 2 veces por ciclo; suavizado doble
        period = juce::jmax (1, (int) (controlRate / (2.0 * hz)));
        coeff = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * hz / controlRate));
        counter = 0;
        target = rng.nextFloat() * 2.0f - 1.0f;
        smooth1 = smooth2 = target;
    }

    float Drift::next() noexcept
    {
        if (--counter <= 0)
        {
            counter = period;
            target = rng.nextFloat() * 2.0f - 1.0f;
        }
        smooth1 += coeff * (target - smooth1);
        smooth2 += coeff * (smooth1 - smooth2);
        return smooth2;
    }

    void Channel::prepare (double baseRate, double osRate, int channelIndex)
    {
        tol = juce::jlimit (0, 1, channelIndex);
        const auto& t = tolerances[tol];
        os = osRate;

        inHp.setLowpass (baseRate, 10.0);
        coreBaseHz = sig::inCoreHz * t.coreHz;
        inCore.setLowpass (osRate, coreBaseHz);
        outCore.setLowpass (osRate, sig::outCoreHz * t.coreHz);
        envelope.setLowpass (osRate, tol == 0 ? 6.0 : 6.6);
        dcBlock.setLowpass (osRate, 4.0);
        resBaseHz = sig::resHz * t.resHz;
        res.setLowpass (osRate, juce::jmin ((double) resBaseHz, 0.42 * osRate), sig::resQ);

        weight.setLowShelf (baseRate, 70.0, 1.0);           // peso abajo
        mud.setBell (baseRate, 350.0, 0.8, -0.6);           // menos barro: más profundidad
        air.setHighShelf (baseRate, juce::jmin (11000.0, 0.4 * baseRate), 0.7); // aire

        // Deriva lenta e independiente por canal (control cada kControlStep muestras)
        const double controlRate = osRate / kControlStep;
        driftA.prepare (controlRate, 0.23f, t.seed);
        driftB.prepare (controlRate, 0.71f, t.seed * 7 + 3);
        driftC.prepare (controlRate, 0.11f, t.seed * 13 + 5);
        reset();
        updateControl();
    }

    void Channel::reset()
    {
        inHp.reset(); inCore.reset(); outCore.reset(); envelope.reset(); dcBlock.reset();
        res.reset(); weight.reset(); mud.reset(); air.reset();
        controlCounter = 0;
    }

    void Channel::updateControl() noexcept
    {
        const auto& t = tolerances[tol];
        const float a = driftA.next(), b = driftB.next(), c = driftC.next();

        cur.inDrive     = sig::inCoreDrive * t.inDrive * (1.0f + driftDrive * (0.7f * a + 0.3f * b));
        cur.inBias      = sig::inCoreBias + t.inBias + 0.06f * c;
        cur.classABias  = sig::classABias + t.classABias + driftBias * (0.6f * b + 0.4f * c);
        cur.classADrive = sig::classADrive * t.classADrive * (1.0f + 0.08f * a);
        cur.outAmount   = sig::outCoreAmount * t.outAmount * (1.0f + 0.05f * c);
        cur.gain        = t.gain * (1.0f + driftGain * (0.5f * a + 0.5f * c));

        // la frecuencia del núcleo también se mueve (el flujo depende de la temperatura)
        inCore.setLowpass (os, coreBaseHz * (1.0f + driftCore * a));
        res.setLowpass (os, juce::jmin ((double) resBaseHz * (1.0 + driftRes * b), 0.42 * os), sig::resQ);
    }

    float Channel::processOversampled (float x) noexcept
    {
        if (--controlCounter <= 0)
        {
            controlCounter = kControlStep;
            updateControl();
        }

        // 1) Transformador de entrada: satura la parte grave (flujo del núcleo)
        //    y encima una saturación general leve
        {
            const float low = inCore.lp (x);
            x += sig::inCoreAmount * (biasedTanh (low, cur.inDrive, cur.inBias) - low);
            x = biasedTanh (x, sig::broadDrive, sig::broadBias);
        }

        // 2) Etapa clase A: curva sesgada (2º armónico). La señal fuerte corre el
        //    punto de trabajo y además deriva sola: el color nunca queda quieto
        {
            const float shift = sig::classAMove * envelope.lp (std::abs (x));
            x = biasedTanh (x - shift, cur.classADrive, cur.classABias)
              - biasedTanh (-shift, cur.classADrive, cur.classABias);
        }

        // 3) Segunda etapa simétrica: aparece 3er armónico cuando se empuja
        x = biasedTanh (x, sig::pushDrive, 0.0f);

        // 4) Transformador de salida
        {
            const float low = outCore.lp (x);
            x += cur.outAmount * (biasedTanh (low, sig::outCoreDrive, sig::outCoreBias) - low);
        }

        // 5) Resonancia de agudos, DC y tolerancia de ganancia del canal
        x = res.process (x);
        return dcBlock.hp (x) * cur.gain;
    }
}
