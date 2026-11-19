#include "PluginEditor.h"

using namespace BlueyColours;

static juce::Font makeFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::Point<float> polar (juce::Point<float> c, float angle, float r)
{
    return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
}

//==============================================================================
void BlueyLookAndFeel::paint1176Knob (juce::Graphics& g, juce::Point<float> c, float R, float proportion)
{
    const float pi = juce::MathConstants<float>::pi;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    // Sombra proyectada
    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (R * 0.5f),
                      { 0, (int) (R * 0.18f) }).drawForPath (g, body);

    // Falda negra
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff38383b), c.x - R * 0.6f, c.y - R * 0.8f,
                                             juce::Colour (0xff07070a), c.x + R * 0.7f, c.y + R * 0.9f, false));
    g.fillPath (body);

    // Estrías (knurling)
    const int ridges = juce::jlimit (40, 96, (int) (R * 1.2f));
    g.setColour (juce::Colour (0xff55555a).withAlpha (0.55f));
    for (int i = 0; i < ridges; ++i)
    {
        const float a = (float) i * 2.0f * pi / (float) ridges;
        const auto p1 = polar (c, a, R * 0.90f);
        const auto p2 = polar (c, a, R * 0.995f);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, juce::jmax (1.0f, R * 0.02f));
    }

    // Borde biselado de la tapa
    const float cr = R * 0.76f;
    g.setColour (juce::Colour (0xffe3e7ea));
    g.fillEllipse (c.x - cr - 1.4f, c.y - cr - 1.4f, (cr + 1.4f) * 2.0f, (cr + 1.4f) * 2.0f);

    // Tapa de aluminio cepillado (efecto cónico con cuñas)
    juce::Path cap;
    cap.addEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.saveState();
    g.reduceClipRegion (cap);
    const int wedges = 96;
    for (int k = 0; k < wedges; ++k)
    {
        const float a0 = (float) k * 2.0f * pi / (float) wedges;
        const float a1 = a0 + 2.0f * pi / (float) wedges * 1.25f;
        const float l = 0.5f + 0.5f * std::cos (2.0f * (a0 - 0.55f));
        const auto col = juce::Colour (0xff7f8e99).interpolatedWith (juce::Colour (0xffeaf2f7), l);

        juce::Path t;
        t.startNewSubPath (c);
        t.lineTo (polar (c, a0, cr * 1.08f));
        t.lineTo (polar (c, a1, cr * 1.08f));
        t.closeSubPath();
        g.setColour (col);
        g.fillPath (t);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.16f), c.x - cr * 0.5f, c.y - cr * 0.6f,
                                             juce::Colours::transparentBlack, c.x + cr * 0.6f, c.y + cr * 0.7f, false));
    g.fillPath (cap);
    g.restoreState();

    g.setColour (juce::Colour (0xff2a2e33).withAlpha (0.8f));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);

    // Marcador: anillo negro sobre la tapa
    const auto m = polar (c, angle, cr * 0.64f);
    const float rr = R * 0.105f;
    g.setColour (juce::Colour (0xff0c0c0d));
    g.drawEllipse (m.x - rr, m.y - rr, rr * 2.0f, rr * 2.0f, juce::jmax (1.5f, R * 0.05f));
}

void BlueyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float, juce::Slider&)
{
    const float size = (float) juce::jmin (width, height);
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);
    paint1176Knob (g, c, size * 0.5f * 0.82f, sliderPos);
}

//==============================================================================
void PowerButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();
    const auto col = on ? cyan : juce::Colour (0xff5a5f6b);

    g.setColour (juce::Colour (0xff0a0b0d).withAlpha (over ? 1.0f : 0.85f));
    g.fillEllipse (b);
    g.setColour (juce::Colour (0xff3a3d43));
    g.drawEllipse (b, 1.5f);

    const auto c = b.getCentre();
    const float r = b.getWidth() * 0.26f;
    juce::Path icon;
    icon.addCentredArc (c.x, c.y, r, r, 0.0f,
                        juce::degreesToRadians (35.0f), juce::degreesToRadians (325.0f), true);
    icon.startNewSubPath (c.x, c.y - r * 1.15f);
    icon.lineTo (c.x, c.y - r * 0.1f);

    if (on)
    {
        g.setColour (col.withAlpha (0.25f));
        g.strokePath (icon, juce::PathStrokeType (5.0f, juce::PathStrokeType::curved,
                                                  juce::PathStrokeType::rounded));
    }
    g.setColour (col);
    g.strokePath (icon, juce::PathStrokeType (2.2f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
}

//==============================================================================
BlueyCompAudioProcessorEditor::BlueyCompAudioProcessorEditor (BlueyCompAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      compressAttachment (p.apvts, "compress", compressKnob),
      outputAttachment (p.apvts, "output", outputKnob),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v)
                        {
                            powerButton.setToggleState (v < 0.5f, juce::dontSendNotification);
                            powerButton.repaint();
                        })
{
    auto setupKnob = [this] (juce::Slider& s, double resetValue)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (BlueyLookAndFeel::startAngle, BlueyLookAndFeel::endAngle, true);
        s.setDoubleClickReturnValue (true, resetValue);
        s.setLookAndFeel (&laf);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };
    setupKnob (compressKnob, 0.35);
    setupKnob (outputKnob, 0.0);

    powerButton.onClick = [this]
    {
        bypassAttachment.setValueAsCompleteGesture (powerButton.getToggleState() ? 0.0f : 1.0f);
    };
    addAndMakeVisible (powerButton);
    bypassAttachment.sendInitialUpdate();

    buildTexture();
    setSize (740, 480);
    startTimerHz (30);
}

BlueyCompAudioProcessorEditor::~BlueyCompAudioProcessorEditor()
{
    compressKnob.setLookAndFeel (nullptr);
    outputKnob.setLookAndFeel (nullptr);
}

void BlueyCompAudioProcessorEditor::buildTexture()
{
    const int w = 740, h = 480;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (1176);

    // Frontal negro con cepillado horizontal
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
    for (int i = 0; i < 160; ++i) // vetas del cepillado
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float len = 80.0f + rng.nextFloat() * 320.0f;
        g.setColour (juce::Colours::white.withAlpha (0.015f + rng.nextFloat() * 0.035f));
        g.drawLine (x, y, x + len, y + (rng.nextFloat() - 0.5f) * 1.5f, 0.8f);
    }
}

void BlueyCompAudioProcessorEditor::resized()
{
    bigC = { 370.0f, 282.0f };
    outC = { 640.0f, 334.0f };

    compressKnob.setBounds (juce::Rectangle<float> (0, 0, 200, 200).withCentre (bigC).toNearestInt());
    outputKnob.setBounds (juce::Rectangle<float> (0, 0, 110, 110).withCentre (outC).toNearestInt());
    powerButton.setBounds (662, 20, 36, 36);

    meterArea = { 220.0f, 98.0f, 300.0f, 14.0f };
}

void BlueyCompAudioProcessorEditor::timerCallback()
{
    const float target = processor.gainReductionDb.load();
    grSmoothed += (target > grSmoothed ? 0.5f : 0.10f) * (target - grSmoothed);
    repaint (meterArea.expanded (90.0f, 30.0f).toNearestInt());
}

void BlueyCompAudioProcessorEditor::drawKnobScale (juce::Graphics& g, juce::Point<float> c, float tickStart,
                                                   float majorLen, float minorLen, float labelRadius,
                                                   const juce::StringArray& majors, int minorsBetween, float fontSize)
{
    const int n = majors.size();
    const int steps = (n - 1) * (minorsBetween + 1);
    g.setFont (makeFont (fontSize));

    for (int s = 0; s <= steps; ++s)
    {
        const float prop = (float) s / (float) steps;
        const float a = BlueyLookAndFeel::startAngle + prop * (BlueyLookAndFeel::endAngle - BlueyLookAndFeel::startAngle);
        const bool major = (s % (minorsBetween + 1)) == 0;

        const auto p1 = polar (c, a, tickStart);
        const auto p2 = polar (c, a, tickStart + (major ? majorLen : minorLen));
        g.setColour (white);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, major ? 1.8f : 1.1f);

        if (major)
        {
            const auto lp = polar (c, a, labelRadius);
            g.drawText (majors[s / (minorsBetween + 1)],
                        juce::Rectangle<float> (44.0f, fontSize + 8.0f).withCentre (lp),
                        juce::Justification::centred, false);
        }
    }
}

void BlueyCompAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d72), c.x - 3.0f, c.y - 3.0f,
                                             juce::Colour (0xff1c1d20), c.x + 8.0f, c.y + 8.0f, false));
    g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.drawLine (c.x - 5.0f, c.y + 2.0f, c.x + 5.0f, c.y - 2.0f, 2.0f);
}

void BlueyCompAudioProcessorEditor::drawBarMeter (juce::Graphics& g)
{
    // Título del medidor
    g.setColour (grey);
    g.setFont (makeFont (12.0f).withExtraKerningFactor (0.14f));
    g.drawText ("GAIN REDUCTION  dB",
                juce::Rectangle<float> (meterArea.getX(), meterArea.getY() - 24.0f, meterArea.getWidth(), 16.0f),
                juce::Justification::centred, false);

    // Ventana negra hundida
    const auto well = meterArea.expanded (6.0f, 5.0f);
    g.setColour (juce::Colour (0xff070708));
    g.fillRoundedRectangle (well, 4.0f);
    g.setColour (juce::Colour (0xff34373c));
    g.drawRoundedRectangle (well.reduced (0.5f), 4.0f, 1.2f);

    const int segs = 30;
    const float segW = meterArea.getWidth() / (float) segs;
    const float maxDb = 20.0f;
    for (int i = 0; i < segs; ++i)
    {
        const bool on = ((float) i + 0.5f) / (float) segs * maxDb < grSmoothed;
        const auto r = juce::Rectangle<float> (meterArea.getX() + segW * (float) i + 0.5f, meterArea.getY(),
                                               segW - 1.5f, meterArea.getHeight());
        if (on)
        {
            g.setColour (ledOn.withAlpha (0.25f));
            g.fillRoundedRectangle (r.expanded (1.5f), 2.0f);
        }
        g.setColour (on ? ledOn : ledOff);
        g.fillRoundedRectangle (r, 1.5f);
    }

    // Escala numerada
    g.setFont (makeFont (11.0f));
    for (int v = 0; v <= 20; v += 2)
    {
        const float x = meterArea.getX() + (float) v / maxDb * meterArea.getWidth();
        g.setColour (white.withAlpha (0.75f));
        g.fillRect (x - 0.5f, well.getBottom() + 2.0f, 1.0f, 4.0f);
        g.drawText (juce::String (v),
                    juce::Rectangle<float> (26.0f, 12.0f).withCentre ({ x, well.getBottom() + 15.0f }),
                    juce::Justification::centred, false);
    }

    // Lectura digital
    g.setColour (grSmoothed > 0.05f ? ledOn : juce::Colour (0xff6b5646));
    g.setFont (makeFont (20.0f));
    g.drawText (juce::String (grSmoothed, 1) + " dB",
                juce::Rectangle<float> (well.getRight() + 14.0f, meterArea.getY() - 5.0f, 90.0f, 24.0f),
                juce::Justification::centredLeft, false);
}

void BlueyCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.drawImageAt (texture, 0, 0);

    // Bisel del frontal
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (8.0f, 8.0f, w - 16.0f, h - 16.0f, 6.0f, 1.2f);
    drawScrew (g, { 24.0f, 24.0f });
    drawScrew (g, { w - 24.0f, 24.0f });
    drawScrew (g, { 24.0f, h - 24.0f });
    drawScrew (g, { w - 24.0f, h - 24.0f });

    // Título grabado
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.setFont (makeFont (30.0f).withExtraKerningFactor (0.14f));
    g.drawText ("BLUEY COMP", 50, 21, 400, 40, juce::Justification::centredLeft);
    g.setColour (white);
    g.drawText ("BLUEY COMP", 49, 20, 400, 40, juce::Justification::centredLeft);
    g.setColour (grey);
    g.setFont (makeFont (11.0f).withExtraKerningFactor (0.16f));
    g.drawText ("FET LIMITING AMPLIFIER", 51, 56, 400, 16, juce::Justification::centredLeft);

    // Meter
    drawBarMeter (g);

    // Escalas y nombres
    drawKnobScale (g, bigC, 92.0f, 11.0f, 6.0f, 119.0f,
                   { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" }, 1, 19.0f);
    g.setColour (white);
    g.setFont (makeFont (22.0f).withExtraKerningFactor (0.14f));
    g.drawText ("COMPRESS", juce::Rectangle<float> (240.0f, 30.0f).withCentre ({ bigC.x, 428.0f }),
                juce::Justification::centred, false);

    drawKnobScale (g, outC, 51.0f, 8.0f, 4.0f, 75.0f, { "-12", "-6", "0", "+6", "+12" }, 1, 13.0f);
    g.setColour (white);
    g.setFont (makeFont (22.0f).withExtraKerningFactor (0.14f));
    g.drawText ("OUTPUT", juce::Rectangle<float> (240.0f, 30.0f).withCentre ({ outC.x, 428.0f }),
                juce::Justification::centred, false);

    g.setColour (juce::Colour (0xff6c7078));
    g.setFont (makeFont (11.0f).withExtraKerningFactor (0.2f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, h - 34.0f, w, 16.0f),
                juce::Justification::centred, false);
}
