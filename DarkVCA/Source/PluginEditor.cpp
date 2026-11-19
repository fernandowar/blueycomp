#include "PluginEditor.h"
#include "DarkVcaFonts.h"

using namespace SslColours;

namespace
{
    constexpr int kW = 780, kH = 540;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    // Michroma: títulos anchos tipo Eurostile. Barlow: números y botones.
    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkVcaFontData::Michroma_ttf, DarkVcaFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkVcaFontData::BarlowSemiBold_ttf, DarkVcaFontData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkVcaFontData::BarlowMedium_ttf, DarkVcaFontData::BarlowMedium_ttfSize);
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
DarkVcaAudioProcessorEditor::DarkVcaAudioProcessorEditor (DarkVcaAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      thresholdAttachment (p.apvts, "threshold", thresholdKnob),
      attackAttachment (p.apvts, "attack", attackKnob),
      makeupAttachment (p.apvts, "makeup", makeupKnob),
      releaseAttachment (*p.apvts.getParameter ("release"),
                         [this] (float v)
                         {
                             const bool autoOn = v > 0.5f;
                             btn100.setToggleState (! autoOn, juce::dontSendNotification);
                             btnAuto.setToggleState (autoOn, juce::dontSendNotification);
                         }),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v)
                        {
                            btnIn.setToggleState (v < 0.5f, juce::dontSendNotification);
                        })
{
    auto setupKnob = [this] (juce::Slider& s, double resetValue)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (SslLookAndFeel::startAngle, SslLookAndFeel::endAngle, true);
        s.setDoubleClickReturnValue (true, resetValue);
        s.setLookAndFeel (&laf);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };
    setupKnob (thresholdKnob, -10.0);
    setupKnob (attackKnob, 3.0);
    setupKnob (makeupKnob, 0.0);

    btn100.setRadioGroupId (1);
    btnAuto.setRadioGroupId (1);
    btn100.onClick  = [this] { releaseAttachment.setValueAsCompleteGesture (0.0f); };
    btnAuto.onClick = [this] { releaseAttachment.setValueAsCompleteGesture (1.0f); };
    btnIn.onClick   = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    addAndMakeVisible (btn100);
    addAndMakeVisible (btnAuto);
    addAndMakeVisible (btnIn);

    releaseAttachment.sendInitialUpdate();
    bypassAttachment.sendInitialUpdate();

    buildTexture();
    setSize (kW, kH);
    startTimerHz (30);
}

DarkVcaAudioProcessorEditor::~DarkVcaAudioProcessorEditor()
{
    thresholdKnob.setLookAndFeel (nullptr);
    attackKnob.setLookAndFeel (nullptr);
    makeupKnob.setLookAndFeel (nullptr);
}

void DarkVcaAudioProcessorEditor::buildTexture()
{
    const int w = kW, h = kH;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (4000);

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
            texture.setPixelAt (x, y, juce::Colour ((juce::uint8) v, (juce::uint8) v, (juce::uint8) (v + 1), (juce::uint8) 255));
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

void DarkVcaAudioProcessorEditor::resized()
{
    thrC = { 390.0f, 290.0f };
    atkC = { 140.0f, 290.0f };
    mkC  = { 640.0f, 290.0f };
    ladderBox = { 300.0f, 46.0f, 268.0f, 72.0f };

    thresholdKnob.setBounds (juce::Rectangle<float> (0, 0, 164, 164).withCentre (thrC).toNearestInt());
    attackKnob.setBounds (juce::Rectangle<float> (0, 0, 100, 100).withCentre (atkC).toNearestInt());
    makeupKnob.setBounds (juce::Rectangle<float> (0, 0, 100, 100).withCentre (mkC).toNearestInt());

    btn100.setBounds (302, 458, 84, 32);
    btnAuto.setBounds (394, 458, 84, 32);
    btnIn.setBounds (kW - 150, 28, 96, 36);
}

void DarkVcaAudioProcessorEditor::timerCallback()
{
    const float target = processor.gainReductionDb.load();
    grSmoothed += (target > grSmoothed ? 0.6f : 0.15f) * (target - grSmoothed);
    repaint (ladderBox.expanded (130.0f, 8.0f).toNearestInt());
}

void DarkVcaAudioProcessorEditor::drawScale (juce::Graphics& g, juce::Point<float> c, float dotR, float labelR,
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

void DarkVcaAudioProcessorEditor::drawKnobTitle (juce::Graphics& g, float cx, float y,
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

void DarkVcaAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d72), c.x - 3.0f, c.y - 3.0f,
                                             juce::Colour (0xff1c1d20), c.x + 8.0f, c.y + 8.0f, false));
    g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.drawLine (c.x - 5.0f, c.y + 2.0f, c.x + 5.0f, c.y - 2.0f, 2.0f);
}

void DarkVcaAudioProcessorEditor::drawLadder (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff070708));
    g.fillRoundedRectangle (ladderBox, 5.0f);
    g.setColour (juce::Colour (0xff34373c));
    g.drawRoundedRectangle (ladderBox.reduced (0.5f), 5.0f, 1.2f);

    struct Row { int db; juce::uint32 colour; };
    const Row rows[] = { { 1, 0xffffe45a }, { 3, 0xffffd23a }, { 6, 0xffffb02e }, { 10, 0xffff9a2e }, { 14, 0xffff3a3a }, { 20, 0xffd62020 } };
    const bool active = btnIn.getToggleState();

    for (int i = 0; i < 6; ++i)
    {
        const float cx = ladderBox.getX() + 24.0f + 44.0f * (float) i;
        const float ly = ladderBox.getY() + 26.0f;

        // LED de reducción
        const bool lit = grSmoothed >= (float) rows[i].db - 0.5f;
        const auto col = juce::Colour (rows[i].colour);
        if (lit)
        {
            g.setColour (col.withAlpha (0.3f));
            g.fillEllipse (cx - 11.0f, ly - 11.0f, 22.0f, 22.0f);
        }
        g.setColour (lit ? col : juce::Colour (0xff3b1a1e));
        g.fillEllipse (cx - 7.0f, ly - 7.0f, 14.0f, 14.0f);

        // Número
        g.setColour (number);
        g.setFont (numberFont (14.0f));
        g.drawText (juce::String (rows[i].db), juce::Rectangle<float> (40.0f, 16.0f).withCentre ({ cx, ladderBox.getBottom() - 15.0f }),
                    juce::Justification::centred, false);
    }

    // LED verde: plugin activo (a la izquierda de la caja)
    const float gx = ladderBox.getX() - 24.0f, gy = ladderBox.getCentreY();
    g.setColour (active ? juce::Colour (0xff3ddc6a).withAlpha (0.28f) : juce::Colours::transparentBlack);
    g.fillEllipse (gx - 10.0f, gy - 10.0f, 20.0f, 20.0f);
    g.setColour (active ? juce::Colour (0xff3ddc6a) : juce::Colour (0xff10301b));
    g.fillEllipse (gx - 6.5f, gy - 6.5f, 13.0f, 13.0f);

    // Lectura digital
    g.setColour (grSmoothed > 0.05f ? amber : juce::Colour (0xff7a6038));
    g.setFont (numberFont (22.0f));
    g.drawText (juce::String (grSmoothed, 1) + " dB",
                juce::Rectangle<float> (ladderBox.getRight() + 12.0f, ladderBox.getY(), 100.0f, ladderBox.getHeight()),
                juce::Justification::centredLeft, false);
}

void DarkVcaAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.drawImageAt (texture, 0, 0);

    // Bisel del frontal y tornillos
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (8.0f, 8.0f, w - 16.0f, h - 16.0f, 6.0f, 1.2f);
    drawScrew (g, { 24.0f, 24.0f });
    drawScrew (g, { w - 24.0f, 24.0f });
    drawScrew (g, { 24.0f, h - 24.0f });
    drawScrew (g, { w - 24.0f, h - 24.0f });

    // Título grabado
    g.setFont (titleFont (26.0f).withExtraKerningFactor (0.08f));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawText ("DARK VCA", 51, 29, 300, 36, juce::Justification::centredLeft);
    g.setColour (title);
    g.drawText ("DARK VCA", 50, 28, 300, 36, juce::Justification::centredLeft);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("VCA BUS COMPRESSOR", 52, 66, 300, 16, juce::Justification::centredLeft);

    drawLadder (g);

    // Perillas y escalas
    drawScale (g, thrC, 100.0f, 120.0f, { "+10", "0", "-10", "-20", "-30" }, 17.0f);
    drawScale (g, atkC, 66.0f, 83.0f, { "0.1", "0.3", "1", "3", "10", "30" }, 15.0f);
    drawScale (g, mkC,  66.0f, 83.0f, { "0", "3", "6", "9", "12", "15" }, 15.0f);

    drawKnobTitle (g, thrC.x, 402.0f, "THRESHOLD", "dB");
    drawKnobTitle (g, atkC.x, 402.0f, "ATTACK", "ms");
    drawKnobTitle (g, mkC.x,  402.0f, "MAKEUP", "dB");

    g.setColour (title);
    g.setFont (titleFont (12.0f).withExtraKerningFactor (0.12f));
    g.drawText ("RELEASE", juce::Rectangle<float> (302.0f, 434.0f, 176.0f, 18.0f), juce::Justification::centred, false);

    g.setColour (juce::Colour (0xff6c7078));
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (50.0f, h - 36.0f, 260.0f, 16.0f),
                juce::Justification::centredLeft, false);
}
