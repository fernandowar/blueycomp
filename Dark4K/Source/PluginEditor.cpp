#include "PluginEditor.h"
#include "Dark4kFonts.h"

using namespace D4kColours;

namespace
{
    constexpr int kW = 1000, kH = 800;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }
    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::Michroma_ttf, Dark4kFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::BarlowSemiBold_ttf, Dark4kFontData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::BarlowMedium_ttf, Dark4kFontData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Point<float> polar (juce::Point<float> c, float angle, float r)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }
    const juce::Colour kCenterLight { 0xffb9bbc0 };
}

//==============================================================================
void D4kLookAndFeel::paintKnob (juce::Graphics& g, juce::Point<float> c, float R, float proportion, juce::Colour capCol)
{
    const float pi = juce::MathConstants<float>::pi;
    const float angle = startAngle + proportion * (endAngle - startAngle);
    const bool coloured = capCol != silver && capCol != white;

    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), (int) (R * 0.5f), { 0, (int) (R * 0.16f) }).drawForPath (g, body);

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3c42), c.x - R * 0.6f, c.y - R * 0.8f,
                                             juce::Colour (0xff0b0b0e), c.x + R * 0.7f, c.y + R * 0.9f, false));
    g.fillPath (body);

    const int ridges = juce::jlimit (36, 110, (int) (R * 1.5f));
    g.setColour (juce::Colour (0xff5d6068).withAlpha (0.5f));
    for (int i = 0; i < ridges; ++i)
    {
        const float a = (float) i * 2.0f * pi / (float) ridges;
        const auto p1 = polar (c, a, R * 0.90f), p2 = polar (c, a, R * 0.995f);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, juce::jmax (1.0f, R * 0.02f));
    }

    const float cr = R * 0.84f;
    g.setColour (juce::Colour (0xff2a2c31));
    g.fillEllipse (c.x - cr - 1.5f, c.y - cr - 1.5f, (cr + 1.5f) * 2.0f, (cr + 1.5f) * 2.0f);

    juce::Path cap;
    cap.addEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);
    g.saveState();
    g.reduceClipRegion (cap);
    const juce::Colour dark = coloured ? capCol.darker (0.55f) : juce::Colour (0xff6d7077);
    const juce::Colour light = coloured ? capCol.brighter (0.55f) : (capCol == white ? juce::Colour (0xffe6e7ea) : juce::Colour (0xffb4b7be));
    const int wedges = 90;
    for (int k = 0; k < wedges; ++k)
    {
        const float a0 = (float) k * 2.0f * pi / (float) wedges;
        const float a1 = a0 + 2.0f * pi / (float) wedges * 1.25f;
        const float l = 0.5f + 0.5f * std::cos (2.0f * (a0 - 0.6f));
        juce::Path t;
        t.startNewSubPath (c);
        t.lineTo (polar (c, a0, cr * 1.08f));
        t.lineTo (polar (c, a1, cr * 1.08f));
        t.closeSubPath();
        g.setColour (dark.interpolatedWith (light, l * 0.8f));
        g.fillPath (t);
    }
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.18f), c.x - cr * 0.5f, c.y - cr * 0.7f,
                                             juce::Colours::transparentBlack, c.x + cr * 0.5f, c.y + cr * 0.7f, false));
    g.fillPath (cap);
    g.restoreState();

    g.setColour (juce::Colours::white.withAlpha (0.28f));
    g.drawEllipse (c.x - cr + 1.5f, c.y - cr + 1.5f, cr * 2.0f - 3.0f, cr * 2.0f - 3.0f, 1.0f);

    const auto p1 = polar (c, angle, R * 0.10f), p2 = polar (c, angle, R * 0.82f);
    juce::Path ptr;
    ptr.startNewSubPath (p1);
    ptr.lineTo (p2);
    g.setColour (coloured ? juce::Colour (0xfff4f4f6) : juce::Colour (0xff111216));
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (3.0f, R * 0.12f), juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
}

void D4kLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float, float, juce::Slider& s)
{
    const float size = (float) juce::jmin (width, height);
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);
    const auto cap = juce::Colour ((juce::uint32) (int) s.getProperties().getWithDefault ("cap", (int) silver.getARGB()));
    paintKnob (g, c, size * 0.5f, sliderPos, cap);
}

void D4kLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                       float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider&)
{
    const float cx = (float) x + (float) width * 0.5f;
    const float top = (float) y + (float) faderThumbHalf, bottom = (float) (y + height - faderThumbHalf);

    // ranura
    g.setColour (juce::Colour (0xff0a0b0d));
    g.fillRoundedRectangle (cx - 4.0f, top - 6.0f, 8.0f, bottom - top + 12.0f, 4.0f);
    g.setColour (juce::Colour (0xff3a3d44));
    g.drawRoundedRectangle (cx - 4.0f, top - 6.0f, 8.0f, bottom - top + 12.0f, 4.0f, 1.0f);

    // tapa del fader (plateada con estrías)
    juce::Rectangle<float> capR (cx - 28.0f, sliderPos - (float) faderThumbHalf, 56.0f, (float) faderThumbHalf * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 10, { 0, 4 }).drawForRectangle (g, capR.toNearestInt());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f3f5), capR.getX(), capR.getY(),
                                             juce::Colour (0xff9da0a8), capR.getX(), capR.getBottom(), false));
    g.fillRoundedRectangle (capR, 4.0f);
    g.setColour (juce::Colour (0xff1c1d20));
    g.drawRoundedRectangle (capR, 4.0f, 1.2f);
    for (int i = 1; i < 8; ++i)
    {
        const float yy = capR.getY() + capR.getHeight() * (float) i / 8.0f;
        g.setColour (juce::Colour (0xff7c7f87).withAlpha (0.8f));
        g.drawLine (capR.getX() + 3.0f, yy, capR.getRight() - 3.0f, yy, 1.4f);
        g.setColour (juce::Colours::white.withAlpha (0.5f));
        g.drawLine (capR.getX() + 3.0f, yy + 1.4f, capR.getRight() - 3.0f, yy + 1.4f, 1.0f);
    }
    g.setColour (juce::Colour (0xff15161a));
    g.fillRect (capR.getX() + 3.0f, capR.getCentreY() - 1.0f, capR.getWidth() - 6.0f, 2.0f);
}

//==============================================================================
void D4kButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();

    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 5, { 0, 2 }).drawForRectangle (g, b.toNearestInt());

    juce::Colour top, bottom;
    if (glow && on) { top = juce::Colour (0xfffdc88f); bottom = juce::Colour (0xffe98a38); }
    else if (on)    { top = juce::Colour (0xff8d9097); bottom = juce::Colour (0xffb6b9c0); }
    else            { top = juce::Colour (0xffc7cad1); bottom = juce::Colour (0xff80838a); }
    if (over && ! on) { top = top.brighter (0.08f); bottom = bottom.brighter (0.08f); }

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
    if (! glow && ledColour.getAlpha() > 0 && b.getWidth() > 50.0f)
    {
        const auto ledC = juce::Point<float> (b.getX() + 11.0f, b.getCentreY());
        if (on) { g.setColour (ledColour.withAlpha (0.3f)); g.fillEllipse (ledC.x - 6.0f, ledC.y - 6.0f, 12.0f, 12.0f); }
        g.setColour (on ? ledColour : juce::Colour (0xff2c2e33));
        g.fillEllipse (ledC.x - 3.5f, ledC.y - 3.5f, 7.0f, 7.0f);
        textArea = textArea.withTrimmedLeft (14.0f);
    }
    g.setColour (juce::Colour (0xff15171b));
    g.setFont (numberFont (b.getHeight() > 30.0f ? 16.0f : 14.0f).withExtraKerningFactor (0.04f));
    g.drawText (getButtonText(), textArea, juce::Justification::centred, false);
}

//==============================================================================
Dark4kAudioProcessorEditor::Knob& Dark4kAudioProcessorEditor::addKnob (const juce::String& id, const juce::String& title,
                                                                      const juce::String& unit, juce::Colour cap, double reset,
                                                                      std::vector<std::pair<double, juce::String>> marks, bool)
{
    auto k = std::make_unique<Knob>();
    k->title = title; k->unit = unit; k->marks = std::move (marks);
    auto& s = k->slider;
    s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    s.setRotaryParameters (D4kLookAndFeel::startAngle, D4kLookAndFeel::endAngle, true);
    s.setDoubleClickReturnValue (true, reset);
    s.getProperties().set ("cap", (int) cap.getARGB());
    s.setLookAndFeel (&laf);
    s.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (s);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, id, s);
    s.setName (id);
    knobs.push_back (std::move (k));
    return *knobs.back();
}

D4kButton& Dark4kAudioProcessorEditor::addToggle (const juce::String& id, const juce::String& text, juce::Colour led, bool glow, bool invert)
{
    auto t = std::make_unique<Toggle>();
    t->button = std::make_unique<D4kButton> (text, led, glow);
    auto* b = t->button.get();
    auto* param = processor.apvts.getParameter (id);
    t->att = std::make_unique<juce::ParameterAttachment> (*param, [b, invert] (float v)
    {
        b->setToggleState (invert ? v < 0.5f : v > 0.5f, juce::dontSendNotification);
    });
    auto* att = t->att.get();
    b->onClick = [b, att, invert] { att->setValueAsCompleteGesture ((b->getToggleState() != invert) ? 1.0f : 0.0f); };
    b->setName (id);
    addAndMakeVisible (*b);
    att->sendInitialUpdate();
    toggles.push_back (std::move (t));
    return *b;
}

Dark4kAudioProcessorEditor::Dark4kAudioProcessorEditor (Dark4kAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      releaseAtt (*p.apvts.getParameter ("release"),
                  [this] (float v)
                  {
                      btn100.setToggleState (v < 0.5f, juce::dontSendNotification);
                      btnAuto.setToggleState (v > 0.5f, juce::dontSendNotification);
                  })
{
    using M = std::vector<std::pair<double, juce::String>>;
    const M gainMarks { { -15, "-15" }, { -10, "" }, { -5, "-5" }, { 0, "0" }, { 5, "5" }, { 10, "" }, { 15, "15" } };
    const M qMarks { { 0.5, ".5" }, { 0.8, "" }, { 1.2, "1" }, { 2.0, "2" }, { 3.0, "3" } };

    // Filtros
    addKnob ("hpf", "HPF", "Hz", white, 16.0, { { 16, "OUT" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 350, "350" } });
    addKnob ("lpf", "LPF", "kHz", white, 22000.0, { { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 22000, "OUT" } });
    // HF
    addKnob ("hfg", "HF", "dB", red, 0.0, gainMarks);
    addKnob ("hff", "FREQ", "kHz", red, 8000.0, { { 1500, "1.5" }, { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 16000, "16" } });
    // HMF
    addKnob ("hmfg", "HMF", "dB", green, 0.0, gainMarks);
    addKnob ("hmff", "FREQ", "kHz", green, 3000.0, { { 600, ".6" }, { 1000, "1" }, { 2000, "2" }, { 4000, "4" }, { 7000, "7" } });
    addKnob ("hmfq", "Q", "", green, 1.2, qMarks);
    // LMF
    addKnob ("lmfg", "LMF", "dB", blue, 0.0, gainMarks);
    addKnob ("lmff", "FREQ", "kHz", blue, 800.0, { { 200, ".2" }, { 400, ".4" }, { 800, ".8" }, { 1500, "1.5" }, { 2500, "2.5" } });
    addKnob ("lmfq", "Q", "", blue, 1.2, qMarks);
    // LF
    addKnob ("lfg", "LF", "dB", brown, 0.0, gainMarks);
    addKnob ("lff", "FREQ", "Hz", brown, 100.0, { { 30, "30" }, { 60, "60" }, { 100, "100" }, { 200, "200" }, { 450, "450" } });
    // Centro
    addKnob ("intrim", "IN TRIM", "dB", white, 0.0, { { -20, "-20" }, { -10, "-10" }, { 0, "0" }, { 10, "10" }, { 20, "+20" } });
    // Dark VCA
    addKnob ("threshold", "THRESHOLD", "dB", silver, 10.0,
             { { 10, "+10" }, { 5, "+5" }, { 0, "0" }, { -5, "-5" }, { -10, "-10" }, { -15, "-15" }, { -20, "-20" } });
    addKnob ("attack", "ATTACK", "ms", silver, 3.0, { { 0, "0.1" }, { 1, "0.3" }, { 2, "1" }, { 3, "3" }, { 4, "10" }, { 5, "30" } });
    addKnob ("makeup", "MAKEUP", "dB", silver, 0.0, { { 0, "0" }, { 3, "3" }, { 6, "6" }, { 9, "9" }, { 12, "12" }, { 15, "15" } });
    // Puerta
    addKnob ("gatethr", "THRESHOLD", "dB", silver, -70.0, { { -70, "OFF" }, { -50, "-50" }, { -30, "-30" }, { -10, "-10" }, { 0, "0" } });
    addKnob ("gaterange", "RANGE", "dB", silver, 0.0, { { 0, "0" }, { 10, "10" }, { 20, "20" }, { 30, "30" }, { 40, "40" } });

    for (auto* kk : { "hfg", "hmfg", "lmfg", "lfg" }) juce::ignoreUnused (kk);

    // Botones
    addToggle ("bypass", "POWER", juce::Colours::transparentBlack, true, true);
    addToggle ("phase", "\xc3\x98", juce::Colours::transparentBlack, true);
    addToggle ("filtin", "IN", juce::Colours::transparentBlack, true);
    addToggle ("eqin", "IN", juce::Colours::transparentBlack, true);
    addToggle ("eqblack", "BLACK", juce::Colour (0xff46e07a));
    addToggle ("hfbell", "BELL", juce::Colour (0xff46e07a));
    addToggle ("lfbell", "BELL", juce::Colour (0xff46e07a));
    addToggle ("dynin", "IN", juce::Colours::transparentBlack, true);

    btn100.setRadioGroupId (7);
    btnAuto.setRadioGroupId (7);
    btn100.onClick  = [this] { releaseAtt.setValueAsCompleteGesture (0.0f); };
    btnAuto.onClick = [this] { releaseAtt.setValueAsCompleteGesture (1.0f); };
    addAndMakeVisible (btn100);
    addAndMakeVisible (btnAuto);
    releaseAtt.sendInitialUpdate();

    fader.setSliderStyle (juce::Slider::LinearVertical);
    fader.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    fader.setDoubleClickReturnValue (true, 0.0);
    fader.setLookAndFeel (&laf);
    fader.setPopupDisplayEnabled (true, true, this);
    addAndMakeVisible (fader);
    faderAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (processor.apvts, "level", fader);

    buildTexture();
    setSize (kW, kH);
    startTimerHz (30);
}

Dark4kAudioProcessorEditor::~Dark4kAudioProcessorEditor()
{
    for (auto& k : knobs) k->slider.setLookAndFeel (nullptr);
    fader.setLookAndFeel (nullptr);
}

void Dark4kAudioProcessorEditor::buildTexture()
{
    texture = juce::Image (juce::Image::RGB, kW, kH, false);
    juce::Graphics g (texture);
    juce::Random rng (4200);

    auto region = [&] (juce::Rectangle<int> r, juce::Colour top, juce::Colour bottom, float lineAlpha, bool darkLines)
    {
        g.setGradientFill (juce::ColourGradient (top, 0.0f, (float) r.getY(), bottom, 0.0f, (float) r.getBottom(), false));
        g.fillRect (r);
        for (int y = r.getY(); y < r.getBottom(); ++y)
        {
            g.setColour ((darkLines ? juce::Colours::black : juce::Colours::white).withAlpha (rng.nextFloat() * lineAlpha));
            g.drawHorizontalLine (y, (float) r.getX(), (float) r.getRight());
        }
        for (int i = 0; i < r.getWidth() * r.getHeight() / 1800; ++i)
        {
            const float x = (float) r.getX() + rng.nextFloat() * (float) r.getWidth(), yy = (float) r.getY() + rng.nextFloat() * (float) r.getHeight();
            const float len = 6.0f + rng.nextFloat() * 30.0f;
            g.setColour (juce::Colours::white.withAlpha (0.015f + rng.nextFloat() * 0.03f));
            g.drawLine (x, yy, juce::jmin (x + len, (float) r.getRight()), yy + (rng.nextFloat() - 0.5f) * 2.0f, 1.0f);
        }
    };

    region ({ 0, 0, 250, kH }, juce::Colour (0xff2c3039), juce::Colour (0xff23262e), 0.05f, true);
    region ({ 750, 0, 250, kH }, juce::Colour (0xff2c3039), juce::Colour (0xff23262e), 0.05f, true);
    region ({ 250, 0, 125, kH }, juce::Colour (0xff16181d), juce::Colour (0xff101216), 0.04f, true);
    region ({ 625, 0, 125, kH }, juce::Colour (0xff16181d), juce::Colour (0xff101216), 0.04f, true);
    region ({ 375, 0, 250, 270 }, juce::Colour (0xff2a2d35), juce::Colour (0xff1f2128), 0.05f, true);
    region ({ 375, 270, 250, kH - 270 }, juce::Colour (0xffc6c8cc), juce::Colour (0xffa9abb0), 0.06f, true);

    g.setColour (juce::Colour (0xff0b0c0f));
    for (int x : { 250, 375, 625, 750 }) g.fillRect (x - 1, 0, 2, kH);
    g.setColour (juce::Colour (0xff0b0c0f));
    g.fillRect (375, 269, 250, 3);
    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRect (0, 0, kW, kH, 1);
}

void Dark4kAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a8d94), c.x - 4, c.y - 4, juce::Colour (0xff2b2d32), c.x + 5, c.y + 5, true));
    g.fillEllipse (c.x - 6, c.y - 6, 12, 12);
    g.setColour (juce::Colour (0xff15161a));
    g.drawLine (c.x - 4, c.y - 2, c.x + 4, c.y + 2, 1.6f);
}

void Dark4kAudioProcessorEditor::mouseDown (const juce::MouseEvent& e)
{
    if (chDisplay.contains (e.position))
    {
        processor.rerollChannel();
        repaint();
    }
}

void Dark4kAudioProcessorEditor::resized()
{
    auto place = [this] (const char* id, float cx, float cy, float R, float scaleR, float labelR, float font)
    {
        for (auto& k : knobs)
            if (k->slider.getName() == id)
            {
                k->slider.setBounds (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre ({ cx, cy }).toNearestInt());
                k->scaleR = scaleR; k->labelR = labelR; k->fontSize = font;
            }
    };
    auto btn = [this] (const char* id, int x, int y, int w, int h)
    {
        for (auto& t : toggles)
            if (t->button->getName() == id) t->button->setBounds (x, y, w, h);
    };

    // --- Izquierda: filtros + EQ ---
    place ("hpf", 70, 92, 28, 38, 50, 12.0f);
    place ("lpf", 180, 92, 28, 38, 50, 12.0f);
    btn ("filtin", 160, 156, 74, 28);

    const float cols[3] = { 44, 125, 206 };
    const float R = 21, sR = 29, lR = 41;
    place ("hfg", cols[0], 268, R, sR, lR, 10.0f);   place ("hff", cols[1], 268, R, sR, lR, 10.0f);   btn ("hfbell", 176, 254, 60, 28);
    place ("hmfg", cols[0], 398, R, sR, lR, 10.0f);  place ("hmff", cols[1], 398, R, sR, lR, 10.0f);  place ("hmfq", cols[2], 398, R, sR, lR, 10.0f);
    btn ("eqin", 90, 476, 64, 30);  btn ("eqblack", 160, 476, 76, 30);
    place ("lmfg", cols[0], 580, R, sR, lR, 10.0f);  place ("lmff", cols[1], 580, R, sR, lR, 10.0f);  place ("lmfq", cols[2], 580, R, sR, lR, 10.0f);
    place ("lfg", cols[0], 706, R, sR, lR, 10.0f);   place ("lff", cols[1], 706, R, sR, lR, 10.0f);   btn ("lfbell", 176, 692, 60, 28);

    // --- Centro ---
    btn ("bypass", 262, 16, 100, 34);
    chDisplay = { 392.0f, 14.0f, 216.0f, 38.0f };
    place ("intrim", 500, 138, 36, 49, 62, 11.0f);
    btn ("phase", 462, 226, 76, 30);
    fader.setBounds (460, 300, 80, 420);
    inMeterBox  = { 262.0f, 96.0f, 100.0f, 640.0f };
    outMeterBox = { 638.0f, 96.0f, 100.0f, 640.0f };

    // --- Derecha: Dark VCA + puerta ---
    btn ("dynin", 846, 22, 92, 34);
    place ("threshold", 875, 176, 44, 60, 76, 12.0f);
    place ("attack", 810, 312, 32, 44, 58, 11.0f);
    place ("makeup", 940, 312, 32, 44, 58, 11.0f);
    btn100.setBounds (772, 404, 98, 32);
    btnAuto.setBounds (880, 404, 98, 32);
    ladderBox = { 785.0f, 460.0f, 180.0f, 160.0f };
    place ("gatethr", 810, 706, 28, 38, 52, 11.0f);
    place ("gaterange", 940, 706, 28, 38, 52, 11.0f);
}

void Dark4kAudioProcessorEditor::timerCallback()
{
    auto upd = [] (float& shown, float& hold, int& age, float lin)
    {
        const float db = juce::Decibels::gainToDecibels (lin, -100.0f);
        shown += (db > shown ? 0.9f : 0.18f) * (db - shown);
        if (db >= hold) { hold = db; age = 0; }
        else if (++age > 45) hold = juce::jmax (db, hold - 0.6f);
    };
    for (int ch = 0; ch < 2; ++ch)
    {
        upd (inLvl[ch], inHold[ch], holdAge[ch], processor.inPeak[ch].exchange (0.0f));
        upd (outLvl[ch], outHold[ch], holdAge[2 + ch], processor.outPeak[ch].exchange (0.0f));
    }
    const float gr = processor.gainReductionDb.load();
    grSmoothed += (gr > grSmoothed ? 0.5f : 0.12f) * (gr - grSmoothed);
    repaint();
}

void Dark4kAudioProcessorEditor::drawKnobScale (juce::Graphics& g, Knob& k)
{
    const auto c = k.slider.getBounds().toFloat().getCentre();
    const bool onLight = false;
    juce::ignoreUnused (onLight);
    for (auto& m : k.marks)
    {
        const float prop = (float) k.slider.valueToProportionOfLength (m.first);
        const float a = D4kLookAndFeel::startAngle + prop * (D4kLookAndFeel::endAngle - D4kLookAndFeel::startAngle);
        const auto p = polar (c, a, k.scaleR);
        g.setColour (dot);
        g.fillEllipse (p.x - 1.8f, p.y - 1.8f, 3.6f, 3.6f);
        if (m.second.isNotEmpty())
        {
            const auto q = polar (c, a, k.labelR);
            g.setColour (number);
            g.setFont (numberFont (k.fontSize));
            g.drawText (m.second, juce::Rectangle<float> (40.0f, 14.0f).withCentre (q), juce::Justification::centred, false);
        }
    }
    // título bajo la perilla
    const float y = c.y + k.labelR * 0.72f + 11.0f;
    g.setColour (title);
    g.setFont (titleFont (k.slider.getWidth() > 70 ? 13.0f : 10.0f).withExtraKerningFactor (0.04f));
    g.drawText (k.title, juce::Rectangle<float> (110.0f, 16.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
    if (k.unit.isNotEmpty())
    {
        g.setColour (grey);
        g.setFont (labelFont (11.0f));
        g.drawText (k.unit, juce::Rectangle<float> (110.0f, 14.0f).withCentre ({ c.x, y + 14.0f }), juce::Justification::centred, false);
    }
}

void Dark4kAudioProcessorEditor::drawMeters (juce::Graphics& g, juce::Rectangle<float> r, const float* levels, const float* holds, const juce::String& titleText)
{
    constexpr float lo = -60.0f;
    const bool scaleLeft = r.getX() < 400.0f;
    const float barW = 22.0f, gap = 8.0f;
    const float bx0 = scaleLeft ? r.getX() + 46.0f : r.getX() + 4.0f;
    const float yTop = r.getY(), h = r.getHeight();
    auto yOf = [&] (float db) { return yTop + h - (juce::jlimit (lo, 0.0f, db) - lo) / (0.0f - lo) * h; };

    g.setColour (grey);
    g.setFont (titleFont (12.0f).withExtraKerningFactor (0.08f));
    g.drawText (titleText, juce::Rectangle<float> (r.getX(), 56.0f, r.getWidth() + 10.0f, 18.0f), juce::Justification::centred, false);

    for (int ch = 0; ch < 2; ++ch)
    {
        const float x = bx0 + (float) ch * (barW + gap);
        g.setColour (juce::Colour (0xff07080a));
        g.fillRoundedRectangle (x, yTop, barW, h, 2.0f);
        const int segs = 64;
        const float segH = h / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float segDb = lo + ((float) i + 0.5f) / (float) segs * (0.0f - lo);
            if (segDb < levels[ch])
            {
                const float t = juce::jlimit (0.0f, 1.0f, (segDb - lo) / 60.0f);
                g.setColour (juce::Colour (0xffc65a14).interpolatedWith (juce::Colour (0xffff9a3c), t));
                g.fillRect (x + 1.0f, yTop + h - segH * (float) (i + 1) + 0.5f, barW - 2.0f, segH - 1.0f);
            }
        }
        // marca de pico
        if (holds[ch] > lo)
        {
            g.setColour (juce::Colour (0xffffd9a8));
            g.fillRect (x + 1.0f, yOf (holds[ch]) - 1.0f, barW - 2.0f, 2.0f);
        }
        // LED de clip
        const bool clip = holds[ch] > -0.1f;
        const juce::Rectangle<float> led (x, yTop - 16.0f, barW, 9.0f);
        g.setColour (clip ? juce::Colour (0xffff3a3a) : juce::Colour (0xff3a1418));
        g.fillRoundedRectangle (led, 2.0f);
    }

    // escala 0..60 (rojo arriba, verde abajo)
    g.setFont (numberFont (14.0f));
    for (int v = 0; v <= 60; v += 6)
    {
        g.setColour (v <= 6 ? juce::Colour (0xffff4a4a) : (v <= 12 ? juce::Colour (0xffffb347) : juce::Colour (0xff3ddc6a)));
        const float y = yOf ((float) -v);
        const float sx = scaleLeft ? r.getX() : r.getX() + 4.0f + 2.0f * (barW + gap) + 2.0f;
        g.drawText (juce::String (v), juce::Rectangle<float> (sx, y - 8.0f, 40.0f, 16.0f),
                    scaleLeft ? juce::Justification::centredLeft : juce::Justification::centredLeft, false);
    }
}

void Dark4kAudioProcessorEditor::drawLadder (juce::Graphics& g, juce::Rectangle<float> box)
{
    g.setColour (juce::Colour (0xff10131c).withAlpha (0.6f));
    g.fillRoundedRectangle (box, 6.0f);
    g.setColour (juce::Colour (0xff5b6684));
    g.drawRoundedRectangle (box.reduced (0.5f), 6.0f, 1.2f);

    struct Row { int db; juce::Colour col; };
    const Row rows[5] = { { 20, juce::Colour (0xffd62020) }, { 14, juce::Colour (0xffff3a3a) }, { 10, juce::Colour (0xffff9a2e) },
                          { 6, juce::Colour (0xffffb02e) }, { 3, juce::Colour (0xffffd23a) } };
    for (int i = 0; i < 5; ++i)
    {
        const float y = box.getY() + 22.0f + 26.0f * (float) i;
        const float gx = box.getX() + 24.0f, rx = box.getRight() - 24.0f;
        g.setColour (juce::Colour (0xff3ddc6a).withAlpha (0.25f));
        g.fillEllipse (gx - 8.0f, y - 8.0f, 16.0f, 16.0f);
        g.setColour (juce::Colour (0xff3ddc6a));
        g.fillEllipse (gx - 5.5f, y - 5.5f, 11.0f, 11.0f);
        g.setColour (number);
        g.setFont (numberFont (14.0f));
        g.drawText (juce::String (rows[i].db), juce::Rectangle<float> (box.getCentreX() - 20.0f, y - 8.0f, 40.0f, 16.0f), juce::Justification::centred, false);
        const bool lit = grSmoothed >= (float) rows[i].db * 0.999f;
        if (lit) { g.setColour (rows[i].col.withAlpha (0.3f)); g.fillEllipse (rx - 9.0f, y - 9.0f, 18.0f, 18.0f); }
        g.setColour (lit ? rows[i].col : rows[i].col.withMultipliedBrightness (0.2f));
        g.fillEllipse (rx - 5.5f, y - 5.5f, 11.0f, 11.0f);
    }
    g.setColour (amber);
    g.setFont (numberFont (18.0f));
    g.drawText (juce::String (grSmoothed, 1) + " dB", juce::Rectangle<float> (box.getX(), box.getBottom() - 26.0f, box.getWidth(), 22.0f), juce::Justification::centred, false);
}

void Dark4kAudioProcessorEditor::drawFaderScale (juce::Graphics& g)
{
    const auto b = fader.getBounds().toFloat();
    const float top = b.getY() + (float) D4kLookAndFeel::faderThumbHalf, bottom = b.getBottom() - (float) D4kLookAndFeel::faderThumbHalf;
    const float x0 = b.getCentreX() + 40.0f;
    g.setFont (numberFont (16.0f));
    struct T { double v; const char* t; };
    for (auto t : { T { 12, "12" }, T { 6, "6" }, T { 0, "0" }, T { -5, "5" }, T { -10, "10" }, T { -20, "20" }, T { -30, "30" },
                    T { -40, "40" }, T { -50, "50" }, T { -80, "\xe2\x88\x9e" } })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (t.v) * (bottom - top);
        g.setColour (juce::Colour (0xff2a2c31));
        g.drawLine (x0, y, x0 + 8.0f, y, 1.4f);
        g.drawText (juce::CharPointer_UTF8 (t.t), juce::Rectangle<float> (x0 + 12.0f, y - 9.0f, 36.0f, 18.0f), juce::Justification::centredLeft, false);
    }
}

void Dark4kAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (texture, 0, 0);

    for (auto c : { juce::Point<float> { 14, 14 }, { 236, 14 }, { 14, 786 }, { 236, 786 }, { 764, 14 }, { 986, 14 }, { 764, 786 }, { 986, 786 } })
        drawScrew (g, c);

    // ---- Izquierda ----
    auto sectionLabel = [&] (const juce::String& t, float x, float y)
    {
        g.setColour (title);
        g.setFont (titleFont (14.0f).withExtraKerningFactor (0.1f));
        g.drawText (t, juce::Rectangle<float> (x, y - 10.0f, 120.0f, 20.0f), juce::Justification::centredLeft, false);
    };
    sectionLabel ("FILTERS", 14.0f, 170.0f);
    g.setColour (juce::Colour (0xff6c7078)); g.drawLine (12.0f, 196.0f, 238.0f, 196.0f, 1.4f);
    sectionLabel ("HF", 14.0f, 218.0f);
    sectionLabel ("HMF", 14.0f, 348.0f);
    sectionLabel ("EQ", 14.0f, 491.0f);
    g.setColour (juce::Colour (0xff6c7078)); g.drawLine (12.0f, 522.0f, 238.0f, 522.0f, 1.4f);
    sectionLabel ("LMF", 14.0f, 530.0f);
    sectionLabel ("LF", 14.0f, 656.0f);
    g.setColour (juce::Colour (0xff6c7078)); g.drawLine (12.0f, 336.0f, 238.0f, 336.0f, 1.0f);
    g.drawLine (12.0f, 646.0f, 238.0f, 646.0f, 1.0f);

    // ---- Derecha ----
    g.setColour (title);
    g.setFont (titleFont (16.0f).withExtraKerningFactor (0.1f));
    g.drawText ("DYN", 766, 22, 70, 34, juce::Justification::centredLeft);
    g.setFont (titleFont (13.0f).withExtraKerningFactor (0.14f));
    g.drawText ("COMPRESS", 750, 74, 250, 20, juce::Justification::centred);
    g.setColour (juce::Colour (0xff6c7078)); g.drawLine (768.0f, 98.0f, 982.0f, 98.0f, 1.2f);
    g.setColour (title);
    g.drawText ("RELEASE", 750, 380, 250, 18, juce::Justification::centred);
    g.drawText ("GATE", 750, 628, 250, 20, juce::Justification::centred);
    g.setColour (juce::Colour (0xff6c7078)); g.drawLine (768.0f, 650.0f, 982.0f, 650.0f, 1.2f);

    drawLadder (g, ladderBox);

    for (auto& k : knobs) drawKnobScale (g, *k);

    // ---- Centro ----
    g.setColour (juce::Colour (0xff0b0d0b));
    g.fillRoundedRectangle (chDisplay, 4.0f);
    g.setColour (juce::Colour (0xff3a3d44));
    g.drawRoundedRectangle (chDisplay.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (juce::Colour (0xff5be08a));
    g.setFont (titleFont (16.0f).withExtraKerningFactor (0.12f));
    g.drawText ("CH " + juce::String (processor.getChannel()).paddedLeft ('0', 3), chDisplay.reduced (8.0f, 0.0f), juce::Justification::centredLeft, false);
    g.setColour (juce::Colour (0xff3d8c5a));
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("DARK 4K", chDisplay.reduced (10.0f, 0.0f), juce::Justification::centredRight, false);

    drawMeters (g, inMeterBox, inLvl, inHold, "INPUT");
    drawMeters (g, outMeterBox, outLvl, outHold, "OUTPUT");

    drawFaderScale (g);
    g.setColour (juce::Colour (0xff2a2c31));
    g.setFont (titleFont (12.0f).withExtraKerningFactor (0.12f));
    g.drawText ("FADER", 375, 284, 250, 18, juce::Justification::centred);

    // lectura del fader
    const juce::Rectangle<float> rd (440.0f, 740.0f, 120.0f, 34.0f);
    g.setColour (juce::Colour (0xff0b0d0b));
    g.fillRoundedRectangle (rd, 4.0f);
    g.setColour (juce::Colour (0xff3a3d44));
    g.drawRoundedRectangle (rd.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (juce::Colour (0xff5be08a));
    g.setFont (numberFont (20.0f));
    const double lv = fader.getValue();
    g.drawText (lv <= -79.5 ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e dB")) : juce::String (lv, 1) + " dB", rd, juce::Justification::centred, false);

    g.setColour (juce::Colour (0xff6c7078));
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.3f));
    g.drawText ("DARK SHAPE STUDIOS", 750, 770, 250, 14, juce::Justification::centred);
}
