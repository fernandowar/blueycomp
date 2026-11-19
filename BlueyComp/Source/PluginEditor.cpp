#include "PluginEditor.h"

using namespace BlueyColours;

static juce::Font makeFont (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
void BlueyLookAndFeel::paintKnob (juce::Graphics& g, juce::Rectangle<float> b, float proportion)
{
    const auto c = b.getCentre();
    const float r = b.getWidth() * 0.5f;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    // Sombra
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.fillEllipse (b.expanded (r * 0.06f).translated (0.0f, r * 0.05f));

    // Anillo exterior
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff30323f), c.x, b.getY(),
                                             juce::Colour (0xff0e0f15), c.x, b.getBottom(), false));
    g.fillEllipse (b);

    // Surco + arco naranja
    const float arcR = r * 0.90f;
    const float thick = juce::jmax (3.0f, r * 0.075f);

    juce::Path track;
    track.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff090a10));
    g.strokePath (track, juce::PathStrokeType (thick + 2.0f, juce::PathStrokeType::curved,
                                               juce::PathStrokeType::rounded));

    if (proportion > 0.002f)
    {
        juce::Path arc;
        arc.addCentredArc (c.x, c.y, arcR, arcR, 0.0f, startAngle, angle, true);

        g.setColour (orange.withAlpha (0.22f));
        g.strokePath (arc, juce::PathStrokeType (thick + 6.0f, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
        g.setColour (orange);
        g.strokePath (arc, juce::PathStrokeType (thick, juce::PathStrokeType::curved,
                                                 juce::PathStrokeType::rounded));
    }

    // Tapa interior
    const auto inner = b.reduced (r * 0.20f);
    g.setColour (juce::Colour (0xff3b3d4a));
    g.fillEllipse (inner.expanded (1.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2d38), c.x, inner.getY(),
                                             juce::Colour (0xff0f1016), c.x, inner.getBottom(), false));
    g.fillEllipse (inner);

    // Punto indicador
    const float dotR = juce::jmax (2.0f, r * 0.045f);
    const float dr = r * 0.62f;
    const float dx = c.x + std::sin (angle) * dr;
    const float dy = c.y - std::cos (angle) * dr;
    g.setColour (orange.withAlpha (0.3f));
    g.fillEllipse (dx - dotR * 2.0f, dy - dotR * 2.0f, dotR * 4.0f, dotR * 4.0f);
    g.setColour (orange);
    g.fillEllipse (dx - dotR, dy - dotR, dotR * 2.0f, dotR * 2.0f);
}

void BlueyLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                         float sliderPos, float, float, juce::Slider&)
{
    const float size = (float) juce::jmin (width, height);
    juce::Rectangle<float> b ((float) x + ((float) width - size) * 0.5f,
                              (float) y + ((float) height - size) * 0.5f, size, size);
    paintKnob (g, b.reduced (4.0f), sliderPos);
}

//==============================================================================
void PowerButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();
    const auto col = on ? cyan : juce::Colour (0xff5a5f73);

    g.setColour (juce::Colour (0xff0f1220).withAlpha (over ? 1.0f : 0.85f));
    g.fillEllipse (b);
    g.setColour (juce::Colour (0xff2b3250));
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
      powerAttachment (p.apvts, "power", powerButton)
{
    compressKnob.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    compressKnob.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    compressKnob.setRotaryParameters (BlueyLookAndFeel::startAngle, BlueyLookAndFeel::endAngle, true);
    compressKnob.setDoubleClickReturnValue (true, 0.35);
    compressKnob.setLookAndFeel (&laf);
    compressKnob.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (compressKnob);

    addAndMakeVisible (powerButton);

    setSize (440, 566);
    startTimerHz (30);
}

BlueyCompAudioProcessorEditor::~BlueyCompAudioProcessorEditor()
{
    compressKnob.setLookAndFeel (nullptr);
}

void BlueyCompAudioProcessorEditor::resized()
{
    bigKnobArea = { 85.0f, 100.0f, 270.0f, 270.0f };
    compressKnob.setBounds (bigKnobArea.toNearestInt());
    powerButton.setBounds (386, 52, 34, 34);

    bottomPanel = { 20.0f, 396.0f, 400.0f, 130.0f };
    grilleArea  = { 150.0f, 538.0f, 140.0f, 16.0f };
}

void BlueyCompAudioProcessorEditor::timerCallback()
{
    const float target = processor.gainReductionDb.load();
    // Subida rápida, caída lenta
    grSmoothed += (target > grSmoothed ? 0.6f : 0.15f) * (target - grSmoothed);
    repaint (grilleArea.expanded (4.0f).toNearestInt());
}

void BlueyCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    const auto w = (float) getWidth();
    const auto h = (float) getHeight();

    // Fondo
    g.setGradientFill (juce::ColourGradient (bgTop, 0.0f, 0.0f, bgBottom, 0.0f, h, false));
    g.fillAll();

    // Barra superior
    g.setColour (juce::Colour (0xff1b1c2a));
    g.fillRect (0.0f, 0.0f, w, 44.0f);
    g.setColour (juce::Colour (0xff3a4163));
    g.drawHorizontalLine (44, 0.0f, w);

    g.setColour (juce::Colour (0xffb7bccd));
    g.setFont (makeFont (17.0f).withExtraKerningFactor (0.06f));
    g.drawText ("DARK SHAPE", 18, 0, 160, 44, juce::Justification::centredLeft);
    g.setColour (orange);
    g.fillPath ([] { juce::Path t; t.addTriangle (0, 8, 7, 8, 3.5f, 0); return t; }(),
                juce::AffineTransform::translation (136.0f, 18.0f));

    g.setColour (juce::Colour (0xffd9dcea));
    g.setFont (makeFont (18.0f).withExtraKerningFactor (0.1f));
    g.drawText ("[ BLUEY COMP ]", getWidth() - 220, 0, 202, 44, juce::Justification::centredRight);

    // Título de la perilla grande
    g.setColour (label);
    g.setFont (makeFont (24.0f).withExtraKerningFactor (0.05f));
    g.drawText ("COMPRESS", 0, 62, getWidth(), 30, juce::Justification::centred);

    // Panel inferior
    g.setColour (juce::Colour (0xff1c2140));
    g.fillRoundedRectangle (bottomPanel, 28.0f);
    g.setColour (juce::Colour (0xff2c3358));
    g.drawRoundedRectangle (bottomPanel.reduced (0.5f), 28.0f, 1.5f);

    // Controles fijos (decorativos): Attack 3, Release 7, Ratio 4
    struct Fixed { const char* name; const char* value; float prop; };
    const Fixed fixedKnobs[] = {
        { "ATTACK",  "3", 3.0f / 7.0f },
        { "RELEASE", "7", 1.0f },
        { "RATIO",   "4", 0.08f }
    };

    const float cy = bottomPanel.getY() + 52.0f;
    for (int i = 0; i < 3; ++i)
    {
        const float cx = bottomPanel.getX() + bottomPanel.getWidth() * (0.18f + 0.32f * (float) i);
        BlueyLookAndFeel::paintKnob (g, { cx - 36.0f, cy - 36.0f, 72.0f, 72.0f }, fixedKnobs[i].prop);

        g.setColour (label);
        g.setFont (makeFont (15.0f).withExtraKerningFactor (0.04f));
        g.drawText (juce::String (fixedKnobs[i].name) + "  " + fixedKnobs[i].value,
                    (int) cx - 60, (int) (cy + 44.0f), 120, 22, juce::Justification::centred);
    }

    // Rejilla inferior = medidor de gain reduction
    const int cols = 14, rows = 3;
    const int lit = juce::jlimit (0, cols, (int) std::round (grSmoothed / 18.0f * (float) cols));
    const float dotSize = 5.0f;
    const float stepX = grilleArea.getWidth() / (float) cols;
    const float stepY = grilleArea.getHeight() / (float) rows;
    for (int r = 0; r < rows; ++r)
        for (int c = 0; c < cols; ++c)
        {
            const bool on = c < lit;
            const float dx = grilleArea.getX() + stepX * (float) c + (stepX - dotSize) * 0.5f;
            const float dy = grilleArea.getY() + stepY * (float) r + (stepY - dotSize) * 0.5f;
            g.setColour (on ? orange : juce::Colour (0xff0d0f19));
            g.fillEllipse (dx, dy, dotSize, dotSize);
        }
}
