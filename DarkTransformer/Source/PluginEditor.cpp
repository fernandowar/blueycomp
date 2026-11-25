#include "PluginEditor.h"
#include "DarkTransformerData.h"

using namespace TrColours;

namespace
{
    constexpr int kW = 800, kH = 520;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::Michroma_ttf, DarkTransformerData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::BarlowSemiBold_ttf, DarkTransformerData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::BarlowMedium_ttf, DarkTransformerData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Point<float> polar (juce::Point<float> c, float angle, float r)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }

    juce::String signedDb (double v)
    {
        return (v > 0.05 ? "+" : "") + juce::String (v, 1) + " dB";
    }

    // Medidor: -36 a +6 dBFS
    constexpr float kMeterMin = -36.0f, kMeterMax = 6.0f;
}

//==============================================================================
void TransformerLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                               float pos, float, float, juce::Slider& s)
{
    const float size = (float) juce::jmin (width, height);
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);
    const float R = size * 0.5f;
    const float ringR = R * 0.90f, thick = juce::jmax (4.0f, R * 0.075f);
    const float angle = startAngle + pos * (endAngle - startAngle);

    // Pista del arco
    juce::Path trackArc;
    trackArc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, startAngle, endAngle, true);
    g.setColour (track);
    g.strokePath (trackArc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Arco de valor (desde el centro de la escala si el parámetro es bipolar)
    const bool bipolar = s.getProperties().getWithDefault ("bipolar", false);
    const auto zeroProp = (float) s.valueToProportionOfLength (0.0);
    const float fromAngle = bipolar ? startAngle + zeroProp * (endAngle - startAngle) : startAngle;
    if (std::abs (angle - fromAngle) > 0.01f)
    {
        juce::Path valueArc;
        valueArc.addCentredArc (c.x, c.y, ringR, ringR, 0.0f, juce::jmin (fromAngle, angle), juce::jmax (fromAngle, angle), true);
        g.setColour (accent.withAlpha (0.25f));
        g.strokePath (valueArc, juce::PathStrokeType (thick * 2.2f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
        g.setColour (accent);
        g.strokePath (valueArc, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Cuerpo de la perilla
    const float kr = R * 0.70f;
    juce::Path body;
    body.addEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (kr * 0.35f), { 0, (int) (kr * 0.12f) }).drawForPath (g, body);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff353a44), c.x, c.y - kr,
                                             juce::Colour (0xff15181d), c.x, c.y + kr, false));
    g.fillPath (body);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawEllipse (c.x - kr + 1.0f, c.y - kr + 1.0f, kr * 2.0f - 2.0f, kr * 2.0f - 2.0f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - kr, c.y - kr, kr * 2.0f, kr * 2.0f, 1.0f);

    // Tapa interior con leve brillo
    const float ir = kr * 0.78f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a2e36), c.x - ir * 0.4f, c.y - ir * 0.6f,
                                             juce::Colour (0xff1a1d22), c.x + ir * 0.5f, c.y + ir * 0.8f, true));
    g.fillEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f);

    // Puntero
    const auto p1 = polar (c, angle, kr * 0.38f), p2 = polar (c, angle, kr * 0.88f);
    juce::Path ptr;
    ptr.startNewSubPath (p1);
    ptr.lineTo (p2);
    g.setColour (ink);
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (2.5f, R * 0.045f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

//==============================================================================
void TrButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();
    g.setColour (on ? accent.withAlpha (0.16f) : (over ? panel.brighter (0.12f) : panel.brighter (0.04f)));
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (on ? accent : line.brighter (0.2f));
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.3f);

    const auto led = juce::Point<float> (b.getX() + 16.0f, b.getCentreY());
    if (on)
    {
        g.setColour (accent.withAlpha (0.3f));
        g.fillEllipse (led.x - 6.0f, led.y - 6.0f, 12.0f, 12.0f);
    }
    g.setColour (on ? accent : dim);
    g.fillEllipse (led.x - 3.5f, led.y - 3.5f, 7.0f, 7.0f);

    g.setColour (on ? ink : grey);
    g.setFont (numberFont (14.0f).withExtraKerningFactor (0.08f));
    g.drawText (getButtonText(), b.withTrimmedLeft (26.0f).withTrimmedRight (8.0f), juce::Justification::centred, false);
}

//==============================================================================
DarkTransformerAudioProcessorEditor::DarkTransformerAudioProcessorEditor (DarkTransformerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      driveAttachment (p.apvts, "drive", driveKnob),
      mixAttachment (p.apvts, "mix", mixKnob),
      outputAttachment (p.apvts, "output", outputKnob),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v) { btnIn.setToggleState (v < 0.5f, juce::dontSendNotification); })
{
    auto setupKnob = [this] (juce::Slider& s, double resetValue, bool bipolar)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (TransformerLookAndFeel::startAngle, TransformerLookAndFeel::endAngle, true);
        s.setDoubleClickReturnValue (true, resetValue);
        s.getProperties().set ("bipolar", bipolar);
        s.setLookAndFeel (&laf);
        s.onValueChange = [this] { repaint(); };
        addAndMakeVisible (s);
    };
    setupKnob (driveKnob, 0.0, true);
    setupKnob (mixKnob, 100.0, false);
    setupKnob (outputKnob, 0.0, true);

    btnIn.onClick = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    addAndMakeVisible (btnIn);

    bypassAttachment.sendInitialUpdate();

    analyser.prepare (48000.0, 192000.0, 0);
    analyseHarmonics();

    setSize (kW, kH);
    startTimerHz (30);
}

DarkTransformerAudioProcessorEditor::~DarkTransformerAudioProcessorEditor()
{
    driveKnob.setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr);
    outputKnob.setLookAndFeel (nullptr);
}

void DarkTransformerAudioProcessorEditor::resized()
{
    harmBox   = { 30.0f, 100.0f, 236.0f, 290.0f };
    driveCard = { 282.0f, 100.0f, 316.0f, 290.0f };
    sideCard  = { 614.0f, 100.0f, 156.0f, 290.0f };
    meterBox  = { 30.0f, 406.0f, 740.0f, 82.0f };

    btnIn.setBounds (kW - 140, 32, 110, 34);

    driveKnob.setBounds (juce::Rectangle<float> (0, 0, 190, 190).withCentre ({ driveCard.getCentreX(), driveCard.getY() + 132.0f }).toNearestInt());
    mixKnob.setBounds (juce::Rectangle<float> (0, 0, 84, 84).withCentre ({ sideCard.getCentreX(), sideCard.getY() + 56.0f }).toNearestInt());
    outputKnob.setBounds (juce::Rectangle<float> (0, 0, 84, 84).withCentre ({ sideCard.getCentreX(), sideCard.getY() + 192.0f }).toNearestInt());
}

//==============================================================================
void DarkTransformerAudioProcessorEditor::analyseHarmonics()
{
    const float drive = processor.apvts.getRawParameterValue ("drive")->load();
    analysedDrive = drive;
    analyseFrames = 0;
    analyser.reset();

    // Seno de 1 kHz a -18 dBFS + DRIVE, procesado directo en la tasa 4x
    constexpr double os = 192000.0, f = 1000.0;
    constexpr int period = 192, total = period * 150, measure = period * 50;
    const float amp = std::pow (10.0f, (-18.0f + drive) / 20.0f);
    std::vector<float> y ((size_t) measure);
    for (int i = 0; i < total; ++i)
    {
        const float out = analyser.processOversampled (amp * (float) std::sin (juce::MathConstants<double>::twoPi * f * i / os));
        if (i >= total - measure)
            y[(size_t) (i - (total - measure))] = out;
    }

    auto mag = [&y] (int h)
    {
        double re = 0.0, im = 0.0;
        for (size_t i = 0; i < y.size(); ++i)
        {
            const double ph = juce::MathConstants<double>::twoPi * (double) h * (double) i / (double) period;
            re += y[i] * std::cos (ph);
            im += y[i] * std::sin (ph);
        }
        return std::sqrt (re * re + im * im);
    };

    const double fund = juce::jmax (1.0e-12, mag (1));
    for (int h = 2; h <= 7; ++h)
        harmonicsDb[(size_t) h] = (float) juce::Decibels::gainToDecibels (mag (h) / fund, -120.0);
}

void DarkTransformerAudioProcessorEditor::timerCallback()
{
    const float drive = processor.apvts.getRawParameterValue ("drive")->load();
    // también se vuelve a medir cada ~0.4 s: la deriva analógica mueve los armónicos
    if (std::abs (drive - analysedDrive) > 0.05f || ++analyseFrames >= 12)
    {
        analyseHarmonics();
        repaint (harmBox.expanded (2.0f).toNearestInt());
    }

    const float target = processor.ironLevelDb.load();
    level += (target > level ? 0.6f : 0.15f) * (target - level);
    if (target >= peakHold) { peakHold = target; peakHoldFrames = 45; }
    else if (--peakHoldFrames <= 0) peakHold = juce::jmax (target, peakHold - 0.6f);
    repaint (meterBox.expanded (2.0f).toNearestInt());
}

//==============================================================================
void DarkTransformerAudioProcessorEditor::drawCard (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title)
{
    g.setColour (panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    if (title.isNotEmpty())
    {
        g.setColour (grey);
        g.setFont (labelFont (11.0f).withExtraKerningFactor (0.3f));
        g.drawText (title, r.withHeight (32.0f).reduced (16.0f, 0.0f), juce::Justification::centredLeft, false);
    }
}

void DarkTransformerAudioProcessorEditor::drawReadout (juce::Graphics& g, juce::Point<float> c, const juce::String& value,
                                                       const juce::String& name, float size)
{
    g.setColour (accent);
    g.setFont (numberFont (size));
    g.drawText (value, juce::Rectangle<float> (160.0f, size + 4.0f).withCentre (c), juce::Justification::centred, false);
    g.setColour (ink);
    g.setFont (titleFont (12.0f).withExtraKerningFactor (0.15f));
    g.drawText (name, juce::Rectangle<float> (160.0f, 18.0f).withCentre ({ c.x, c.y + size * 0.5f + 14.0f }), juce::Justification::centred, false);
}

void DarkTransformerAudioProcessorEditor::drawHarmonics (juce::Graphics& g)
{
    const auto b = harmBox;
    drawCard (g, b, juce::String (juce::CharPointer_UTF8 ("ARM\xc3\x93NICOS")));

    g.setColour (dim);
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.2f));
    g.drawText ("1 kHz  /  -18 dBFS + DRIVE", juce::Rectangle<float> (b.getX() + 16.0f, b.getY() + 26.0f, b.getWidth() - 32.0f, 14.0f),
                juce::Justification::centredLeft, false);

    // Pantalla con barras H2..H7: de -90 a -10 dBc. Pares en ámbar, impares en crema.
    const auto scr = juce::Rectangle<float> (b.getX() + 14.0f, b.getY() + 48.0f, b.getWidth() - 28.0f, b.getHeight() - 62.0f);
    g.setColour (screen);
    g.fillRoundedRectangle (scr, 6.0f);

    const auto area = scr.reduced (12.0f, 0.0f).withTrimmedTop (30.0f).withTrimmedBottom (26.0f);
    g.setColour (line);
    for (int db : { -80, -60, -40, -20 })
    {
        const float y = area.getBottom() - area.getHeight() * ((float) db + 90.0f) / 80.0f;
        g.drawHorizontalLine ((int) y, area.getX(), area.getRight());
    }

    const float slot = area.getWidth() / 6.0f;
    for (int h = 2; h <= 7; ++h)
    {
        const float db = harmonicsDb[(size_t) h];
        const float prop = juce::jlimit (0.0f, 1.0f, (db + 90.0f) / 80.0f);
        const float x = area.getX() + slot * (float) (h - 2);
        const auto barArea = juce::Rectangle<float> (x + slot * 0.2f, area.getY(), slot * 0.6f, area.getHeight());
        const auto col = (h % 2 == 0) ? accent : cream;
        const auto bar = barArea.withTop (barArea.getBottom() - barArea.getHeight() * prop);
        g.setColour (col.withAlpha (0.18f));
        g.fillRoundedRectangle (bar.expanded (2.0f, 0.0f), 3.0f);
        g.setColour (col);
        g.fillRoundedRectangle (bar, 2.0f);

        g.setColour (prop > 0.0f ? ink.withAlpha (0.9f) : dim);
        g.setFont (numberFont (11.0f));
        g.drawText (prop > 0.0f ? juce::String (juce::roundToInt (db)) : juce::String ("--"),
                    juce::Rectangle<float> (x, bar.getY() - 18.0f, slot, 14.0f), juce::Justification::centred, false);
        g.setColour (grey);
        g.drawText ("H" + juce::String (h), juce::Rectangle<float> (x, area.getBottom() + 6.0f, slot, 14.0f),
                    juce::Justification::centred, false);
    }
}

void DarkTransformerAudioProcessorEditor::drawMeter (juce::Graphics& g)
{
    const auto b = meterBox;
    g.setColour (screen);
    g.fillRoundedRectangle (b, 6.0f);
    g.setColour (line);
    g.drawRoundedRectangle (b.reduced (0.5f), 6.0f, 1.0f);

    g.setColour (grey);
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.3f));
    g.drawText ("NIVEL AL HIERRO  (dBFS)", b.withHeight (22.0f).reduced (12.0f, 0.0f), juce::Justification::centredLeft, false);

    // Lectura del pico retenido
    const bool hasSignal = peakHold > -60.0f;
    g.setColour (peakHold > 0.0f ? juce::Colour (0xffff4a3a) : (hasSignal ? accent : dim));
    g.setFont (numberFont (18.0f));
    g.drawText (hasSignal ? juce::String (peakHold, 1) : juce::String ("--"),
                juce::Rectangle<float> (b.getRight() - 72.0f, b.getY() + 4.0f, 60.0f, 20.0f), juce::Justification::centredRight, false);

    // Barra segmentada
    const auto bar = juce::Rectangle<float> (b.getX() + 14.0f, b.getY() + 32.0f, b.getWidth() - 28.0f, 16.0f);
    constexpr int segments = 72;
    const float segW = bar.getWidth() / (float) segments;
    for (int i = 0; i < segments; ++i)
    {
        const float segDb = kMeterMin + (kMeterMax - kMeterMin) * ((float) i + 0.5f) / (float) segments;
        juce::Colour col = segDb > 0.0f ? juce::Colour (0xffff4a3a) : (segDb > -6.0f ? accent : juce::Colour (0xff3ddc6a));
        const bool lit = level >= segDb && level > -90.0f;
        g.setColour (lit ? col : col.withAlpha (0.12f));
        g.fillRoundedRectangle (bar.getX() + segW * (float) i + 1.0f, bar.getY(), segW - 2.0f, bar.getHeight(), 1.5f);
    }
    if (peakHold > kMeterMin)
    {
        const float px = bar.getX() + bar.getWidth() * (juce::jmin (peakHold, kMeterMax) - kMeterMin) / (kMeterMax - kMeterMin);
        g.setColour (ink);
        g.fillRect (px - 1.0f, bar.getY() - 2.0f, 2.0f, bar.getHeight() + 4.0f);
    }

    // Escala
    g.setFont (numberFont (11.0f));
    for (int db : { -36, -24, -18, -12, -6, 0, 6 })
    {
        const float x = bar.getX() + bar.getWidth() * ((float) db - kMeterMin) / (kMeterMax - kMeterMin);
        g.setColour (db == 0 ? ink : grey);
        g.drawText (db > 0 ? "+" + juce::String (db) : juce::String (db),
                    juce::Rectangle<float> (30.0f, 14.0f).withCentre ({ x, bar.getBottom() + 14.0f }), juce::Justification::centred, false);
    }
}

void DarkTransformerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBot, 0.0f, h, false));
    g.fillAll();

    // Encabezado
    g.setColour (ink);
    g.setFont (titleFont (26.0f).withExtraKerningFactor (0.08f));
    g.drawText ("DARK TRANSFORMER", 30, 28, 520, 36, juce::Justification::centredLeft);
    g.setColour (accent);
    g.fillRoundedRectangle (32.0f, 70.0f, 46.0f, 3.0f, 1.5f);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.25f));
    g.drawText ("TRANSFORMER & PREAMP SATURATION", 88, 63, 400, 16, juce::Justification::centredLeft);

    // Tarjetas
    drawCard (g, driveCard, {});
    drawCard (g, sideCard, {});

    // Lecturas de las perillas
    drawReadout (g, { driveCard.getCentreX(), driveCard.getY() + 246.0f }, signedDb (driveKnob.getValue()), "DRIVE", 22.0f);
    drawReadout (g, { sideCard.getCentreX(), sideCard.getY() + 110.0f }, juce::String (juce::roundToInt (mixKnob.getValue())) + " %", "MIX", 16.0f);
    drawReadout (g, { sideCard.getCentreX(), sideCard.getY() + 248.0f }, signedDb (outputKnob.getValue()), "OUTPUT", 16.0f);

    // Escala del DRIVE
    {
        const auto c = juce::Point<float> (driveCard.getCentreX(), driveCard.getY() + 132.0f);
        g.setFont (numberFont (12.0f));
        g.setColour (grey);
        const std::pair<float, const char*> marks[] = { { -12.0f, "-12" }, { 0.0f, "0" }, { 12.0f, "+12" }, { 24.0f, "+24" } };
        for (const auto& m : marks)
        {
            const float prop = (float) driveKnob.valueToProportionOfLength (m.first);
            const float a = TransformerLookAndFeel::startAngle + prop * (TransformerLookAndFeel::endAngle - TransformerLookAndFeel::startAngle);
            g.drawText (m.second, juce::Rectangle<float> (34.0f, 14.0f).withCentre (polar (c, a, 112.0f)), juce::Justification::centred, false);
        }
    }

    drawHarmonics (g);
    drawMeter (g);

    g.setColour (dim);
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.3f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (30.0f, h - 26.0f, 240.0f, 14.0f), juce::Justification::centredLeft, false);
    g.drawText ("4X OVERSAMPLING", juce::Rectangle<float> (w - 270.0f, h - 26.0f, 240.0f, 14.0f), juce::Justification::centredRight, false);
}
