#include "PluginEditor.h"
#include "DarkLevelFonts.h"

using namespace SslColours;

namespace
{
    constexpr int kW = 740, kH = 480;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    // Michroma: títulos anchos tipo Eurostile. Barlow: números y botones.
    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLevelFontData::Michroma_ttf, DarkLevelFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLevelFontData::BarlowSemiBold_ttf, DarkLevelFontData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLevelFontData::BarlowMedium_ttf, DarkLevelFontData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Point<float> polar (juce::Point<float> c, float angle, float r)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }
}

//==============================================================================
void SslLookAndFeel::paintKnob (juce::Graphics& g, juce::Point<float> c, float R, float proportion)
{
    const float pi = juce::MathConstants<float>::pi;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    // Sombra proyectada
    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), (int) (R * 0.5f),
                      { 0, (int) (R * 0.16f) }).drawForPath (g, body);

    // Falda oscura con estrías
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3c42), c.x - R * 0.6f, c.y - R * 0.8f,
                                             juce::Colour (0xff0b0b0e), c.x + R * 0.7f, c.y + R * 0.9f, false));
    g.fillPath (body);

    const int ridges = juce::jlimit (44, 110, (int) (R * 1.5f));
    g.setColour (juce::Colour (0xff5d6068).withAlpha (0.5f));
    for (int i = 0; i < ridges; ++i)
    {
        const float a = (float) i * 2.0f * pi / (float) ridges;
        const auto p1 = polar (c, a, R * 0.90f);
        const auto p2 = polar (c, a, R * 0.995f);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, juce::jmax (1.0f, R * 0.02f));
    }

    // Tapa gris metálica (cuñas suaves) estilo SSL
    const float cr = R * 0.84f;
    g.setColour (juce::Colour (0xff2a2c31));
    g.fillEllipse (c.x - cr - 1.5f, c.y - cr - 1.5f, (cr + 1.5f) * 2.0f, (cr + 1.5f) * 2.0f);

    juce::Path cap;
    cap.addEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.saveState();
    g.reduceClipRegion (cap);
    const int wedges = 90;
    for (int k = 0; k < wedges; ++k)
    {
        const float a0 = (float) k * 2.0f * pi / (float) wedges;
        const float a1 = a0 + 2.0f * pi / (float) wedges * 1.25f;
        const float l = 0.5f + 0.5f * std::cos (2.0f * (a0 - 0.6f));
        const auto col = juce::Colour (0xff6d7077).interpolatedWith (juce::Colour (0xffb4b7be), l * 0.8f);

        juce::Path t;
        t.startNewSubPath (c);
        t.lineTo (polar (c, a0, cr * 1.08f));
        t.lineTo (polar (c, a1, cr * 1.08f));
        t.closeSubPath();
        g.setColour (col);
        g.fillPath (t);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), c.x - cr * 0.5f, c.y - cr * 0.7f,
                                             juce::Colours::transparentBlack, c.x + cr * 0.5f, c.y + cr * 0.7f, false));
    g.fillPath (cap);
    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawEllipse (c.x - cr + 1.5f, c.y - cr + 1.5f, cr * 2.0f - 3.0f, cr * 2.0f - 3.0f, 1.0f);

    // Puntero: barra oscura gruesa
    const auto p1 = polar (c, angle, R * 0.10f);
    const auto p2 = polar (c, angle, R * 0.82f);
    juce::Path ptr;
    ptr.startNewSubPath (p1);
    ptr.lineTo (p2);
    g.setColour (juce::Colour (0xff111216));
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (4.0f, R * 0.12f), juce::PathStrokeType::curved,
                                             juce::PathStrokeType::rounded));
}

void SslLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float, float, juce::Slider&)
{
    const float size = (float) juce::jmin (width, height);
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);
    paintKnob (g, c, size * 0.5f, sliderPos);
}

//==============================================================================
void SslButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();

    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 6, { 0, 2 }).drawForRectangle (g, b.toNearestInt());

    juce::Colour top, bottom;
    if (glow && on) { top = juce::Colour (0xfffdc88f); bottom = juce::Colour (0xffe98a38); }
    else if (on)    { top = juce::Colour (0xff8d9097); bottom = juce::Colour (0xffb6b9c0); }
    else            { top = juce::Colour (0xffc7cad1); bottom = juce::Colour (0xff80838a); }
    if (over && !on) { top = top.brighter (0.08f); bottom = bottom.brighter (0.08f); }

    g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), bottom, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 3.0f);
    g.setColour (juce::Colour (0xff0d0e10));
    g.drawRoundedRectangle (b, 3.0f, 1.3f);

    if (glow && on)
    {
        g.setColour (juce::Colour (0xffff9a3a).withAlpha (0.25f));
        g.drawRoundedRectangle (b.expanded (2.0f), 5.0f, 3.0f);
    }

    auto textArea = b;
    if (! glow)
    {
        const auto ledC = juce::Point<float> (b.getX() + 12.0f, b.getCentreY());
        if (on)
        {
            g.setColour (ledColour.withAlpha (0.3f));
            g.fillEllipse (ledC.x - 6.0f, ledC.y - 6.0f, 12.0f, 12.0f);
        }
        g.setColour (on ? ledColour : juce::Colour (0xff2c2e33));
        g.fillEllipse (ledC.x - 3.5f, ledC.y - 3.5f, 7.0f, 7.0f);
        textArea = textArea.withTrimmedLeft (16.0f);
    }
    g.setColour (juce::Colour (0xff15171b));
    g.setFont (numberFont (16.0f).withExtraKerningFactor (0.04f));
    g.drawText (getButtonText(), textArea, juce::Justification::centred, false);
}

//==============================================================================
DarkLevelAudioProcessorEditor::DarkLevelAudioProcessorEditor (DarkLevelAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      gainAttachment (p.apvts, "gain", gainKnob),
      autoAttachment (*p.apvts.getParameter ("auto"),
                      [this] (float v)
                      {
                          btnAuto.setToggleState (v > 0.5f, juce::dontSendNotification);
                          gainKnob.setEnabled (v < 0.5f);
                          gainKnob.setAlpha (v > 0.5f ? 0.45f : 1.0f);
                          repaint();
                      }),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v) { btnIn.setToggleState (v < 0.5f, juce::dontSendNotification); })
{
    gainKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    gainKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    gainKnob.setRotaryParameters (SslLookAndFeel::startAngle, SslLookAndFeel::endAngle, true);
    gainKnob.setDoubleClickReturnValue (true, 0.0);
    gainKnob.setLookAndFeel (&laf);
    gainKnob.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (gainKnob);

    btnAuto.onClick = [this] { autoAttachment.setValueAsCompleteGesture (btnAuto.getToggleState() ? 1.0f : 0.0f); };
    addAndMakeVisible (btnAuto);
    btnIn.onClick = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    addAndMakeVisible (btnIn);
    autoAttachment.sendInitialUpdate();
    bypassAttachment.sendInitialUpdate();

    buildTexture();
    setSize (kW, kH);
    startTimerHz (30);
}

DarkLevelAudioProcessorEditor::~DarkLevelAudioProcessorEditor()
{
    gainKnob.setLookAndFeel (nullptr);
}

void DarkLevelAudioProcessorEditor::buildTexture()
{
    const int w = kW, h = kH;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (7000);

    // Frontal negro con cepillado horizontal (igual que Bluey)
    float rowOffset = 0.0f;
    for (int y = 0; y < h; ++y)
    {
        rowOffset = rowOffset * 0.6f + (rng.nextFloat() - 0.5f) * 5.0f;
        const float vignette = 1.0f - 0.25f * std::abs ((float) y / (float) h - 0.5f);
        for (int x = 0; x < w; ++x)
        {
            const float n = (rng.nextFloat() - 0.5f) * 5.0f;
            const int v = juce::jlimit (0, 255, (int) ((24.0f + rowOffset + n) * vignette));
            texture.setPixelAt (x, y, juce::Colour ((juce::uint8) v, (juce::uint8) (v + 2), (juce::uint8) (v + 6), (juce::uint8) 255));
        }
    }

    juce::Graphics g (texture);
    for (int i = 0; i < 180; ++i) // vetas del cepillado
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float len = 80.0f + rng.nextFloat() * 320.0f;
        g.setColour (juce::Colours::white.withAlpha (0.015f + rng.nextFloat() * 0.035f));
        g.drawLine (x, y, x + len, y + (rng.nextFloat() - 0.5f) * 1.5f, 0.8f);
    }
}

void DarkLevelAudioProcessorEditor::resized()
{
    knobC = { 220.0f, 262.0f };
    meterBox = { 440.0f, 86.0f, 110.0f, 316.0f };
    gainKnob.setBounds (juce::Rectangle<float> (0, 0, 220, 220).withCentre (knobC).toNearestInt());
    btnAuto.setBounds (kW - 262, 28, 104, 36);
    btnIn.setBounds (kW - 150, 28, 96, 36);
}

void DarkLevelAudioProcessorEditor::timerCallback()
{
    const float p = processor.peakLin.exchange (0.0f);
    const float db = juce::Decibels::gainToDecibels (p, -100.0f);
    peakNow += (db > peakNow ? 0.85f : 0.15f) * (db - peakNow);
    if (db >= holdDb) { holdDb = db; holdAge = 0; }
    else if (++holdAge > 75) holdDb = juce::jmax (db, holdDb - 0.8f); // mantiene ~2.5 s y baja
    repaint();
}

void DarkLevelAudioProcessorEditor::drawScale (juce::Graphics& g, juce::Point<float> c, float dotR, float labelR,
                                             const juce::StringArray& labels, float fontSize)
{
    const int n = labels.size();
    g.setFont (numberFont (fontSize));
    for (int i = 0; i < n; ++i)
    {
        const float prop = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        const float a = SslLookAndFeel::startAngle + prop * (SslLookAndFeel::endAngle - SslLookAndFeel::startAngle);
        const auto dp = polar (c, a, dotR);
        g.setColour (dot);
        g.fillEllipse (dp.x - 2.4f, dp.y - 2.4f, 4.8f, 4.8f);

        g.setColour (number);
        g.drawText (labels[i], juce::Rectangle<float> (44.0f, fontSize + 8.0f).withCentre (polar (c, a, labelR)),
                    juce::Justification::centred, false);
    }
}

void DarkLevelAudioProcessorEditor::drawKnobTitle (juce::Graphics& g, float cx, float y,
                                                 const juce::String& name, const juce::String& unit)
{
    g.setColour (title);
    g.setFont (titleFont (14.0f).withExtraKerningFactor (0.1f));
    g.drawText (name, juce::Rectangle<float> (200.0f, 22.0f).withCentre ({ cx, y }), juce::Justification::centred, false);
    if (unit.isNotEmpty())
    {
        g.setColour (grey);
        g.setFont (labelFont (13.0f).withExtraKerningFactor (0.1f));
        g.drawText (unit, juce::Rectangle<float> (80.0f, 16.0f).withCentre ({ cx, y + 17.0f }), juce::Justification::centred, false);
    }
}

void DarkLevelAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d72), c.x - 3.0f, c.y - 3.0f,
                                             juce::Colour (0xff1c1d20), c.x + 8.0f, c.y + 8.0f, false));
    g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.drawLine (c.x - 5.0f, c.y + 2.0f, c.x + 5.0f, c.y - 2.0f, 2.0f);
}

void DarkLevelAudioProcessorEditor::drawMeter (juce::Graphics& g)
{
    constexpr float lo = -18.0f, hi = 0.0f;
    constexpr int segs = 36;
    const auto m = meterBox;
    auto yOf = [&] (float db) { return m.getBottom() - (juce::jlimit (lo, hi, db) - lo) / (hi - lo) * m.getHeight(); };

    // panel
    const auto panel = m.expanded (22.0f, 16.0f);
    g.setColour (juce::Colour (0xff060809));
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (juce::Colour (0xff343a42));
    g.drawRoundedRectangle (panel.reduced (0.5f), 8.0f, 1.2f);

    // zona objetivo -12..-6 (ancha, verde)
    const auto zone = juce::Rectangle<float> (panel.getX() + 4.0f, yOf (-6.0f), panel.getWidth() - 8.0f, yOf (-12.0f) - yOf (-6.0f));
    g.setColour (juce::Colour (0xff3ddc6a).withAlpha (0.16f));
    g.fillRoundedRectangle (zone, 4.0f);
    g.setColour (juce::Colour (0xff3ddc6a).withAlpha (0.55f));
    g.drawRoundedRectangle (zone, 4.0f, 1.4f);

    const float segH = m.getHeight() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float segDb = lo + ((float) i + 0.5f) / (float) segs * (hi - lo);
        const bool lit = segDb < peakNow;
        juce::Colour col;
        if (segDb > -6.0f)        col = juce::Colour (0xffff3a3a);
        else if (segDb >= -12.0f) col = juce::Colour (0xff3ddc6a);
        else                      col = juce::Colour (0xffffb02e);
        const auto r = juce::Rectangle<float> (m.getX(), m.getBottom() - segH * (float) (i + 1) + 1.0f, m.getWidth(), segH - 2.0f);
        if (lit) { g.setColour (col.withAlpha (0.22f)); g.fillRoundedRectangle (r.expanded (2.0f), 2.0f); }
        g.setColour (lit ? col : col.withMultipliedBrightness (0.16f).withMultipliedSaturation (0.7f));
        g.fillRoundedRectangle (r, 1.5f);
    }

    // marca de pico: barra blanca gruesa con contorno oscuro, brillo y flechas a los lados
    if (holdDb > lo - 1.0f)
    {
        const float y = yOf (holdDb);
        const bool hot = holdDb > -6.0f;
        const auto line = juce::Rectangle<float> (m.getX() - 12.0f, y - 2.5f, m.getWidth() + 24.0f, 5.0f);
        g.setColour (juce::Colours::black.withAlpha (0.85f));
        g.fillRoundedRectangle (line.expanded (1.5f), 2.5f);
        g.setColour ((hot ? juce::Colour (0xffffe3e3) : juce::Colours::white).withAlpha (0.28f));
        g.fillRoundedRectangle (line.expanded (4.0f, 3.0f), 4.0f);
        g.setColour (juce::Colours::white);
        g.fillRoundedRectangle (line, 2.0f);
        for (int s = -1; s <= 1; s += 2)
        {
            juce::Path tri;
            const float x0 = s < 0 ? panel.getX() - 2.0f : panel.getRight() + 2.0f;
            tri.addTriangle (x0, y - 7.0f, x0, y + 7.0f, x0 + (float) -s * 11.0f, y);
            g.setColour (juce::Colours::white);
            g.fillPath (tri);
        }
    }

    // escala
    g.setFont (numberFont (14.0f));
    for (float v : { 0.0f, -3.0f, -6.0f, -9.0f, -12.0f, -15.0f, -18.0f })
    {
        const bool edge = v == -6.0f || v == -12.0f;
        g.setColour (edge ? juce::Colour (0xff6bf09a) : number);
        g.drawText (juce::String ((int) v), juce::Rectangle<float> (40.0f, 16.0f).withCentre ({ panel.getRight() + 44.0f, yOf (v) }),
                    juce::Justification::centredLeft, false);
    }

    g.setColour (juce::Colour (0xff6bf09a));
    g.setFont (titleFont (10.0f).withExtraKerningFactor (0.18f));
    g.drawText ("TARGET", juce::Rectangle<float> (panel.getX() - 80.0f, zone.getCentreY() - 9.0f, 72.0f, 18.0f), juce::Justification::centredRight, false);
}

void DarkLevelAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    g.drawImageAt (texture, 0, 0);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (8.0f, 8.0f, w - 16.0f, h - 16.0f, 6.0f, 1.2f);
    drawScrew (g, { 24.0f, 24.0f });
    drawScrew (g, { w - 24.0f, 24.0f });
    drawScrew (g, { 24.0f, h - 24.0f });
    drawScrew (g, { w - 24.0f, h - 24.0f });

    g.setFont (titleFont (26.0f).withExtraKerningFactor (0.08f));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawText ("DARK LEVEL", 51, 29, 320, 36, juce::Justification::centredLeft);
    g.setColour (title);
    g.drawText ("DARK LEVEL", 50, 28, 320, 36, juce::Justification::centredLeft);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("GAIN STAGING", 52, 66, 300, 16, juce::Justification::centredLeft);

    // placa empotrada detrás de la perilla
    g.setColour (juce::Colour (0xff0a0c0f));
    g.fillRoundedRectangle (juce::Rectangle<float> (64.0f, 108.0f, 312.0f, 312.0f), 14.0f);
    g.setColour (juce::Colour (0xff2b3038));
    g.drawRoundedRectangle (juce::Rectangle<float> (64.5f, 108.5f, 311.0f, 311.0f), 14.0f, 1.2f);

    drawScale (g, knobC, 118.0f, 142.0f, { "-24", "-12", "0", "+12", "+24" }, 17.0f);
    const bool autoOn = btnAuto.getToggleState();
    const float shownGain = autoOn ? processor.autoGainDb.load() : (float) gainKnob.getValue();
    drawKnobTitle (g, knobC.x, 436.0f, autoOn ? "AUTO GAIN" : "GAIN",
                   (shownGain >= 0.0f ? "+" : "") + juce::String (shownGain, 1) + " dB");

    drawMeter (g);

    // estado
    const bool active = btnIn.getToggleState();
    const float pk = holdDb;
    juce::String status; juce::Colour sc;
    if (pk < -90.0f)       { status = "--";       sc = grey; }
    else if (pk > -6.0f)   { status = "TOO HOT";  sc = juce::Colour (0xffff4a4a); }
    else if (pk >= -12.0f) { status = "GOOD";     sc = juce::Colour (0xff46e07a); }
    else                   { status = "TOO LOW";  sc = amber; }
    g.setColour (sc);
    g.setFont (titleFont (16.0f).withExtraKerningFactor (0.12f));
    g.drawText (status, juce::Rectangle<float> (meterBox.getCentreX() - 90.0f, 424.0f, 180.0f, 22.0f), juce::Justification::centred, false);
    g.setFont (numberFont (22.0f));
    g.drawText (pk > -90.0f ? juce::String (pk, 1) + " dB" : juce::String ("-inf"),
                juce::Rectangle<float> (meterBox.getCentreX() - 90.0f, 446.0f, 180.0f, 24.0f), juce::Justification::centred, false);
    juce::ignoreUnused (active);

    g.setColour (juce::Colour (0xff6c7078));
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (50.0f, h - 36.0f, 260.0f, 16.0f), juce::Justification::centredLeft, false);
}
