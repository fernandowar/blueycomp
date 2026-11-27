#include "PluginEditor.h"
#include "Dark4kFonts.h"

using namespace D4kColours;

namespace
{
    constexpr int kW = 948, kH = 879;
    constexpr float K = 1.5f;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }
    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::Michroma_ttf, Dark4kFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Font headFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::BarlowSemiBold_ttf, Dark4kFontData::BarlowSemiBold_ttfSize);
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
    juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (R * 0.45f), { 0, (int) (R * 0.14f) }).drawForPath (g, body);

    // falda estriada
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2e33), c.x - R * 0.5f, c.y - R * 0.7f,
                                             juce::Colour (0xff060607), c.x + R * 0.6f, c.y + R * 0.8f, false));
    g.fillPath (body);
    const int ridges = juce::jlimit (40, 120, (int) (R * 1.8f));
    for (int i = 0; i < ridges; ++i)
    {
        const float a = (float) i * 2.0f * pi / (float) ridges;
        const auto p1 = polar (c, a, R * 0.86f), p2 = polar (c, a, R * 0.995f);
        const float lit = 0.5f + 0.5f * std::cos (a + 0.8f);
        g.setColour (juce::Colour (0xff6a6d75).withAlpha (0.12f + 0.38f * lit));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, juce::jmax (0.9f, R * 0.018f));
    }

    // tapa
    const float cr = R * 0.80f;
    g.setColour (juce::Colours::black.withAlpha (0.9f));
    g.fillEllipse (c.x - cr - 1.5f, c.y - cr - 1.5f, (cr + 1.5f) * 2.0f, (cr + 1.5f) * 2.0f);

    const juce::Colour top = capCol.interpolatedWith (juce::Colours::white, coloured ? 0.28f : 0.35f);
    const juce::Colour bot = capCol.interpolatedWith (juce::Colours::black, coloured ? 0.55f : 0.45f);
    g.setGradientFill (juce::ColourGradient (top, c.x - cr * 0.55f, c.y - cr * 0.7f, bot, c.x + cr * 0.7f, c.y + cr * 0.9f, false));
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);

    // cara superior ligeramente hundida
    const float ir = cr * 0.72f;
    g.setGradientFill (juce::ColourGradient (bot.withAlpha (0.55f), c.x - ir * 0.6f, c.y - ir * 0.8f,
                                             top.withAlpha (0.45f), c.x + ir * 0.6f, c.y + ir * 0.8f, false));
    g.drawEllipse (c.x - ir, c.y - ir, ir * 2.0f, ir * 2.0f, 1.0f);

    // brillo de borde
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (coloured ? 0.40f : 0.60f), c.x - cr * 0.7f, c.y - cr * 0.7f,
                                             juce::Colours::transparentWhite, c.x + cr * 0.2f, c.y + cr * 0.2f, false));
    g.drawEllipse (c.x - cr + 0.8f, c.y - cr + 0.8f, cr * 2.0f - 1.6f, cr * 2.0f - 1.6f, 1.2f);

    // indicador
    const auto p1 = polar (c, angle, cr * 0.18f), p2 = polar (c, angle, cr * 0.97f);
    juce::Path ptr;
    ptr.startNewSubPath (p1);
    ptr.lineTo (p2);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (3.0f, R * 0.11f) + 1.5f, juce::PathStrokeType::curved, juce::PathStrokeType::butt),
                  juce::AffineTransform::translation (0.0f, 0.8f));
    g.setColour (coloured ? juce::Colour (0xfff6f6f8) : juce::Colour (0xff15161a));
    g.strokePath (ptr, juce::PathStrokeType (juce::jmax (2.6f, R * 0.095f), juce::PathStrokeType::curved, juce::PathStrokeType::butt));
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
    const juce::Rectangle<float> slot (cx - 3.0f, top - 8.0f, 6.0f, bottom - top + 16.0f);
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (slot.expanded (2.0f, 1.0f), 3.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), slot.getX(), 0.0f, juce::Colour (0xff2a2c31), slot.getRight(), 0.0f, false));
    g.fillRoundedRectangle (slot, 3.0f);

    // tapa: bloque plateado plano con estrías y línea central
    const float w = 36.0f, hh = (float) faderThumbHalf;
    const juce::Rectangle<float> capR (cx - w * 0.5f, sliderPos - hh, w, hh * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), 8, { 0, 5 }).drawForRectangle (g, capR.toNearestInt());
    g.setColour (juce::Colour (0xff17181b));
    g.fillRoundedRectangle (capR.expanded (1.0f), 2.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe7e8ea), capR.getX(), 0.0f, juce::Colour (0xffbfc1c6), capR.getRight(), 0.0f, false));
    g.fillRoundedRectangle (capR, 2.0f);
    // sombreado vertical (cara frontal curva)
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f), 0.0f, capR.getY(),
                                             juce::Colours::black.withAlpha (0.22f), 0.0f, capR.getBottom(), false));
    g.fillRoundedRectangle (capR, 2.0f);
    for (int i = 1; i < 6; ++i)
    {
        const float yy = capR.getY() + 4.0f + (capR.getHeight() - 8.0f) * (float) i / 6.0f;
        if (std::abs (yy - capR.getCentreY()) < 3.0f) continue;
        g.setColour (juce::Colours::black.withAlpha (0.28f));
        g.drawLine (capR.getX() + 3.0f, yy, capR.getRight() - 3.0f, yy, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawLine (capR.getX() + 3.0f, yy + 1.0f, capR.getRight() - 3.0f, yy + 1.0f, 1.0f);
    }
    g.setColour (juce::Colour (0xff0f1013));
    g.fillRect (capR.getX() + 2.0f, capR.getCentreY() - 1.2f, capR.getWidth() - 4.0f, 2.4f);
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
                  [this] (float v) { btnAuto.setToggleState (v > 0.5f, juce::dontSendNotification); })
{
    using M = std::vector<std::pair<double, juce::String>>;
    const M gainMarks { { -15, "-15" }, { -10, "" }, { -5, "-5" }, { 0, "0" }, { 5, "5" }, { 10, "" }, { 15, "15" } };
    const M qMarks { { 0.5, ".5" }, { 0.8, "" }, { 1.2, "1" }, { 2.0, "2" }, { 3.0, "3" } };

    addKnob ("hpf", "", "Hz", white, 16.0, { { 16, "OUT" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 350, "350" } });
    addKnob ("lpf", "", "kHz", white, 22000.0, { { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 22000, "OUT" } });
    addKnob ("hfg", "", "dB", red, 0.0, gainMarks);
    addKnob ("hff", "", "kHz", red, 8000.0, { { 1500, "1.5" }, { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 16000, "16" } });
    addKnob ("hmfg", "", "dB", green, 0.0, gainMarks);
    addKnob ("hmff", "", "kHz", green, 3000.0, { { 600, ".6" }, { 1000, "1" }, { 2000, "2" }, { 4000, "4" }, { 7000, "7" } });
    addKnob ("hmfq", "", "Q", green, 1.2, qMarks);
    addKnob ("lmfg", "", "dB", blue, 0.0, gainMarks);
    addKnob ("lmff", "", "kHz", blue, 800.0, { { 200, ".2" }, { 400, ".4" }, { 800, ".8" }, { 1500, "1.5" }, { 2500, "2.5" } });
    addKnob ("lmfq", "", "Q", blue, 1.2, qMarks);
    addKnob ("lfg", "", "dB", brown, 0.0, gainMarks);
    addKnob ("lff", "", "Hz", brown, 100.0, { { 30, "30" }, { 60, "60" }, { 100, "100" }, { 200, "200" }, { 450, "450" } });
    addKnob ("intrim", "IN TRIM", "dB", white, 0.0, { { -20, "-20" }, { -10, "" }, { 0, "0" }, { 10, "" }, { 20, "+20" } });
    addKnob ("threshold", "THRESHOLD", "dB", silver, 10.0,
             { { 10, "+10" }, { 5, "+5" }, { 0, "0" }, { -5, "-5" }, { -10, "-10" }, { -15, "-15" }, { -20, "-20" } });
    addKnob ("attack", "ATTACK", "ms", silver, 3.0, { { 0, "0.1" }, { 1, "0.3" }, { 2, "1" }, { 3, "3" }, { 4, "10" }, { 5, "30" } });
    addKnob ("makeup", "MAKEUP", "dB", silver, 0.0, { { 0, "0" }, { 3, "3" }, { 6, "6" }, { 9, "9" }, { 12, "12" }, { 15, "15" } });
    addKnob ("gatethr", "THRESHOLD", "dB", silver, -70.0, { { -70, "OFF" }, { -50, "-50" }, { -30, "-30" }, { -10, "-10" }, { 0, "0" } });
    addKnob ("gaterange", "RANGE", "dB", silver, 0.0, { { 0, "0" }, { 10, "10" }, { 20, "20" }, { 30, "30" }, { 40, "40" } });

    addToggle ("bypass", "PWR", juce::Colours::transparentBlack, true, true);
    addToggle ("phase", "\xc3\x98", juce::Colours::transparentBlack, true);
    addToggle ("filtin", "IN", juce::Colours::transparentBlack, true);
    addToggle ("eqin", "IN", juce::Colours::transparentBlack, true);
    addToggle ("hfbell", "BELL", juce::Colours::transparentBlack, true);
    addToggle ("lfbell", "BELL", juce::Colours::transparentBlack, true);
    addToggle ("dynin", "IN", juce::Colours::transparentBlack, true);

    btnAuto.setClickingTogglesState (false);
    btnAuto.onClick = [this] { releaseAtt.setValueAsCompleteGesture (btnAuto.getToggleState() ? 0.0f : 1.0f); };
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

    auto region = [&] (juce::Rectangle<int> r, juce::Colour top, juce::Colour bottom, float lineAlpha)
    {
        g.setGradientFill (juce::ColourGradient (top, 0.0f, (float) r.getY(), bottom, 0.0f, (float) r.getBottom(), false));
        g.fillRect (r);
        for (int y = r.getY(); y < r.getBottom(); ++y)
        {
            g.setColour (juce::Colours::black.withAlpha (rng.nextFloat() * lineAlpha));
            g.drawHorizontalLine (y, (float) r.getX(), (float) r.getRight());
        }
    };
    const int L = (int) (160 * K), R0 = (int) (472 * K), cL = (int) (238 * K), cR = (int) (395 * K);
    region ({ 0, 0, L, kH }, juce::Colour (0xff30343d), juce::Colour (0xff252830), 0.06f);
    region ({ R0, 0, kW - R0, kH }, juce::Colour (0xff30343d), juce::Colour (0xff252830), 0.06f);
    region ({ L, 0, cL - L, kH }, juce::Colour (0xff17191e), juce::Colour (0xff101216), 0.05f);
    region ({ cR, 0, R0 - cR, kH }, juce::Colour (0xff17191e), juce::Colour (0xff101216), 0.05f);
    region ({ cL, 0, cR - cL, (int) (185 * K) }, juce::Colour (0xff2c2f37), juce::Colour (0xff1f2128), 0.05f);
    region ({ cL, (int) (185 * K), cR - cL, kH - (int) (185 * K) }, juce::Colour (0xffc9cbcf), juce::Colour (0xffa9abb0), 0.07f);

    g.setColour (juce::Colour (0xff0b0c0f));
    for (int x : { L, cL, cR, R0 }) g.fillRect (x - 1, 0, 2, kH);
    g.fillRect (cL, (int) (185 * K) - 1, cR - cL, 3);

    // barra inferior
    region ({ 0, (int) (548 * K), kW, kH - (int) (548 * K) }, juce::Colour (0xff1b1d23), juce::Colour (0xff121419), 0.04f);
    g.setColour (juce::Colour (0xff0b0c0f));
    g.fillRect (0, (int) (548 * K) - 1, kW, 2);
}

void Dark4kAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a8d94), c.x - 3, c.y - 3, juce::Colour (0xff2b2d32), c.x + 4, c.y + 4, true));
    g.fillEllipse (c.x - 5, c.y - 5, 10, 10);
    g.setColour (juce::Colour (0xff15161a));
    g.drawLine (c.x - 3, c.y - 1.5f, c.x + 3, c.y + 1.5f, 1.4f);
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
    auto place = [this] (const char* id, float cx, float cy, float Rr)
    {
        for (auto& k : knobs)
            if (k->slider.getName() == id)
            {
                const float R = Rr * K;
                k->slider.setBounds (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre ({ cx * K, cy * K }).toNearestInt());
                const bool big = Rr >= 17.0f;
                k->scaleR = R + (big ? 12.0f : 9.0f);
                k->labelR = R + (big ? 24.0f : 20.0f);
                k->fontSize = big ? 11.0f : 10.0f;
            }
    };
    auto btn = [this] (const char* id, float x, float y, float w, float h)
    {
        for (auto& t : toggles)
            if (t->button->getName() == id)
                t->button->setBounds (juce::Rectangle<float> (x * K, y * K, w * K, h * K).toNearestInt());
    };

    // Izquierda: filtros + EQ (posiciones del layout de referencia)
    btn ("filtin", 98, 8, 36, 20);
    place ("hpf", 45, 44, 19);  place ("lpf", 115, 76, 19);
    btn ("hfbell", 96, 126, 40, 20);
    place ("hfg", 45, 150, 15); place ("hff", 115, 182, 16);
    place ("hmfg", 45, 232, 15); place ("hmff", 115, 262, 15); place ("hmfq", 45, 296, 15);
    btn ("eqin", 100, 318, 34, 20);
    place ("lmfg", 45, 368, 15); place ("lmff", 115, 404, 15); place ("lmfq", 45, 434, 15);
    place ("lff", 115, 474, 16); place ("lfg", 45, 512, 15);
    btn ("lfbell", 96, 519, 40, 20);

    // Centro
    btn ("bypass", 176, 6, 32, 22);
    chDisplay = { 220.0f * K, 6.0f * K, 196.0f * K, 22.0f * K };
    place ("intrim", 285, 76, 17);
    btn ("phase", 330, 60, 34, 22);
    fader.setBounds ((int) (300 * K), (int) (232 * K), (int) (70 * K), (int) (276 * K));
    inMeterBox  = { 247.0f, 98.0f, 100.0f, 706.0f };
    outMeterBox = { 603.0f, 98.0f, 100.0f, 706.0f };

    // Derecha
    place ("threshold", 517, 72, 17);
    btnAuto.setBounds (juce::Rectangle<float> (566 * K, 60 * K, 40 * K, 22 * K).toNearestInt());
    place ("attack", 585, 128, 17);
    place ("makeup", 517, 160, 15);
    btn ("dynin", 514, 208, 32, 22);
    place ("gatethr", 585, 232, 16);
    place ("gaterange", 517, 276, 16);
    ladderBox = { 556.0f * K, 276.0f * K, 62.0f * K, 76.0f * K };
    eqScreen = { 482.0f * K, 440.0f * K, 140.0f * K, 98.0f * K };
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
    for (auto& m : k.marks)
    {
        const float prop = (float) k.slider.valueToProportionOfLength (m.first);
        const float a = D4kLookAndFeel::startAngle + prop * (D4kLookAndFeel::endAngle - D4kLookAndFeel::startAngle);
        const auto p = polar (c, a, k.scaleR);
        g.setColour (dot);
        g.fillEllipse (p.x - 1.7f, p.y - 1.7f, 3.4f, 3.4f);
        if (m.second.isNotEmpty())
        {
            const auto q = polar (c, a, k.labelR);
            g.setColour (number);
            g.setFont (numberFont (k.fontSize));
            g.drawText (m.second, juce::Rectangle<float> (40.0f, 14.0f).withCentre (q), juce::Justification::centred, false);
        }
    }
    float y = c.y + k.labelR * 0.72f + 10.0f;
    if (k.title.isNotEmpty())
    {
        g.setColour (title);
        g.setFont (headFont (13.0f).withExtraKerningFactor (0.10f));
        g.drawText (k.title, juce::Rectangle<float> (110.0f, 16.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
        y += 14.0f;
    }
    if (k.unit.isNotEmpty())
    {
        g.setColour (grey);
        g.setFont (labelFont (11.0f));
        g.drawText (k.unit, juce::Rectangle<float> (110.0f, 14.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
    }
}

void Dark4kAudioProcessorEditor::drawMeters (juce::Graphics& g, juce::Rectangle<float> r, const float* levels, const float* holds, const juce::String& titleText)
{
    constexpr float lo = -60.0f;
    const bool scaleLeft = r.getX() < 400.0f;
    const float barW = 20.0f, gap = 4.0f;
    const float bx0 = scaleLeft ? r.getX() + 36.0f : r.getX() + 3.0f;
    const float yTop = r.getY(), h = r.getHeight();
    auto yOf = [&] (float db) { return yTop + h - (juce::jlimit (lo, 0.0f, db) - lo) / (0.0f - lo) * h; };

    g.setColour (grey);
    g.setFont (headFont (14.0f).withExtraKerningFactor (0.14f));
    g.drawText (titleText, juce::Rectangle<float> (bx0 - 10.0f, 54.0f, 2.0f * (barW + gap) + 20.0f, 18.0f), juce::Justification::centred, false);

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
        if (holds[ch] > lo)
        {
            g.setColour (juce::Colour (0xffffd9a8));
            g.fillRect (x + 1.0f, yOf (holds[ch]) - 1.0f, barW - 2.0f, 2.0f);
        }
        const bool clip = holds[ch] > -0.1f;
        g.setColour (clip ? juce::Colour (0xffff3a3a) : juce::Colour (0xff4a1418));
        g.fillRoundedRectangle (x, yTop - 14.0f, barW, 8.0f, 2.0f);
    }

    g.setFont (numberFont (13.0f));
    const float sx = scaleLeft ? r.getX() : bx0 + 2.0f * (barW + gap) + 3.0f;
    for (int v = 0; v <= 60; v += 6)
    {
        g.setColour (v <= 6 ? juce::Colour (0xffff4a4a) : (v <= 12 ? juce::Colour (0xffffb347) : juce::Colour (0xff3ddc6a)));
        g.drawText (juce::String (v), juce::Rectangle<float> (sx, yOf ((float) -v) - 8.0f, 34.0f, 16.0f),
                    scaleLeft ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
    }
}

void Dark4kAudioProcessorEditor::drawLadder (juce::Graphics& g, juce::Rectangle<float> box)
{
    g.setColour (juce::Colour (0xff10131c).withAlpha (0.65f));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (juce::Colour (0xff5b6684));
    g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.2f);

    struct Row { int db; juce::Colour col; };
    const Row rows[5] = { { 20, juce::Colour (0xffd62020) }, { 14, juce::Colour (0xffff3a3a) }, { 10, juce::Colour (0xffff9a2e) },
                          { 6, juce::Colour (0xffffb02e) }, { 3, juce::Colour (0xffffd23a) } };
    const float step = (box.getHeight() - 20.0f) / 4.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float y = box.getY() + 10.0f + step * (float) i;
        const float gx = box.getX() + 12.0f, rx = box.getRight() - 12.0f;
        g.setColour (juce::Colour (0xff3ddc6a));
        g.fillEllipse (gx - 4.0f, y - 4.0f, 8.0f, 8.0f);
        g.setColour (number);
        g.setFont (numberFont (12.0f));
        g.drawText (juce::String (rows[i].db), juce::Rectangle<float> (box.getCentreX() - 14.0f, y - 7.0f, 28.0f, 14.0f), juce::Justification::centred, false);
        const bool lit = grSmoothed >= (float) rows[i].db * 0.999f;
        if (lit) { g.setColour (rows[i].col.withAlpha (0.3f)); g.fillEllipse (rx - 7.0f, y - 7.0f, 14.0f, 14.0f); }
        g.setColour (lit ? rows[i].col : rows[i].col.withMultipliedBrightness (0.2f));
        g.fillEllipse (rx - 4.0f, y - 4.0f, 8.0f, 8.0f);
    }
}

void Dark4kAudioProcessorEditor::drawFaderScale (juce::Graphics& g)
{
    const auto b = fader.getBounds().toFloat();
    const float top = b.getY() + (float) D4kLookAndFeel::faderThumbHalf, bottom = b.getBottom() - (float) D4kLookAndFeel::faderThumbHalf;
    const float x0 = b.getCentreX() + 36.0f;
    g.setFont (numberFont (15.0f));
    struct T { double v; const char* t; };
    for (auto t : { T { 12, "12" }, T { 6, "6" }, T { 0, "0" }, T { -5, "5" }, T { -10, "10" }, T { -20, "20" }, T { -30, "30" },
                    T { -40, "40" }, T { -50, "50" }, T { -80, "\xe2\x88\x9e" } })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (t.v) * (bottom - top);
        g.setColour (juce::Colour (0xff2a2c31));
        g.drawLine (x0, y, x0 + 7.0f, y, 1.3f);
        g.drawText (juce::CharPointer_UTF8 (t.t), juce::Rectangle<float> (x0 + 11.0f, y - 9.0f, 34.0f, 18.0f), juce::Justification::centredLeft, false);
    }
}

void Dark4kAudioProcessorEditor::drawEqScreen (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colour (0xff0b1410));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (juce::Colour (0xff2c4a3a));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.4f);

    auto v = [this] (const char* id) { return processor.apvts.getRawParameterValue (id)->load(); };
    const double sr = 48000.0;
    d4k::EqVoicing vo; vo.black = true;
    d4k::Biquad hf, hmf, lmf, lf, hp, lp;
    const bool eqOn = v ("eqin") > 0.5f, fOn = v ("filtin") > 0.5f;
    const float gHf = v ("hfg"), gHm = v ("hmfg"), gLm = v ("lmfg"), gLf = v ("lfg");
    if (v ("hfbell") > 0.5f) hf.peak (sr, v ("hff"), gHf, vo.bellQ (1.0, gHf)); else hf.highShelf (sr, v ("hff"), gHf, vo.shelfQ());
    hmf.peak (sr, v ("hmff"), gHm, vo.bellQ (v ("hmfq"), gHm));
    lmf.peak (sr, v ("lmff"), gLm, vo.bellQ (v ("lmfq"), gLm));
    if (v ("lfbell") > 0.5f) lf.peak (sr, v ("lff"), gLf, vo.bellQ (1.0, gLf)); else lf.lowShelf (sr, v ("lff"), gLf, vo.shelfQ());
    const bool hpOn = v ("hpf") > 16.5f, lpOn = v ("lpf") < 21900.0f;
    if (hpOn) hp.highPass2 (sr, v ("hpf"), 0.707);
    if (lpOn) lp.lowPass2 (sr, v ("lpf"), 0.707);

    const auto inner = r.reduced (8.0f, 10.0f);
    g.setColour (juce::Colour (0xff1c3a2a));
    for (int i = 1; i < 4; ++i)
        g.drawHorizontalLine ((int) (inner.getY() + inner.getHeight() * (float) i / 4.0f), inner.getX(), inner.getRight());
    for (double f : { 100.0, 1000.0, 10000.0 })
    {
        const float x = inner.getX() + (float) (std::log (f / 20.0) / std::log (1000.0)) * inner.getWidth();
        g.drawVerticalLine ((int) x, inner.getY(), inner.getBottom());
    }
    g.setColour (juce::Colour (0xff2c6a48));
    g.drawHorizontalLine ((int) inner.getCentreY(), inner.getX(), inner.getRight());

    juce::Path p;
    for (int i = 0; i <= 160; ++i)
    {
        const double f = 20.0 * std::pow (1000.0, (double) i / 160.0);
        double db = 0.0;
        if (eqOn) db += hf.magDb (f, sr) + hmf.magDb (f, sr) + lmf.magDb (f, sr) + lf.magDb (f, sr);
        if (fOn)  { if (hpOn) db += hp.magDb (f, sr); if (lpOn) db += lp.magDb (f, sr); }
        const float y = inner.getCentreY() - (float) juce::jlimit (-20.0, 20.0, db) / 20.0f * inner.getHeight() * 0.5f;
        const float x = inner.getX() + (float) i / 160.0f * inner.getWidth();
        if (i == 0) p.startNewSubPath (x, y); else p.lineTo (x, y);
    }
    g.setColour (juce::Colour (0xff5be08a));
    g.strokePath (p, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved));
}

void Dark4kAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.drawImageAt (texture, 0, 0);

    for (auto c : { juce::Point<float> { 232, 810 }, { 716, 810 }, { 8, 810 }, { 940, 810 } }) juce::ignoreUnused (c);

    auto sectionLabel = [&] (const juce::String& t, float x, float y)
    {
        g.setColour (title);
        g.setFont (headFont (16.0f).withExtraKerningFactor (0.14f));
        g.drawText (t, juce::Rectangle<float> (x * K, y * K - 10.0f, 120.0f, 20.0f), juce::Justification::centredLeft, false);
    };
    auto hline = [&] (float y, float x0, float x1)
    {
        g.setColour (juce::Colour (0xff6c7078));
        g.drawLine (x0 * K, y * K, x1 * K, y * K, 1.4f);
    };

    // ---- Izquierda ----
    sectionLabel ("FILTERS", 8, 106); hline (118, 4, 156);
    sectionLabel ("HF", 8, 184);
    hline (204, 4, 156);
    sectionLabel ("HMF", 8, 266);
    hline (312, 4, 156);
    sectionLabel ("EQ", 8, 328); hline (342, 4, 156);
    sectionLabel ("LMF", 8, 404);
    hline (448, 4, 156);
    sectionLabel ("LF", 8, 470);

    // ---- Derecha ----
    sectionLabel ("COMPRESS", 478, 16);
    {
        const juce::Rectangle<float> d (562.0f * K, 6.0f * K, 46.0f * K, 20.0f * K);
        g.setColour (juce::Colour (0xff0b0d0b));
        g.fillRoundedRectangle (d, 4.0f);
        g.setColour (juce::Colour (0xff3a3d44));
        g.drawRoundedRectangle (d.reduced (0.5f), 4.0f, 1.2f);
        g.setColour (juce::Colour (0xff5be08a));
        g.setFont (numberFont (16.0f));
        g.drawText (juce::String (grSmoothed, 1) + " dB", d, juce::Justification::centred, false);
        g.setColour (grey);
        g.setFont (labelFont (11.0f));
        g.drawText ("GR", juce::Rectangle<float> (612.0f * K, 6.0f * K, 18.0f * K, 20.0f * K), juce::Justification::centred, false);
    }
    g.setColour (juce::Colour (0xff6c7078));
    sectionLabel ("DYN", 478, 220);
    sectionLabel ("GATE", 556, 362);
    hline (196, 476, 628);
    drawLadder (g, ladderBox);

    {
        const juce::Rectangle<float> badge (476.0f * K, 392.0f * K, 148.0f * K, 20.0f * K);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd3d5d9), 0.0f, badge.getY(), juce::Colour (0xff9fa2a8), 0.0f, badge.getBottom(), false));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (juce::Colour (0xff1c1d20));
        g.drawRoundedRectangle (badge, 3.0f, 1.0f);
        g.setFont (numberFont (13.0f).withExtraKerningFactor (0.08f));
        g.drawText ("DARK SHAPE \xe2\x80\xa2 4000 E", badge, juce::Justification::centred, false);
    }
    drawEqScreen (g, eqScreen);

    for (auto& k : knobs) drawKnobScale (g, *k);

    // ---- Centro ----
    {
        const auto& r = chDisplay;
        g.setColour (juce::Colour (0xff0b0d0b));
        g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xff3a3d44));
        g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.2f);
        g.setColour (juce::Colour (0xffe0343a));
        g.setFont (titleFont (17.0f).withExtraKerningFactor (0.1f));
        g.drawText (juce::String (processor.getChannel()).paddedLeft ('0', 3), r.withTrimmedLeft (10.0f), juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xff5be08a));
        g.setFont (titleFont (15.0f).withExtraKerningFactor (0.12f));
        g.drawText ("CHANNEL", r.withTrimmedLeft (74.0f), juce::Justification::centredLeft, false);
    }

    g.setColour (juce::Colours::white);
    g.setFont (titleFont (30.0f).withExtraKerningFactor (0.1f));
    g.drawText ("DARK 4K", juce::Rectangle<float> (cLx(), 188.0f, cW(), 40.0f), juce::Justification::centred, false);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.3f));
    g.drawText ("CHANNEL STRIP", juce::Rectangle<float> (cLx(), 230.0f, cW(), 16.0f), juce::Justification::centred, false);

    drawMeters (g, inMeterBox, inLvl, inHold, "INPUT");
    drawMeters (g, outMeterBox, outLvl, outHold, "OUTPUT");

    drawFaderScale (g);
    g.setColour (juce::Colour (0xff2a2c31));
    g.setFont (headFont (13.0f).withExtraKerningFactor (0.14f));
    g.drawText ("OUT", juce::Rectangle<float> (fader.getX() - 20.0f, fader.getY() - 22.0f, 110.0f, 16.0f), juce::Justification::centred, false);

    const juce::Rectangle<float> rd (cLx() + cW() * 0.5f - 52.0f, 770.0f, 104.0f, 34.0f);
    g.setColour (juce::Colour (0xff0b0d0b));
    g.fillRoundedRectangle (rd, 4.0f);
    g.setColour (juce::Colour (0xff3a3d44));
    g.drawRoundedRectangle (rd.reduced (0.5f), 4.0f, 1.2f);
    g.setColour (juce::Colour (0xff5be08a));
    g.setFont (numberFont (19.0f));
    const double lv = fader.getValue();
    g.drawText (lv <= -79.5 ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e dB")) : juce::String (lv, 1) + " dB", rd, juce::Justification::centred, false);

    // ---- Barra inferior ----
    g.setColour (juce::Colour (0xff8a8e97));
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.3f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, 822.0f, (float) kW, 57.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff5b5f68));
    g.setFont (labelFont (12.0f));
    g.drawText ("v1.0", juce::Rectangle<float> (12.0f, 822.0f, 100.0f, 57.0f), juce::Justification::centredLeft, false);
    g.drawText ("click CH = nuevo canal", juce::Rectangle<float> ((float) kW - 212.0f, 822.0f, 200.0f, 57.0f), juce::Justification::centredRight, false);
}
