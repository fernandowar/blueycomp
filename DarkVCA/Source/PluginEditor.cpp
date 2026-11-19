#include "PluginEditor.h"

using namespace SslColours;

static juce::Font makeFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::Point<float> polar (juce::Point<float> c, float angle, float r)
{
    return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
}

//==============================================================================
void SslLookAndFeel::paintKnob (juce::Graphics& g, juce::Point<float> c, float R, float proportion)
{
    const float angle = startAngle + proportion * (endAngle - startAngle);

    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), (int) (R * 0.5f),
                      { 0, (int) (R * 0.14f) }).drawForPath (g, body);

    // Base oscura
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4b4f5b), c.x, c.y - R,
                                             juce::Colour (0xff17181e), c.x, c.y + R, false));
    g.fillPath (body);

    // Tapa crema
    const float cr = R * 0.90f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff7f5ee), c.x - cr * 0.6f, c.y - cr * 0.8f,
                                             juce::Colour (0xffb4b2a8), c.x + cr * 0.7f, c.y + cr * 0.9f, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.setColour (juce::Colour (0xff5f5e58).withAlpha (0.55f));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawEllipse (c.x - cr + 2.0f, c.y - cr + 2.0f, cr * 2.0f - 4.0f, cr * 2.0f - 4.0f, 1.0f);

    // Puntero oscuro
    const auto p1 = polar (c, angle, R * 0.06f);
    const auto p2 = polar (c, angle, R * 0.80f);
    juce::Path ptr;
    ptr.startNewSubPath (p1);
    ptr.lineTo (p2);
    g.setColour (juce::Colour (0xff1b1e24));
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (3.0f, R * 0.10f), juce::PathStrokeType::curved,
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
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();

    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 6, { 0, 2 }).drawForRectangle (g, b.toNearestInt());

    juce::Colour top, bottom;
    if (glow && on) { top = juce::Colour (0xfffdc88f); bottom = juce::Colour (0xffe98a38); }
    else if (on)    { top = juce::Colour (0xffa9a79f); bottom = juce::Colour (0xffcdcbc3); }
    else            { top = juce::Colour (0xffd9d7cf); bottom = juce::Colour (0xff9e9c94); }
    if (over && !on) { top = top.brighter (0.08f); bottom = bottom.brighter (0.08f); }

    g.setGradientFill (juce::ColourGradient (top, b.getX(), b.getY(), bottom, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 3.0f);
    g.setColour (juce::Colour (0xff44464c));
    g.drawRoundedRectangle (b, 3.0f, 1.2f);

    if (glow && on)
    {
        g.setColour (juce::Colour (0xffff9a3a).withAlpha (0.25f));
        g.drawRoundedRectangle (b.expanded (2.0f), 5.0f, 3.0f);
    }

    g.setColour (juce::Colour (0xff1d2026));
    g.setFont (makeFont (15.0f));
    auto textArea = b;
    if (! glow)
    {
        const auto ledC = juce::Point<float> (b.getX() + 11.0f, b.getCentreY());
        if (on)
        {
            g.setColour (ledColour.withAlpha (0.3f));
            g.fillEllipse (ledC.x - 6.0f, ledC.y - 6.0f, 12.0f, 12.0f);
        }
        g.setColour (on ? ledColour : juce::Colour (0xff3a3d44));
        g.fillEllipse (ledC.x - 3.5f, ledC.y - 3.5f, 7.0f, 7.0f);
        textArea = textArea.withTrimmedLeft (14.0f);
        g.setColour (juce::Colour (0xff1d2026));
    }
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
    setupKnob (thresholdKnob, 0.0);
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
    setSize (740, 440);
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
    const int w = 740, h = 440;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (4000);

    for (int y = 0; y < h; ++y)
    {
        const float t = (float) y / (float) h;
        for (int x = 0; x < w; ++x)
        {
            const float n = (rng.nextFloat() - 0.5f) * 8.0f;
            const int r = juce::jlimit (0, 255, (int) (30.0f - 8.0f * t + n));
            const int gg = juce::jlimit (0, 255, (int) (37.0f - 9.0f * t + n));
            const int b = juce::jlimit (0, 255, (int) (58.0f - 14.0f * t + n));
            texture.setPixelAt (x, y, juce::Colour ((juce::uint8) r, (juce::uint8) gg, (juce::uint8) b, (juce::uint8) 255));
        }
    }

    juce::Graphics g (texture);
    for (int i = 0; i < 90; ++i) // rayones y marcas de uso
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float len = 8.0f + rng.nextFloat() * 45.0f;
        const float a = rng.nextFloat() * juce::MathConstants<float>::twoPi;
        g.setColour (juce::Colours::white.withAlpha (0.015f + rng.nextFloat() * 0.04f));
        g.drawLine (x, y, x + std::cos (a) * len, y + std::sin (a) * len, 0.7f);
    }
}

void DarkVcaAudioProcessorEditor::resized()
{
    thrC = { 112.0f, 196.0f };
    atkC = { 300.0f, 196.0f };
    mkC  = { 475.0f, 196.0f };
    ladderBox = { 590.0f, 66.0f, 124.0f, 184.0f };

    thresholdKnob.setBounds (juce::Rectangle<float> (0, 0, 100, 100).withCentre (thrC).toNearestInt());
    attackKnob.setBounds (juce::Rectangle<float> (0, 0, 84, 84).withCentre (atkC).toNearestInt());
    makeupKnob.setBounds (juce::Rectangle<float> (0, 0, 84, 84).withCentre (mkC).toNearestInt());

    btn100.setBounds (303, 336, 80, 32);
    btnAuto.setBounds (391, 336, 80, 32);
    btnIn.setBounds (590, 302, 124, 34);
}

void DarkVcaAudioProcessorEditor::timerCallback()
{
    const float target = processor.gainReductionDb.load();
    grSmoothed += (target > grSmoothed ? 0.6f : 0.15f) * (target - grSmoothed);
    repaint (ladderBox.expanded (6.0f, 40.0f).toNearestInt());
}

void DarkVcaAudioProcessorEditor::drawScale (juce::Graphics& g, juce::Point<float> c, float dotR, float labelR,
                                             const juce::StringArray& labels, float fontSize)
{
    const int n = labels.size();
    g.setFont (makeFont (fontSize));
    for (int i = 0; i < n; ++i)
    {
        const float prop = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        const float a = SslLookAndFeel::startAngle + prop * (SslLookAndFeel::endAngle - SslLookAndFeel::startAngle);
        const auto dp = polar (c, a, dotR);
        g.setColour (dot);
        g.fillEllipse (dp.x - 2.2f, dp.y - 2.2f, 4.4f, 4.4f);

        g.setColour (number);
        g.drawText (labels[i], juce::Rectangle<float> (40.0f, fontSize + 8.0f).withCentre (polar (c, a, labelR)),
                    juce::Justification::centred, false);
    }
}

void DarkVcaAudioProcessorEditor::drawKnobTitle (juce::Graphics& g, juce::Point<float> c, float labelR,
                                                 const juce::String& name, const juce::String& unit)
{
    const float y = c.y + labelR * 0.71f + 22.0f;
    g.setColour (title);
    g.setFont (makeFont (16.0f).withExtraKerningFactor (0.06f));
    g.drawText (name, juce::Rectangle<float> (130.0f, 22.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
    if (unit.isNotEmpty())
    {
        g.setColour (number);
        g.setFont (makeFont (12.0f, false));
        g.drawText (unit, juce::Rectangle<float> (60.0f, 16.0f).withCentre ({ c.x, y + 16.0f }), juce::Justification::centred, false);
    }
}

void DarkVcaAudioProcessorEditor::drawLadder (juce::Graphics& g)
{
    g.setColour (juce::Colour (0xff10131f).withAlpha (0.6f));
    g.fillRoundedRectangle (ladderBox, 6.0f);
    g.setColour (juce::Colour (0xff5b6684));
    g.drawRoundedRectangle (ladderBox.reduced (0.5f), 6.0f, 1.4f);

    struct Row { int db; juce::uint32 colour; };
    const Row rows[] = { { 20, 0xffd62020 }, { 14, 0xffff3a3a }, { 10, 0xffff9a2e }, { 6, 0xffffb02e }, { 3, 0xffffd23a } };

    for (int i = 0; i < 5; ++i)
    {
        const float y = ladderBox.getY() + 26.0f + 33.0f * (float) i;
        const float gx = ladderBox.getX() + 24.0f;
        const float rx = ladderBox.getRight() - 24.0f;

        // LED verde: plugin activo
        const bool active = btnIn.getToggleState();
        g.setColour (active ? juce::Colour (0xff3ddc6a).withAlpha (0.25f) : juce::Colours::transparentBlack);
        g.fillEllipse (gx - 9.0f, y - 9.0f, 18.0f, 18.0f);
        g.setColour (active ? juce::Colour (0xff3ddc6a) : juce::Colour (0xff10301b));
        g.fillEllipse (gx - 6.0f, y - 6.0f, 12.0f, 12.0f);

        // Número
        g.setColour (number);
        g.setFont (makeFont (15.0f));
        g.drawText (juce::String (rows[i].db), juce::Rectangle<float> (40.0f, 18.0f).withCentre ({ ladderBox.getCentreX(), y }),
                    juce::Justification::centred, false);

        // LED de reducción
        const bool lit = grSmoothed >= (float) rows[i].db;
        const auto col = juce::Colour (rows[i].colour);
        if (lit)
        {
            g.setColour (col.withAlpha (0.3f));
            g.fillEllipse (rx - 10.0f, y - 10.0f, 20.0f, 20.0f);
        }
        g.setColour (lit ? col : juce::Colour (0xff3b1a1e));
        g.fillEllipse (rx - 6.5f, y - 6.5f, 13.0f, 13.0f);
    }

    g.setColour (grSmoothed > 0.05f ? amber : juce::Colour (0xff7a6038));
    g.setFont (makeFont (17.0f));
    g.drawText (juce::String (grSmoothed, 1) + " dB",
                juce::Rectangle<float> (ladderBox.getX(), ladderBox.getBottom() + 8.0f, ladderBox.getWidth(), 22.0f),
                juce::Justification::centred, false);
}

void DarkVcaAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth();

    g.drawImageAt (texture, 0, 0);

    // Encabezado: —— COMPRESSOR ——
    g.setColour (juce::Colour (0xff6c7694));
    g.drawHorizontalLine (28, 18.0f, 270.0f);
    g.drawHorizontalLine (28, w - 270.0f, w - 18.0f);
    g.setColour (title);
    g.setFont (makeFont (20.0f).withExtraKerningFactor (0.1f));
    g.drawText ("COMPRESSOR", juce::Rectangle<float> (0.0f, 14.0f, w, 28.0f), juce::Justification::centred, false);

    // Divisiones de panel
    g.setColour (juce::Colour (0xff5b6684).withAlpha (0.8f));
    g.drawVerticalLine (566, 52.0f, 372.0f);
    g.drawHorizontalLine (382, 0.0f, w);

    // Perillas y escalas
    drawScale (g, thrC, 68.0f, 86.0f, { "+10", "+5", "0", "-5", "-10", "-15", "-20" }, 14.0f);
    drawScale (g, atkC, 57.0f, 73.0f, { "0.1", "0.3", "1", "3", "10", "30" }, 14.0f);
    drawScale (g, mkC,  57.0f, 73.0f, { "0", "3", "6", "9", "12", "15" }, 14.0f);

    drawKnobTitle (g, thrC, 86.0f, "THRESHOLD", "dB");
    drawKnobTitle (g, atkC, 73.0f, "ATTACK", "ms");
    drawKnobTitle (g, mkC,  73.0f, "MAKEUP", "dB");

    g.setColour (title);
    g.setFont (makeFont (16.0f).withExtraKerningFactor (0.06f));
    g.drawText ("RELEASE", juce::Rectangle<float> (303.0f, 310.0f, 168.0f, 20.0f), juce::Justification::centred, false);

    drawLadder (g);

    // Marca inferior
    g.setColour (juce::Colour (0xffd2d4dc));
    g.setFont (juce::Font (juce::FontOptions ("Times New Roman", 36.0f, juce::Font::bold)));
    g.drawText ("Dark VCA", 26, 388, 360, 50, juce::Justification::centredLeft);
    g.setColour (juce::Colour (0xff8890a8));
    g.setFont (makeFont (15.0f).withExtraKerningFactor (0.12f));
    g.drawText ("VCA BUS COMPRESSOR", getWidth() - 340, 388, 316, 50, juce::Justification::centredRight);
}
