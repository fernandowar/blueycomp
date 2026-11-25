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

    void Channel::prepare (double baseRate, double osRate)
    {
        inHp.setLowpass (baseRate, 10.0);
        inCore.setLowpass (osRate, sig::inCoreHz);
        outCore.setLowpass (osRate, sig::outCoreHz);
        envelope.setLowpass (osRate, 6.0);
        dcBlock.setLowpass (osRate, 4.0);
        res.setLowpass (osRate, juce::jmin ((double) sig::resHz, 0.42 * osRate), sig::resQ);

        weight.setLowShelf (baseRate, 70.0, 1.0);           // peso abajo
        mud.setBell (baseRate, 350.0, 0.8, -0.6);           // menos barro: más profundidad
        air.setHighShelf (baseRate, juce::jmin (11000.0, 0.4 * baseRate), 0.7); // aire
        reset();
    }

    void Channel::reset()
    {
        inHp.reset(); inCore.reset(); outCore.reset(); envelope.reset(); dcBlock.reset();
        res.reset(); weight.reset(); mud.reset(); air.reset();
    }

    float Channel::processOversampled (float x) noexcept
    {
        // 1) Transformador de entrada: satura la parte grave (flujo del núcleo)
        //    y encima una saturación general leve
        {
            const float low = inCore.lp (x);
            x += sig::inCoreAmount * (biasedTanh (low, sig::inCoreDrive, sig::inCoreBias) - low);
            x = biasedTanh (x, sig::broadDrive, sig::broadBias);
        }

        // 2) Etapa clase A: curva sesgada (2º armónico). La señal fuerte corre el
        //    punto de trabajo, así que el color "respira" con la dinámica
        {
            const float shift = sig::classAMove * envelope.lp (std::abs (x));
            x = biasedTanh (x - shift, sig::classADrive, sig::classABias)
              - biasedTanh (-shift, sig::classADrive, sig::classABias);
        }

        // 3) Segunda etapa simétrica: aparece 3er armónico cuando se empuja
        x = biasedTanh (x, sig::pushDrive, 0.0f);

        // 4) Transformador de salida
        {
            const float low = outCore.lp (x);
            x += sig::outCoreAmount * (biasedTanh (low, sig::outCoreDrive, sig::outCoreBias) - low);
        }

        // 5) Resonancia de agudos y DC
        x = res.process (x);
        return dcBlock.hp (x);
    }
}
