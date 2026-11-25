#include "IronEngine.h"

namespace iron
{
    juce::StringArray voiceNames()
    {
        return { "NICKEL", "STEEL", "VINTAGE", "BRIT A", "US DISC", "TUBE" };
    }

    const VoiceParams& paramsFor (int v)
    {
        //  lowCut core  amt   drive bias  stage sDrv  sBias outC  resHz    resQ  | lowShelf    | bell              | highShelf
        static const VoiceParams table[numVoices] = {
            // NICKEL: abierto y rápido, el núcleo satura poco y tarde; leve brillo arriba
            {  6.0f,  55.0f, 0.55f, 1.2f, 0.05f, 0, 1.0f, 0.0f,  0.0f, 52000.0f, 0.95f,  80.0f, 0.0f,  1000.0f, 0.0f, 0.7f, 15000.0f,  0.4f },
            // STEEL: mucho flujo en graves, satura temprano, agudos redondeados
            { 12.0f, 140.0f, 1.00f, 2.2f, 0.12f, 0, 1.0f, 0.0f,  0.0f, 26000.0f, 0.62f,  90.0f, 0.9f,  1000.0f, 0.0f, 0.7f, 13000.0f, -0.7f },
            // VINTAGE: pares temprano (asimetría) y luego subida de 3er armónico
            { 10.0f, 100.0f, 0.90f, 1.7f, 0.20f, 1, 0.4f, 0.35f, 0.0f, 22000.0f, 0.75f,  90.0f, 0.0f,   180.0f, 0.7f, 0.7f, 12000.0f, -1.0f },
            // BRIT A: trafo de entrada + clase A + trafo de salida; graves y aire
            {  9.0f,  80.0f, 0.70f, 1.4f, 0.10f, 1, 0.5f, 0.30f, 0.8f, 40000.0f, 1.10f,  60.0f, 0.7f,  1000.0f, 0.0f, 0.7f, 10000.0f,  0.8f },
            // US DISC: op-amp discreto con codo más duro, medios al frente
            {  8.0f,  70.0f, 0.50f, 1.2f, 0.06f, 2, 1.4f, 0.05f, 0.6f, 45000.0f, 0.80f, 100.0f, 0.4f,  1800.0f, 1.0f, 0.6f, 12000.0f,  0.0f },
            // TUBE: triodo asimétrico con corriente de reja (compresión suave)
            { 10.0f,  90.0f, 0.60f, 1.3f, 0.10f, 3, 0.6f, 0.35f, 0.9f, 24000.0f, 0.70f, 120.0f, 0.5f,  1000.0f, 0.0f, 0.7f, 11000.0f, -0.8f },
        };
        return table[juce::jlimit (0, numVoices - 1, v)];
    }

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

        // Codo más duro (op-amp discreto): x / (1 + |kx|^p)^(1/p)
        inline float hardKnee (float x, float k) noexcept
        {
            constexpr float p = 2.5f;
            return x / std::pow (1.0f + std::pow (std::abs (k * x), p), 1.0f / p);
        }
    }

    void Channel::prepare (double base, double os)
    {
        baseRate = base;
        osRate = os;
        setVoice (voice);
        reset();
    }

    void Channel::setVoice (int v)
    {
        voice = juce::jlimit (0, numVoices - 1, v);
        p = paramsFor (voice);

        inHp.setLowpass (baseRate, p.lowCutHz);
        coreLp.setLowpass (osRate, p.coreHz);
        outCoreLp.setLowpass (osRate, p.coreHz * 1.2);
        envelope.setLowpass (osRate, 6.0);
        dcBlock.setLowpass (osRate, 4.0);
        res.setLowpass (osRate, juce::jmin ((double) p.resHz, 0.42 * osRate), p.resQ);

        lowShelf.setLowShelf (baseRate, p.lowShelfHz, p.lowShelfDb);
        bell.setBell (baseRate, p.bellHz, p.bellQ, p.bellDb);
        highShelf.setHighShelf (baseRate, juce::jmin ((double) p.highShelfHz, 0.4 * baseRate), p.highShelfDb);
    }

    void Channel::reset()
    {
        inHp.reset(); coreLp.reset(); outCoreLp.reset(); envelope.reset(); dcBlock.reset();
        res.reset(); lowShelf.reset(); bell.reset(); highShelf.reset();
    }

    float Channel::processOversampled (float x) noexcept
    {
        // 1) Núcleo de entrada: el flujo magnético crece en graves, así que se
        //    satura la parte baja del espectro; encima, una saturación general leve.
        {
            const float low = coreLp.lp (x);
            const float lowSat = biasedTanh (low, p.coreDrive, p.coreBias);
            x += p.coreAmount * (lowSat - low);
            x = biasedTanh (x, p.coreDrive * 0.35f, p.coreBias * 0.5f);
        }

        // 2) Etapa de ganancia del preamp
        switch (p.stage)
        {
            case 1: // clase A (un transistor, asimétrica: 2º armónico)
                x = biasedTanh (x, p.stageDrive, p.stageBias);
                break;

            case 2: // op-amp discreto: simétrico con codo más duro + leve sesgo
            {
                const float b = p.stageBias;
                x = (hardKnee (x + b, p.stageDrive) - hardKnee (b, p.stageDrive));
                break;
            }

            case 3: // válvula con corriente de reja: la señal fuerte corre el punto de trabajo
            {
                // Triodo: curva muy sesgada (2º armónico dominante); la señal
                // fuerte corre el punto de trabajo y comprime con suavidad
                const float shift = p.stageBias * envelope.lp (std::abs (x));
                constexpr float triodeBias = 0.45f;
                x = biasedTanh (x - shift, p.stageDrive, triodeBias) - biasedTanh (-shift, p.stageDrive, triodeBias);
                break;
            }

            default: break;
        }

        // 3) Transformador de salida
        if (p.outCoreAmount > 0.0f)
        {
            const float low = outCoreLp.lp (x);
            const float lowSat = biasedTanh (low, p.coreDrive * 0.8f, p.coreBias);
            x += p.outCoreAmount * (lowSat - low);
        }

        // 4) Resonancia de agudos (inductancia de fuga + capacitancia) y DC
        x = res.process (x);
        return dcBlock.hp (x);
    }
}
