#include "PluginProcessor.h"
#include "PluginEditor.h"

constexpr float Dark4kAudioProcessor::kAttackMs[6];

namespace
{
    // Rango logarítmico para frecuencias
    juce::NormalisableRange<float> logRange (float lo, float hi, float step = 0.0f)
    {
        juce::NormalisableRange<float> r (lo, hi, step);
        r.setSkewForCentre (std::sqrt (lo * hi));
        return r;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout Dark4kAudioProcessor::createLayout()
{
    using F = juce::AudioParameterFloat;
    using B = juce::AudioParameterBool;
    using C = juce::AudioParameterChoice;
    using Attr = juce::AudioParameterFloatAttributes;
    juce::AudioProcessorValueTreeState::ParameterLayout l;

    auto db = [&] (const char* id, const char* name, float lo, float hi, float def)
    { l.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (lo, hi, 0.1f), def, Attr().withLabel ("dB"))); };
    auto hz = [&] (const char* id, const char* name, float lo, float hi, float def)
    { l.add (std::make_unique<F> (juce::ParameterID { id, 1 }, name, logRange (lo, hi), def, Attr().withLabel ("Hz"))); };
    auto sw = [&] (const char* id, const char* name, bool def)
    { l.add (std::make_unique<B> (juce::ParameterID { id, 1 }, name, def)); };

    db ("intrim", "Input Trim", -20.0f, 20.0f, 0.0f);
    sw ("phase", "Phase", false);

    // Fader de salida: -80 (-inf) a +12 dB, curva con más resolución cerca de 0 dB
    {
        juce::NormalisableRange<float> r (-80.0f, 12.0f, 0.1f);
        r.setSkewForCentre (-18.0f);
        l.add (std::make_unique<F> (juce::ParameterID { "level", 1 }, "Fader", r, 0.0f, Attr().withLabel ("dB")));
    }
    sw ("bypass", "Bypass", false);

    sw ("eqin", "EQ In", true);
    sw ("eqblack", "EQ Black", false);
    sw ("filtin", "Filters In", true);
    hz ("hpf", "High Pass", 16.0f, 350.0f, 16.0f);
    hz ("lpf", "Low Pass", 3000.0f, 22000.0f, 22000.0f);

    db ("hfg", "HF Gain", -15.0f, 15.0f, 0.0f);
    hz ("hff", "HF Freq", 1500.0f, 16000.0f, 8000.0f);
    sw ("hfbell", "HF Bell", false);
    db ("hmfg", "HMF Gain", -15.0f, 15.0f, 0.0f);
    hz ("hmff", "HMF Freq", 600.0f, 7000.0f, 3000.0f);
    l.add (std::make_unique<F> (juce::ParameterID { "hmfq", 1 }, "HMF Q", logRange (0.5f, 3.0f), 1.2f));
    db ("lmfg", "LMF Gain", -15.0f, 15.0f, 0.0f);
    hz ("lmff", "LMF Freq", 200.0f, 2500.0f, 800.0f);
    l.add (std::make_unique<F> (juce::ParameterID { "lmfq", 1 }, "LMF Q", logRange (0.5f, 3.0f), 1.2f));
    db ("lfg", "LF Gain", -15.0f, 15.0f, 0.0f);
    hz ("lff", "LF Freq", 30.0f, 450.0f, 100.0f);
    sw ("lfbell", "LF Bell", false);

    sw ("dynin", "Dyn In", true);

    // Threshold estilo SSL: la perilla va de +10 dB (izquierda) a -20 dB (derecha), con rango extendido a -30
    juce::NormalisableRange<float> thr (
        -30.0f, 10.0f,
        [] (float s, float e, float pr) { return e - pr * (e - s); },
        [] (float s, float e, float v) { return (e - v) / (e - s); },
        [] (float s, float e, float v) { return juce::jlimit (s, e, std::round (v * 10.0f) / 10.0f); });
    l.add (std::make_unique<F> (juce::ParameterID { "threshold", 1 }, "Threshold", thr, 10.0f, Attr().withLabel ("dB")));
    l.add (std::make_unique<C> (juce::ParameterID { "attack", 1 }, "Attack",
                                juce::StringArray { "0.1 ms", "0.3 ms", "1 ms", "3 ms", "10 ms", "30 ms" }, 3));
    l.add (std::make_unique<C> (juce::ParameterID { "release", 1 }, "Release", juce::StringArray { "100 ms", "Auto" }, 0));
    db ("makeup", "Makeup", 0.0f, 15.0f, 0.0f);
    db ("gatethr", "Gate Threshold", -70.0f, 0.0f, -70.0f);
    db ("gaterange", "Gate Range", 0.0f, 40.0f, 0.0f);
    return l;
}

Dark4kAudioProcessor::Dark4kAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    const char* ids[pCount] = { "intrim", "phase", "level", "bypass", "eqin", "eqblack", "filtin", "hpf", "lpf",
                                "hfg", "hff", "hfbell", "hmfg", "hmff", "hmfq", "lmfg", "lmff", "lmfq", "lfg", "lff", "lfbell",
                                "dynin", "threshold", "attack", "release", "makeup", "gatethr", "gaterange" };
    for (int i = 0; i < pCount; ++i) prm[i] = apvts.getRawParameterValue (ids[i]);

    // Cada instancia nueva nace como un canal distinto de la consola
    setChannel (1 + juce::Random::getSystemRandom().nextInt (999));
}

void Dark4kAudioProcessor::setChannel (int ch)
{
    ch = juce::jlimit (1, 999, ch);
    channel.store (ch);
    apvts.state.setProperty ("channel", ch, nullptr);

    // Tolerancias del canal (pequeñas, como componentes reales de una consola)
    juce::Random r ((juce::int64) ch * 7919 + 13);
    auto u = [&r] (float amt) { return (r.nextFloat() * 2.0f - 1.0f) * amt; };
    for (int i = 0; i < 4; ++i) { var.eqGain[i] = u (0.20f); var.eqFreq[i] = 1.0f + u (0.025f); }
    var.hpf = 1.0f + u (0.035f);
    var.lpf = 1.0f + u (0.035f);
    var.thrDb = u (0.5f);
    var.atk = 1.0f + u (0.08f);
    var.makeupDb = u (0.15f);
    var.satBias = u (0.003f);
    var.trimDb = u (0.12f);
    var.lrGain = u (0.004f);
    var.outDb = u (0.1f);
    varDirty.store (true);
}

void Dark4kAudioProcessor::rerollChannel()
{
    int n;
    do { n = 1 + juce::Random::getSystemRandom().nextInt (999); } while (n == channel.load());
    setChannel (n);
}

bool Dark4kAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void Dark4kAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    const auto coeffFor = [sr] (float ms) { return 1.0f - std::exp (-1.0f / (0.001f * ms * (float) sr)); };
    relFixedCoeff = coeffFor (kRelFixedMs);
    relFastCoeff  = coeffFor (kRelFastMs);
    relSlowCoeff  = coeffFor (kRelSlowMs);
    atkSlowCoeff  = coeffFor (kAtkSlowMs);
    envFast = envSlow = 0.0f;
    dcX[0] = dcX[1] = dcY[0] = dcY[1] = 0.0f;
    dcCoeff = 1.0f - juce::MathConstants<float>::twoPi * 15.0f / (float) sr;

    const auto onePole = [sr] (float f) { return 1.0f - std::exp (-juce::MathConstants<float>::twoPi * f / (float) sr); };
    exCoeff = onePole (2500.0f);
    sideLoCoeff = onePole (150.0f);
    sideHiCoeff = onePole (1500.0f);
    exLp[0] = exLp[1] = sideLo = sideHi = 0.0f;
    gateLevel = 0.0f; gateGainDb = 0.0f; gateHold = 0.0f;
    for (auto* b : { &hpf1, &hpf2, &lpf, &hf, &hmf, &lmf, &lf }) b->reset();

    const double ramp = 0.02;
    thresholdDb.reset (sr, ramp);  thresholdDb.setCurrentAndTargetValue (prm[pThr]->load());
    makeupGain.reset (sr, 0.03);   makeupGain.setCurrentAndTargetValue (1.0f);
    inGain.reset (sr, ramp);       inGain.setCurrentAndTargetValue (1.0f);
    outGain.reset (sr, ramp);      outGain.setCurrentAndTargetValue (1.0f);
    wetMix.reset (sr, ramp);       wetMix.setCurrentAndTargetValue (prm[pBypass]->load() > 0.5f ? 0.0f : 1.0f);
    gateThrDb.reset (sr, ramp);    gateThrDb.setCurrentAndTargetValue (prm[pGateThr]->load());
    gateRangeDb.reset (sr, ramp);  gateRangeDb.setCurrentAndTargetValue (prm[pGateRange]->load());
    eqMix.reset (sr, 0.01);        eqMix.setCurrentAndTargetValue (prm[pEqIn]->load() > 0.5f ? 1.0f : 0.0f);
    dynMix.reset (sr, 0.01);       dynMix.setCurrentAndTargetValue (prm[pDynIn]->load() > 0.5f ? 1.0f : 0.0f);
    varDirty.store (true);
}

void Dark4kAudioProcessor::updateFilters()
{
    const double sr = sampleRate;
    const Variation v = var;
    d4k::EqVoicing vo;
    vo.black = prm[pEqBlack]->load() > 0.5f;

    // Filtros (18 dB/oct pasa-altos, 12 dB/oct pasa-bajos). Fuera de rango = apagados.
    const float hpfHz = prm[pHpf]->load() * v.hpf;
    hpfOn = prm[pHpf]->load() > 16.5f;
    if (hpfOn) { hpf1.highPass1 (sr, hpfHz); hpf2.highPass2 (sr, hpfHz, 1.0); }
    const float lpfHz = prm[pLpf]->load() * v.lpf;
    lpfOn = prm[pLpf]->load() < 21900.0f;
    if (lpfOn) lpf.lowPass2 (sr, lpfHz, 0.707);

    const float gHf = prm[pHfG]->load() + (prm[pHfG]->load() != 0.0f ? v.eqGain[0] : 0.0f);
    const float gHm = prm[pHmfG]->load() + (prm[pHmfG]->load() != 0.0f ? v.eqGain[1] : 0.0f);
    const float gLm = prm[pLmfG]->load() + (prm[pLmfG]->load() != 0.0f ? v.eqGain[2] : 0.0f);
    float gLf = prm[pLfG]->load() + (prm[pLfG]->load() != 0.0f ? v.eqGain[3] : 0.0f);
    if (vo.black && gLf > 0.0f) gLf += 0.04f * gLf; // Black: algo más de peso en graves

    hfOn = std::abs (prm[pHfG]->load()) > 0.05f;
    hmfOn = std::abs (prm[pHmfG]->load()) > 0.05f;
    lmfOn = std::abs (prm[pLmfG]->load()) > 0.05f;
    lfOn = std::abs (prm[pLfG]->load()) > 0.05f;

    if (hfOn)
    {
        const double f = prm[pHfF]->load() * v.eqFreq[0];
        if (prm[pHfBell]->load() > 0.5f) hf.peak (sr, f, gHf, vo.bellQ (1.0, gHf));
        else                              hf.highShelf (sr, f, gHf, vo.shelfQ());
    }
    if (hmfOn) hmf.peak (sr, prm[pHmfF]->load() * v.eqFreq[1], gHm, vo.bellQ (prm[pHmfQ]->load(), gHm));
    if (lmfOn) lmf.peak (sr, prm[pLmfF]->load() * v.eqFreq[2], gLm, vo.bellQ (prm[pLmfQ]->load(), gLm));
    if (lfOn)
    {
        const double f = prm[pLfF]->load() * v.eqFreq[3];
        if (prm[pLfBell]->load() > 0.5f) lf.peak (sr, f, gLf, vo.bellQ (1.0, gLf));
        else                              lf.lowShelf (sr, f, gLf, vo.shelfQ());
    }
}

void Dark4kAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int numSamples = buffer.getNumSamples();
    if (numCh == 0 || numSamples == 0) return;

    updateFilters();
    varDirty.store (false);
    const Variation v = var;

    const int atkIdx = juce::jlimit (0, 5, (int) std::round (prm[pAtk]->load()));
    const bool autoRelease = prm[pRel]->load() > 0.5f;
    const float attackCoeff = 1.0f - std::exp (-1.0f / (0.001f * kAttackMs[atkIdx] * v.atk * (float) sampleRate));
    const bool filtersOn = prm[pFiltIn]->load() > 0.5f;
    const bool black = prm[pEqBlack]->load() > 0.5f;
    const float phaseSign = prm[pPhase]->load() > 0.5f ? -1.0f : 1.0f;

    inGain.setTargetValue (juce::Decibels::decibelsToGain (prm[pInTrim]->load() + v.trimDb));
    const float lvl = prm[pLevel]->load();
    outGain.setTargetValue (lvl <= -79.5f ? 0.0f : juce::Decibels::decibelsToGain (lvl + v.outDb));
    thresholdDb.setTargetValue (prm[pThr]->load() + v.thrDb);
    makeupGain.setTargetValue (juce::Decibels::decibelsToGain (prm[pMakeup]->load() + v.makeupDb));
    wetMix.setTargetValue (prm[pBypass]->load() > 0.5f ? 0.0f : 1.0f);
    gateThrDb.setTargetValue (prm[pGateThr]->load());
    gateRangeDb.setTargetValue (prm[pGateRange]->load());
    eqMix.setTargetValue (prm[pEqIn]->load() > 0.5f ? 1.0f : 0.0f);
    dynMix.setTargetValue (prm[pDynIn]->load() > 0.5f ? 1.0f : 0.0f);

    const float slope = 1.0f - 1.0f / kRatio;
    const float gateAtk = 1.0f - std::exp (-1.0f / (0.0004f * (float) sampleRate));
    const float gateRel = 1.0f - std::exp (-1.0f / (0.18f * (float) sampleRate));
    const float lvlDecay = std::exp (-1.0f / (0.005f * (float) sampleRate));
    const float holdSamples = 0.02f * (float) sampleRate;

    float peakGr = 0.0f, inPk[2] { 0, 0 }, outPk[2] { 0, 0 };
    float* data[2] = { buffer.getWritePointer (0), numCh > 1 ? buffer.getWritePointer (1) : nullptr };

    for (int i = 0; i < numSamples; ++i)
    {
        const float thr = thresholdDb.getNextValue();
        const float mk = makeupGain.getNextValue();
        const float ig = inGain.getNextValue();
        const float og = outGain.getNextValue();
        const float m = wetMix.getNextValue();
        const float gThr = gateThrDb.getNextValue();
        const float gRng = gateRangeDb.getNextValue();
        const float eqM = eqMix.getNextValue();
        const float dynM = dynMix.getNextValue();

        float dry[2] { 0, 0 }, w[2] { 0, 0 };
        for (int ch = 0; ch < numCh; ++ch)
        {
            dry[ch] = data[ch][i];
            w[ch] = dry[ch] * ig * phaseSign;
            inPk[ch] = juce::jmax (inPk[ch], std::abs (w[ch]));
        }

        // ---- Filtros y EQ ----
        for (int ch = 0; ch < numCh; ++ch)
        {
            float x = w[ch];
            if (filtersOn)
            {
                if (hpfOn) x = hpf2.process (ch, hpf1.process (ch, x));
                if (lpfOn) x = lpf.process (ch, x);
            }
            if (eqM > 0.0f)
            {
                float y = x;
                if (lfOn)  y = lf.process (ch, y);
                if (lmfOn) y = lmf.process (ch, y);
                if (hmfOn) y = hmf.process (ch, y);
                if (hfOn)  y = hf.process (ch, y);
                // Brown: un poco de "grit" (tanh muy suave) en el EQ; Black más limpio
                if (! black)
                {
                    const float k = 0.35f;
                    y = std::tanh (k * y) / k;
                }
                x = x + eqM * (y - x);
            }
            w[ch] = x;
        }

        // ---- Dinámica: puerta básica + Dark VCA ----
        if (dynM > 0.0f)
        {
            float dyn[2] { w[0], w[1] };

            // Puerta (downward expander básico, enlazada)
            float pk = 0.0f;
            for (int ch = 0; ch < numCh; ++ch) pk = juce::jmax (pk, std::abs (dyn[ch]));
            gateLevel = juce::jmax (pk, gateLevel * lvlDecay);
            if (gRng > 0.05f)
            {
                const float lvDb = juce::Decibels::gainToDecibels (gateLevel, -120.0f);
                const bool open = lvDb > gThr;
                if (open) gateHold = holdSamples; else if (gateHold > 0.0f) gateHold -= 1.0f;
                const float target = (open || gateHold > 0.0f) ? 0.0f : -gRng;
                gateGainDb += (target > gateGainDb ? gateAtk : gateRel) * (target - gateGainDb);
                const float gg = juce::Decibels::decibelsToGain (gateGainDb);
                for (int ch = 0; ch < numCh; ++ch) dyn[ch] *= gg;
            }

            // Detector enlazado (stereo link)
            float peak = 0.0f;
            for (int ch = 0; ch < numCh; ++ch) peak = juce::jmax (peak, std::abs (dyn[ch]));
            const float over = juce::Decibels::gainToDecibels (peak, -120.0f) - thr;

            float targetGr = 0.0f;
            if (2.0f * over > kKneeDb)       targetGr = slope * over;
            else if (2.0f * over > -kKneeDb) { const float x = over + kKneeDb * 0.5f; targetGr = slope * x * x / (2.0f * kKneeDb); }

            float gr;
            if (autoRelease)
            {
                envFast += (targetGr > envFast ? attackCoeff : relFastCoeff) * (targetGr - envFast);
                envSlow += (targetGr > envSlow ? atkSlowCoeff : relSlowCoeff) * (targetGr - envSlow);
                gr = juce::jmax (envFast, envSlow);
            }
            else
            {
                envFast += (targetGr > envFast ? attackCoeff : relFixedCoeff) * (targetGr - envFast);
                envSlow = envFast;
                gr = envFast;
            }
            peakGr = juce::jmax (peakGr, gr);

            // Saturación del Dark VCA. "Dinámica": el drive sube un poco con la compresión (el VCA
            // trabajando = más color), y cada canal tiene su propia asimetría/tolerancia.
            const float gain = juce::Decibels::decibelsToGain (-gr) * mk;
            const float drive = kSatDrive * (1.0f + 0.012f * gr);
            const float biasCh[2] = { 0.004f + v.satBias, 0.006f + v.satBias };
            const float gainCh[2] = { 1.0f, 0.994f + v.lrGain };

            float wet[2] { 0, 0 };
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float ww = dyn[ch] * gain * gainCh[ch];
                const float bias = biasCh[ch], tb = std::tanh (bias);
                const float shaped = (std::tanh (drive * ww + bias) - tb) / (drive * (1.0f - tb * tb));
                const float y = shaped - dcX[ch] + dcCoeff * dcY[ch];
                dcX[ch] = shaped; dcY[ch] = y;
                wet[ch] = y;
            }

            // DEPTH fijo: frontalidad + tridimensionalidad
            for (int ch = 0; ch < numCh; ++ch)
            {
                exLp[ch] += exCoeff * (wet[ch] - exLp[ch]);
                const float hp = wet[ch] - exLp[ch];
                const float harm = std::tanh (4.0f * hp) * 0.25f - hp;
                wet[ch] += kDepth * (0.3f * hp + 0.6f * harm);
            }
            if (numCh == 2)
            {
                const float mid = 0.5f * (wet[0] + wet[1]);
                float side = 0.5f * (wet[0] - wet[1]);
                sideLo += sideLoCoeff * (side - sideLo);
                sideHi += sideHiCoeff * (side - sideHi);
                side += kDepth * (0.45f * (side - sideHi) - 0.6f * sideLo);
                wet[0] = mid + side; wet[1] = mid - side;
            }
            for (int ch = 0; ch < numCh; ++ch) w[ch] = w[ch] + dynM * (wet[ch] - w[ch]);
        }

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float o = w[ch] * og;
            outPk[ch] = juce::jmax (outPk[ch], std::abs (o));
            data[ch][i] = dry[ch] + m * (o - dry[ch]);
        }
    }

    for (int ch = 0; ch < 2; ++ch)
    {
        auto bump = [] (std::atomic<float>& a, float val) { float c = a.load(); while (val > c && ! a.compare_exchange_weak (c, val)) {} };
        bump (inPeak[ch], inPk[ch]);
        bump (outPeak[ch], outPk[ch]);
    }
    gainReductionDb.store (wetMix.getCurrentValue() > 0.5f && dynMix.getCurrentValue() > 0.5f ? peakGr : 0.0f);
}

juce::AudioProcessorEditor* Dark4kAudioProcessor::createEditor()
{
    return new Dark4kAudioProcessorEditor (*this);
}

void Dark4kAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void Dark4kAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
            const int ch = (int) apvts.state.getProperty ("channel", channel.load());
            setChannel (ch);
        }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Dark4kAudioProcessor();
}
