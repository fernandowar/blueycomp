#include "PluginEditor.h"
#include "Dark4kFonts.h"

using namespace D4kColours;

namespace
{
    constexpr int kW = 960, kH = 910;
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
    // Cilindro visto desde arriba/adelante: tapa mate de color con aro plateado,
    // cuerpo de aluminio estriado debajo, base negra. Indicador blanco grueso y plano.
    const float angle = startAngle + proportion * (endAngle - startAngle);
    const bool coloured = capCol != silver && capCol != white;

    const float cr = R * 0.80f;                       // radio de la tapa
    const float h  = R * 0.26f;                       // altura visible del cuerpo
    const juce::Point<float> cc (c.x, c.y - R * 0.12f);   // centro de la tapa
    const float botY = cc.y + h;

    // sombra proyectada sobre la chapa
    juce::Path shadowP;
    shadowP.addEllipse (cc.x - cr, botY - cr, cr * 2.0f, cr * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), (int) (R * 0.45f), { (int) (R * 0.10f), (int) (R * 0.18f) }).drawForPath (g, shadowP);

    // base negra
    g.setColour (juce::Colour (0xff050506));
    g.fillEllipse (cc.x - cr * 1.0f, botY - cr * 0.98f + R * 0.05f, cr * 2.0f, cr * 2.0f);

    // cuerpo: rectangulo + elipse inferior, relleno cilindrico
    juce::Path body;
    body.addEllipse (cc.x - cr, botY - cr, cr * 2.0f, cr * 2.0f);
    body.addRectangle (cc.x - cr, cc.y, cr * 2.0f, h);
    {
        juce::ColourGradient cg (juce::Colour (0xff3a3c40), cc.x - cr, 0.0f, juce::Colour (0xff3a3c40), cc.x + cr, 0.0f, false);
        cg.addColour (0.18, juce::Colour (0xffd0d2d4));
        cg.addColour (0.42, juce::Colour (0xff9a9c9e));
        cg.addColour (0.75, juce::Colour (0xff6c6e72));
        g.setGradientFill (cg);
        g.fillPath (body);
    }
    // estrias verticales (solo en la franja visible del cuerpo)
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path vis;
        vis.addRectangle (cc.x - cr, cc.y, cr * 2.0f, h + cr);
        juce::Path clipP;
        clipP.addEllipse (cc.x - cr, botY - cr, cr * 2.0f, cr * 2.0f);
        clipP.addRectangle (cc.x - cr, cc.y, cr * 2.0f, h);
        g.reduceClipRegion (juce::Rectangle<int> ((int) (cc.x - cr), (int) cc.y, (int) (cr * 2.0f) + 1, (int) h + 2));
        for (int i = -5; i <= 5; ++i)
        {
            const float x = cc.x + std::sin ((float) i * 0.30f) * cr;
            g.setColour (juce::Colours::black.withAlpha (0.20f));
            g.drawLine (x, cc.y, x, cc.y + h + R * 0.2f, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.14f));
            g.drawLine (x + 1.2f, cc.y, x + 1.2f, cc.y + h + R * 0.2f, 1.0f);
        }
        // linea negra de referencia (visible cuando el indicador apunta hacia el frente)
        const float front = -std::cos (angle);
        if (front > 0.05f)
        {
            g.setColour (juce::Colours::black.withAlpha (0.85f * front));
            const float x = cc.x + std::sin (angle) * cr;
            g.drawLine (x, cc.y, x, cc.y + h + R * 0.3f, juce::jmax (1.4f, R * 0.07f));
        }
    }
    // oscurecer el borde inferior del cuerpo
    {
        juce::ColourGradient dg (juce::Colours::transparentBlack, 0.0f, botY - R * 0.1f,
                                 juce::Colours::black.withAlpha (0.55f), 0.0f, botY + cr * 0.55f, false);
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path lower;
        lower.addEllipse (cc.x - cr, botY - cr, cr * 2.0f, cr * 2.0f);
        g.reduceClipRegion (juce::Rectangle<int> ((int) (cc.x - cr) - 1, (int) botY, (int) (cr * 2.0f) + 2, (int) (cr + 2.0f)));
        g.setGradientFill (dg);
        g.fillPath (lower);
    }

    // aro plateado de la tapa
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff2f3f4), cc.x - cr, cc.y - cr,
                                             juce::Colour (0xff8d9094), cc.x + cr, cc.y + cr, false));
    g.fillEllipse (cc.x - cr, cc.y - cr, cr * 2.0f, cr * 2.0f);

    // tapa mate plana
    const float fr = cr * 0.92f;
    const juce::Colour top = capCol.interpolatedWith (juce::Colours::white, coloured ? 0.08f : 0.10f);
    const juce::Colour bot = capCol.interpolatedWith (juce::Colours::black, coloured ? 0.20f : 0.12f);
    g.setGradientFill (juce::ColourGradient (top, cc.x, cc.y - fr, bot, cc.x, cc.y + fr, false));
    g.fillEllipse (cc.x - fr, cc.y - fr, fr * 2.0f, fr * 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.35f));
    g.drawEllipse (cc.x - fr, cc.y - fr, fr * 2.0f, fr * 2.0f, 0.8f);

    // indicador: barra plana gruesa, del centro al borde
    const float pw = juce::jmax (2.6f, R * 0.13f);
    juce::Path ptr;
    ptr.addRectangle (-pw * 0.5f, -fr * 0.97f, pw, fr * 0.97f - fr * 0.10f);
    const auto xf = juce::AffineTransform::rotation (angle).translated (cc);
    g.setColour (juce::Colours::black.withAlpha (0.30f));
    g.fillPath (ptr, xf.translated (0.0f, R * 0.03f));
    g.setColour (coloured ? juce::Colour (0xfff2f2f2) : juce::Colour (0xff111214));
    g.fillPath (ptr, xf);
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
void D4kButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat().reduced (2.0f);
    const bool on = getToggleState();

    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 6, { 0, 2 }).drawForRectangle (g, b.toNearestInt());
    // bisel oscuro
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff34363c), b.getX(), b.getY(), juce::Colour (0xff08090b), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 5.0f);
    g.setColour (juce::Colour (0xff5a5d66).withAlpha (0.55f));
    g.drawRoundedRectangle (b.reduced (0.6f), 5.0f, 1.0f);

    // lente
    auto lens = b.reduced (juce::jmax (3.0f, b.getHeight() * 0.13f));
    if (on)
    {
        g.setColour (juce::Colour (0xffff9a3a).withAlpha (0.22f));
        g.fillRoundedRectangle (lens.expanded (3.0f), 5.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffffdca6), lens.getX(), lens.getY(), juce::Colour (0xffe9862a), lens.getX(), lens.getBottom(), false));
    }
    else
    {
        juce::Colour t (0xffc8cbd1), bt (0xff878a92);
        if (down) { t = t.darker (0.2f); bt = bt.darker (0.2f); }
        else if (over) { t = t.brighter (0.08f); bt = bt.brighter (0.08f); }
        g.setGradientFill (juce::ColourGradient (t, lens.getX(), lens.getY(), bt, lens.getX(), lens.getBottom(), false));
    }
    g.fillRoundedRectangle (lens, 3.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (on ? 0.45f : 0.38f), lens.getX(), lens.getY(),
                                             juce::Colours::transparentWhite, lens.getX(), lens.getCentreY(), false));
    g.fillRoundedRectangle (lens.withHeight (lens.getHeight() * 0.5f), 3.0f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.drawRoundedRectangle (lens.reduced (0.5f), 3.0f, 1.0f);

    g.setColour (on ? juce::Colour (0xff3a1d05) : juce::Colour (0xff1b1c20));
    g.setFont (headFont (lens.getHeight() > 20.0f ? 16.0f : 13.5f).withExtraKerningFactor (0.06f));
    g.drawText (getButtonText(), lens, juce::Justification::centred, false);
}

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
    const M qMarks { { 3.0, "3" }, { 2.5, "" }, { 2.0, "2" }, { 1.75, "" }, { 1.5, "1.5" }, { 1.25, "" }, { 1.0, "1" }, { 0.75, "" }, { 0.5, ".5" } };

    addKnob ("hpf", "", "Hz", white, 16.0, { { 16, "OUT" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 350, "350" } });
    addKnob ("lpf", "", "kHz", white, 22000.0, { { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 22000, "OUT" } });
    addKnob ("hfg", "", "dB", red, 0.0, gainMarks);
    addKnob ("hff", "", "kHz", red, 8000.0, { { 1500, "1.5" }, { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 16000, "16" } });
    addKnob ("hmfg", "", "dB", green, 0.0, gainMarks);
    addKnob ("hmff", "", "kHz", green, 3000.0, { { 600, ".6" }, { 1000, "1" }, { 2000, "2" }, { 4000, "4" }, { 7000, "7" } });
    addKnob ("hmfq", "", "Q", green, 1.5, qMarks);
    addKnob ("lmfg", "", "dB", blue, 0.0, gainMarks);
    addKnob ("lmff", "", "kHz", blue, 800.0, { { 200, ".2" }, { 400, ".4" }, { 800, ".8" }, { 1500, "1.5" }, { 2500, "2.6" } });
    addKnob ("lmfq", "", "Q", blue, 1.5, qMarks);
    addKnob ("lfg", "", "dB", brown, 0.0, gainMarks);
    addKnob ("lff", "", "Hz", brown, 100.0, { { 30, "30" }, { 60, "60" }, { 100, "100" }, { 200, "200" }, { 450, "450" } });
    addKnob ("intrim", "IN TRIM", "dB", white, 0.0, { { -20, "-20" }, { -10, "" }, { 0, "0" }, { 10, "" }, { 20, "+20" } });
    addKnob ("threshold", "THRESHOLD", "dB", silver, 10.0,
             { { 10, "+10" }, { 5, "+5" }, { 0, "0" }, { -5, "-5" }, { -10, "-10" }, { -15, "-15" }, { -20, "-20" } });
    addKnob ("attack", "ATTACK", "ms", silver, 3.0, { { 0, "0.1" }, { 1, "0.3" }, { 2, "1" }, { 3, "3" }, { 4, "10" }, { 5, "30" } });
    addKnob ("gatethr", "THRESHOLD", "dB", silver, -70.0, { { -70, "OFF" }, { -50, "-50" }, { -30, "-30" }, { -10, "-10" }, { 0, "0" } });
    addKnob ("gaterange", "RANGE", "dB", silver, 0.0, { { 0, "0" }, { 10, "10" }, { 20, "20" }, { 30, "30" }, { 40, "40" } });

    const juce::Colour none = juce::Colours::transparentBlack;
    addToggle ("bypass", "PWR", none, true, true);
    addToggle ("phase", juce::String::fromUTF8 ("\xc3\x98"), none, true);
    addToggle ("filtin", "IN", none, true);
    addToggle ("eqin", "IN", none, true);
    addToggle ("hfbell", "BELL", none, true);
    addToggle ("lfbell", "BELL", none, true);
    addToggle ("dynin", "IN", none, true);

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

namespace Lay
{
    constexpr int leftW = 250, rightX = 710, meterInX = 250, cX = 360, cR = 600;
    constexpr int darkH = 268, barY = 864;
}

void Dark4kAudioProcessorEditor::buildTexture()
{
    texture = juce::Image (juce::Image::RGB, kW, kH, false);
    juce::Graphics g (texture);
    juce::Random rng (4200);

    auto brushed = [&] (juce::Rectangle<int> r, juce::Colour top, juce::Colour bottom, float grain, bool light)
    {
        g.setGradientFill (juce::ColourGradient (top, 0.0f, (float) r.getY(), bottom, 0.0f, (float) r.getBottom(), false));
        g.fillRect (r);
        for (int y = r.getY(); y < r.getBottom(); ++y)
        {
            const float a = rng.nextFloat();
            g.setColour ((a > 0.5f ? juce::Colours::white : juce::Colours::black).withAlpha (std::abs (a - 0.5f) * grain));
            g.drawHorizontalLine (y, (float) r.getX(), (float) r.getRight());
        }
        for (int i = 0; i < r.getWidth() * r.getHeight() / 900; ++i)
        {
            const float x = (float) r.getX() + rng.nextFloat() * (float) r.getWidth(), yy = (float) r.getY() + rng.nextFloat() * (float) r.getHeight();
            const float len = 10.0f + rng.nextFloat() * 70.0f;
            g.setColour ((light ? juce::Colours::black : juce::Colours::white).withAlpha (0.012f + rng.nextFloat() * 0.03f));
            g.drawLine (x, yy, juce::jmin (x + len, (float) r.getRight()), yy, 1.0f);
        }
    };
    using namespace Lay;
    brushed ({ 0, 0, leftW, barY }, juce::Colour (0xff343841), juce::Colour (0xff272a31), 0.10f, false);
    brushed ({ rightX, 0, kW - rightX, barY }, juce::Colour (0xff343841), juce::Colour (0xff272a31), 0.10f, false);
    brushed ({ meterInX, 0, cX - meterInX, barY }, juce::Colour (0xff1a1c21), juce::Colour (0xff111317), 0.06f, false);
    brushed ({ cR, 0, rightX - cR, barY }, juce::Colour (0xff1a1c21), juce::Colour (0xff111317), 0.06f, false);
    brushed ({ cX, 0, cR - cX, darkH }, juce::Colour (0xff2e3139), juce::Colour (0xff1f2128), 0.08f, false);
    brushed ({ cX, darkH, cR - cX, barY - darkH }, juce::Colour (0xffcbcdd1), juce::Colour (0xffadafb4), 0.10f, true);

    for (int x : { leftW, cX, cR, rightX })
    {
        g.setColour (juce::Colour (0xff07080a)); g.fillRect (x - 2, 0, 3, barY);
        g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRect (x + 1, 0, 1, barY);
    }
    g.setColour (juce::Colour (0xff07080a)); g.fillRect (cX, darkH - 2, cR - cX, 3);
    g.setColour (juce::Colours::white.withAlpha (0.12f)); g.fillRect (cX, darkH + 1, cR - cX, 1);

    // sombra interior junto a cada ranura (profundidad)
    for (int x : { leftW, cR })
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), (float) x - 2.0f, 0.0f, juce::Colours::transparentBlack, (float) x - 22.0f, 0.0f, false));
        g.fillRect (x - 22, 0, 20, barY);
    }
    for (int x : { cX, rightX })
    {
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.35f), (float) x + 2.0f, 0.0f, juce::Colours::transparentBlack, (float) x + 22.0f, 0.0f, false));
        g.fillRect (x + 2, 0, 20, barY);
    }

    brushed ({ 0, barY, kW, kH - barY }, juce::Colour (0xff1e2026), juce::Colour (0xff121419), 0.05f, false);
    g.setColour (juce::Colour (0xff07080a)); g.fillRect (0, barY - 2, kW, 3);
    g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRect (0, barY + 1, kW, 1);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRect (0, 0, kW, kH, 1);
}

void Dark4kAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    juce::Path p; p.addEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 4, { 0, 1 }).drawForPath (g, p);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff9a9da5), c.x - 4, c.y - 4, juce::Colour (0xff2a2c31), c.x + 5, c.y + 5, true));
    g.fillEllipse (c.x - 6.0f, c.y - 6.0f, 12.0f, 12.0f);
    g.setColour (juce::Colour (0xff0e0f12));
    g.drawLine (c.x - 4.0f, c.y - 1.8f, c.x + 4.0f, c.y + 1.8f, 1.7f);
    g.setColour (juce::Colours::white.withAlpha (0.25f));
    g.drawLine (c.x - 4.0f, c.y - 0.8f, c.x + 4.0f, c.y + 2.8f, 0.8f);
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
    auto place = [this] (const char* id, float cx, float cy, float R0)
    {
        const float R = R0 * 0.88f;   // tamano general de las perillas
        for (auto& k : knobs)
            if (k->slider.getName() == id)
            {
                k->slider.setBounds (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre ({ cx, cy }).toNearestInt());
                const bool big = R0 >= 30.0f;
                k->scaleR = R + (big ? 9.0f : 8.0f);
                k->labelR = R + (big ? 20.0f : 17.0f);
                k->fontSize = big ? 11.0f : 10.0f;
            }
    };
    auto btn = [this] (const char* id, int x, int y, int w, int h)
    {
        for (auto& t : toggles)
            if (t->button->getName() == id) t->button->setBounds (x, y, w, h);
    };
    using namespace Lay;

    // Izquierda: filtros + EQ
    btn ("filtin", 172, 16, 62, 30);
    place ("hpf", 76, 80, 27);  place ("lpf", 176, 124, 27);
    btn ("hfbell", 172, 196, 62, 28);
    place ("hfg", 74, 244, 21); place ("hff", 176, 282, 21);
    place ("hmfg", 74, 384, 21); place ("hmff", 176, 422, 21); place ("hmfq", 74, 478, 21);
    btn ("eqin", 108, 538, 64, 30);
    place ("lmfg", 74, 626, 21); place ("lmff", 176, 664, 21); place ("lmfq", 74, 720, 21);
    place ("lff", 176, 782, 21); place ("lfg", 74, 822, 21);
    btn ("lfbell", 172, 826, 62, 28);

    // Centro
    btn ("bypass", 266, 14, 58, 32);
    chDisplay = { 372.0f, 14.0f, 216.0f, 34.0f };
    place ("intrim", 450, 120, 34);
    btn ("phase", 526, 98, 52, 40);
    fader.setBounds (cX + 80, 296, 80, 500);
    inMeterBox  = { 262.0f, 100.0f, 90.0f, 744.0f };
    outMeterBox = { 609.0f, 100.0f, 90.0f, 744.0f };

    // Derecha: Dark VCA + puerta
    place ("threshold", 796, 190, 36);
    btnAuto.setBounds (872, 96, 70, 34);
    place ("attack", 884, 318, 32);
    btn ("dynin", 800, 412, 62, 34);
    place ("gatethr", 880, 512, 30);
    place ("gaterange", 776, 592, 28);
    ladderBox = { 836.0f, 618.0f, 108.0f, 94.0f };
    eqScreen = { 724.0f, 764.0f, 222.0f, 84.0f };
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

    // repintado parcial: los knobs (costosos) solo se redibujan cuando se mueven
    repaint (inMeterBox.expanded (6.0f, 50.0f).getSmallestIntegerContainer());
    repaint (outMeterBox.expanded (6.0f, 50.0f).getSmallestIntegerContainer());
    repaint (ladderBox.expanded (2.0f).getSmallestIntegerContainer());
    repaint (juce::Rectangle<int> (856, 8, 100, 38));
    repaint (eqScreen.expanded (2.0f).getSmallestIntegerContainer());
    repaint (juce::Rectangle<int> (420, 800, 120, 50));
}


void Dark4kAudioProcessorEditor::drawKnobScale (juce::Graphics& g, Knob& k)
{
    const auto c = k.slider.getBounds().toFloat().getCentre();
    for (auto& m : k.marks)
    {
        const float prop = (float) k.slider.valueToProportionOfLength (m.first);
        const float a = D4kLookAndFeel::startAngle + prop * (D4kLookAndFeel::endAngle - D4kLookAndFeel::startAngle);
        const auto p = polar (c, a, k.scaleR);
        g.setColour (juce::Colour (0xffe9eaee));
        g.fillEllipse (p.x - 1.7f, p.y - 1.7f, 3.4f, 3.4f);
        if (m.second.isNotEmpty())
        {
            const auto q = polar (c, a, k.labelR);
            g.setColour (juce::Colour (0xffc3c6cd));
            g.setFont (numberFont (k.fontSize));
            g.drawText (m.second, juce::Rectangle<float> (40.0f, 14.0f).withCentre (q), juce::Justification::centred, false);
        }
    }
    float y = c.y + k.labelR * 0.72f + 9.0f;
    if (k.title.isNotEmpty())
    {
        g.setColour (juce::Colour (0xfff2f3f5));
        g.setFont (headFont (13.0f).withExtraKerningFactor (0.12f));
        g.drawText (k.title, juce::Rectangle<float> (110.0f, 16.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
        y += 13.0f;
    }
    if (k.unit.isNotEmpty())
    {
        g.setColour (juce::Colour (0xff7e828b));
        g.setFont (labelFont (10.5f));
        g.drawText (k.unit, juce::Rectangle<float> (60.0f, 13.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
    }
}

void Dark4kAudioProcessorEditor::drawMeters (juce::Graphics& g, juce::Rectangle<float> r, const float* levels, const float* holds, const juce::String& titleText)
{
    constexpr float lo = -60.0f;
    const bool scaleLeft = r.getX() < 400.0f;
    const float barW = 22.0f, gap = 4.0f;
    const float bx0 = scaleLeft ? r.getX() + 36.0f : r.getX() + 3.0f;
    const float yTop = r.getY(), h = r.getHeight();
    auto yOf = [&] (float db) { return yTop + h - (juce::jlimit (lo, 0.0f, db) - lo) / (0.0f - lo) * h; };

    g.setColour (juce::Colour (0xff9a9ea8));
    g.setFont (headFont (14.0f).withExtraKerningFactor (0.16f));
    g.drawText (titleText, juce::Rectangle<float> (bx0 - 10.0f, yTop - 44.0f, 2.0f * (barW + gap) + 20.0f, 18.0f), juce::Justification::centred, false);

    // ventana de vidrio / bisel
    const juce::Rectangle<float> win (bx0 - 5.0f, yTop - 22.0f, 2.0f * barW + gap + 10.0f, h + 28.0f);
    g.setColour (juce::Colour (0xff050608));
    g.fillRoundedRectangle (win, 4.0f);
    g.setColour (juce::Colour (0xff3a3d45));
    g.drawRoundedRectangle (win.reduced (0.5f), 4.0f, 1.0f);

    for (int ch = 0; ch < 2; ++ch)
    {
        const float x = bx0 + (float) ch * (barW + gap);
        g.setColour (juce::Colour (0xff0d0e11));
        g.fillRect (x, yTop, barW, h);
        const int segs = 74;
        const float segH = h / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float segDb = lo + ((float) i + 0.5f) / (float) segs * (0.0f - lo);
            const float t = juce::jlimit (0.0f, 1.0f, (segDb - lo) / 60.0f);
            const auto col = juce::Colour (0xffd0600e).interpolatedWith (juce::Colour (0xffffa042), t);
            const bool lit = segDb < levels[ch];
            g.setColour (lit ? col : col.withMultipliedBrightness (0.13f));
            g.fillRect (x + 1.5f, yTop + h - segH * (float) (i + 1) + 0.6f, barW - 3.0f, segH - 1.2f);
        }
        if (holds[ch] > lo)
        {
            g.setColour (juce::Colour (0xffffe2bd));
            g.fillRect (x + 1.5f, yOf (holds[ch]) - 1.0f, barW - 3.0f, 2.0f);
        }
        const bool clip = holds[ch] > -0.1f;
        const juce::Rectangle<float> led (x + 2.0f, yTop - 16.0f, barW - 4.0f, 9.0f);
        if (clip) { g.setColour (juce::Colour (0xffff3a3a).withAlpha (0.35f)); g.fillRoundedRectangle (led.expanded (3.0f), 4.0f); }
        g.setColour (clip ? juce::Colour (0xffff4040) : juce::Colour (0xff4a1418));
        g.fillRoundedRectangle (led, 2.0f);
    }
    // reflejo del vidrio
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), win.getX(), 0.0f, juce::Colours::transparentWhite, win.getX() + win.getWidth() * 0.6f, 0.0f, false));
    g.fillRoundedRectangle (win.reduced (1.0f), 4.0f);

    g.setFont (numberFont (13.0f));
    const float sx = scaleLeft ? r.getX() : bx0 + 2.0f * (barW + gap) + 5.0f;
    for (int v = 0; v <= 60; v += 6)
    {
        g.setColour (v <= 6 ? juce::Colour (0xffff5a5a) : (v <= 12 ? juce::Colour (0xffffb347) : juce::Colour (0xff4fd97a)));
        g.drawText (juce::String (v), juce::Rectangle<float> (sx, yOf ((float) -v) - 8.0f, 30.0f, 16.0f),
                    scaleLeft ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
    }
}

void Dark4kAudioProcessorEditor::drawLadder (juce::Graphics& g, juce::Rectangle<float> box)
{
    g.setColour (juce::Colour (0xff07080a));
    g.fillRoundedRectangle (box, 5.0f);
    g.setColour (juce::Colour (0xff4a4e58));
    g.drawRoundedRectangle (box.reduced (0.5f), 5.0f, 1.0f);

    struct Row { int db; juce::Colour col; };
    const Row rows[5] = { { 20, juce::Colour (0xffd62020) }, { 14, juce::Colour (0xffff3a3a) }, { 10, juce::Colour (0xffff9a2e) },
                          { 6, juce::Colour (0xffffb02e) }, { 3, juce::Colour (0xffffd23a) } };
    const float step = (box.getHeight() - 22.0f) / 4.0f;
    for (int i = 0; i < 5; ++i)
    {
        const float y = box.getY() + 11.0f + step * (float) i;
        const float gx = box.getX() + 14.0f, rx = box.getRight() - 14.0f;
        g.setColour (juce::Colour (0xff3ddc6a).withAlpha (0.25f)); g.fillEllipse (gx - 6.0f, y - 6.0f, 12.0f, 12.0f);
        g.setColour (juce::Colour (0xff3ddc6a)); g.fillEllipse (gx - 3.5f, y - 3.5f, 7.0f, 7.0f);
        g.setColour (juce::Colour (0xffc3c6cd));
        g.setFont (numberFont (12.0f));
        g.drawText (juce::String (rows[i].db), juce::Rectangle<float> (box.getCentreX() - 14.0f, y - 7.0f, 28.0f, 14.0f), juce::Justification::centred, false);
        const bool lit = grSmoothed >= (float) rows[i].db * 0.999f;
        if (lit) { g.setColour (rows[i].col.withAlpha (0.35f)); g.fillEllipse (rx - 7.0f, y - 7.0f, 14.0f, 14.0f); }
        g.setColour (lit ? rows[i].col : rows[i].col.withMultipliedBrightness (0.2f));
        g.fillEllipse (rx - 3.5f, y - 3.5f, 7.0f, 7.0f);
    }
}

void Dark4kAudioProcessorEditor::drawFaderScale (juce::Graphics& g)
{
    const auto b = fader.getBounds().toFloat();
    const float top = b.getY() + (float) D4kLookAndFeel::faderThumbHalf, bottom = b.getBottom() - (float) D4kLookAndFeel::faderThumbHalf;
    const float cx = b.getCentreX();
    struct T { double v; const char* t; };
    // marcas menores
    for (double v : { 10.0, 8.0, 4.0, 2.0, -2.5, -7.5, -15.0, -25.0, -35.0, -45.0 })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (v) * (bottom - top);
        g.setColour (juce::Colour (0xff4a4c52));
        g.drawLine (cx - 30.0f, y, cx - 25.0f, y, 1.0f);
        g.drawLine (cx + 25.0f, y, cx + 30.0f, y, 1.0f);
    }
    g.setFont (numberFont (15.0f));
    for (auto t : { T { 12, "12" }, T { 6, "6" }, T { 0, "0" }, T { -5, "5" }, T { -10, "10" }, T { -20, "20" }, T { -30, "30" },
                    T { -40, "40" }, T { -50, "50" }, T { -80, "\xe2\x88\x9e" } })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (t.v) * (bottom - top);
        g.setColour (juce::Colour (0xff23252a));
        g.drawLine (cx - 36.0f, y, cx - 25.0f, y, 1.4f);
        g.drawLine (cx + 25.0f, y, cx + 36.0f, y, 1.4f);
        g.drawText (juce::CharPointer_UTF8 (t.t), juce::Rectangle<float> (cx + 40.0f, y - 9.0f, 34.0f, 18.0f), juce::Justification::centredLeft, false);
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
    if (v ("hfbell") > 0.5f) hf.peak (sr, v ("hff"), vo.bellGain (gHf), vo.bellQFixed());
    else                      hf.highShelf (sr, vo.hfShelfFreq (v ("hff")), vo.shelfGain (gHf), vo.shelfQHf());
    hmf.peak (sr, vo.hmfFreq (v ("hmff")), vo.midGain (gHm, v ("hmfq")), vo.bellQ (v ("hmfq")));
    lmf.peak (sr, vo.lmfFreq (v ("lmff")), vo.midGain (gLm, v ("lmfq")), vo.bellQ (v ("lmfq"), true));
    if (v ("lfbell") > 0.5f) lf.peak (sr, v ("lff"), vo.bellGain (gLf), vo.bellQFixed());
    else                      lf.lowShelf (sr, vo.lfShelfFreq (v ("lff")), vo.shelfGain (gLf), vo.shelfQLf());
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
    using namespace Lay;
    g.drawImageAt (texture, 0, 0);

    // tornillos de rack
    for (float y : { 14.0f, 604.0f, 850.0f })
    {
        drawScrew (g, { 12.0f, y });
        drawScrew (g, { (float) kW - 12.0f, y });
    }
    for (auto c : { juce::Point<float> { (float) cX + 14.0f, 14.0f }, { (float) cR - 14.0f, 14.0f },
                    { (float) cX + 14.0f, (float) barY - 14.0f }, { (float) cR - 14.0f, (float) barY - 14.0f } })
        drawScrew (g, c);

    // utilidades de rotulado
    auto groove = [&] (float y, float x0, float x1)
    {
        g.setColour (juce::Colour (0xff0a0b0d)); g.drawLine (x0, y, x1, y, 1.4f);
        g.setColour (juce::Colours::white.withAlpha (0.18f)); g.drawLine (x0, y + 1.5f, x1, y + 1.5f, 1.0f);
    };
    auto label = [&] (const juce::String& t, float x, float y, float size = 15.0f)
    {
        g.setColour (juce::Colour (0xfff2f3f5));
        g.setFont (headFont (size).withExtraKerningFactor (0.14f));
        g.drawText (t, juce::Rectangle<float> (x, y - 11.0f, 150.0f, 22.0f), juce::Justification::centredLeft, false);
    };
    // llave tipo consola: linea con quiebre diagonal
    auto bracket = [&] (float y, float x0, float x1, float rise)
    {
        juce::Path p;
        p.startNewSubPath (x0, y);
        p.lineTo (x0 + 70.0f, y);
        p.lineTo (x0 + 70.0f + rise, y - rise);
        p.lineTo (x1, y - rise);
        g.setColour (juce::Colour (0xff0a0b0d)); g.strokePath (p, juce::PathStrokeType (1.4f));
        g.setColour (juce::Colours::white.withAlpha (0.20f)); g.strokePath (p, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.6f));
    };

    // ---- Izquierda ----
    label ("FILTERS", 14.0f, 176.0f);
    groove (190.0f, 10.0f, 240.0f);
    label ("HF", 14.0f, 300.0f);
    bracket (338.0f, 10.0f, 240.0f, 0.0f);
    label ("HMF", 14.0f, 444.0f);
    groove (530.0f, 10.0f, 240.0f);
    label ("EQ", 14.0f, 553.0f);
    groove (578.0f, 10.0f, 240.0f);
    label ("LMF", 14.0f, 692.0f);
    label ("LF", 14.0f, 790.0f);

    // ---- Derecha ----
    label ("COMPRESS", 726.0f, 30.0f, 16.0f);
    {
        const juce::Rectangle<float> d (860.0f, 12.0f, 70.0f, 30.0f);
        g.setColour (juce::Colour (0xff050806)); g.fillRoundedRectangle (d, 4.0f);
        g.setColour (juce::Colour (0xff3a3d45)); g.drawRoundedRectangle (d.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (juce::Colour (0xff4fe08a));
        g.setFont (numberFont (17.0f));
        g.drawText (juce::String (grSmoothed, 1) + " dB", d, juce::Justification::centred, false);
        g.setColour (juce::Colour (0xff7e828b)); g.setFont (labelFont (10.0f));
        g.drawText ("GR", juce::Rectangle<float> (934.0f, 12.0f, 24.0f, 30.0f), juce::Justification::centred, false);
    }
    groove (56.0f, 722.0f, 948.0f);
    groove (390.0f, 722.0f, 948.0f);
    label ("DYN", 726.0f, 429.0f, 16.0f);
    label ("GATE", 726.0f, 470.0f);
    drawLadder (g, ladderBox);

    {
        const juce::Rectangle<float> badge (724.0f, 724.0f, 222.0f, 26.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (badge.translated (0.0f, 2.0f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8dadd), 0.0f, badge.getY(), juce::Colour (0xffa4a7ad), 0.0f, badge.getBottom(), false));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (juce::Colour (0xff1c1d20)); g.drawRoundedRectangle (badge, 3.0f, 1.0f);
        g.setFont (headFont (14.0f).withExtraKerningFactor (0.12f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("DARK SHAPE \xe2\x80\xa2 4000 E")), badge, juce::Justification::centred, false);
    }
    drawEqScreen (g, eqScreen);

    for (auto& k : knobs) drawKnobScale (g, *k);

    // ---- Centro ----
    {
        const auto& r = chDisplay;
        g.setColour (juce::Colour (0xff050806)); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xff3a3d45)); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        g.setColour (juce::Colour (0xffe0343a));
        g.setFont (titleFont (18.0f).withExtraKerningFactor (0.1f));
        g.drawText (juce::String (processor.getChannel()).paddedLeft ('0', 3), r.withTrimmedLeft (12.0f), juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xff4fe08a));
        g.setFont (titleFont (14.0f).withExtraKerningFactor (0.14f));
        g.drawText ("CHANNEL", r.withTrimmedLeft (84.0f), juce::Justification::centredLeft, false);
    }

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.setFont (titleFont (31.0f).withExtraKerningFactor (0.1f));
    g.drawText ("DARK 4K", juce::Rectangle<float> ((float) cX, 214.0f, (float) (cR - cX), 40.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white);
    g.drawText ("DARK 4K", juce::Rectangle<float> ((float) cX, 212.0f, (float) (cR - cX), 40.0f), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff8d919a));
    g.setFont (headFont (12.5f).withExtraKerningFactor (0.36f));
    g.drawText ("CHANNEL STRIP", juce::Rectangle<float> ((float) cX, 248.0f, (float) (cR - cX), 16.0f), juce::Justification::centred, false);

    drawMeters (g, inMeterBox, inLvl, inHold, "INPUT");
    drawMeters (g, outMeterBox, outLvl, outHold, "OUTPUT");

    drawFaderScale (g);
    g.setColour (juce::Colour (0xff3a3c42));
    g.setFont (headFont (13.0f).withExtraKerningFactor (0.2f));
    g.drawText ("OUT", juce::Rectangle<float> ((float) cX, 284.0f, (float) (cR - cX), 16.0f), juce::Justification::centred, false);

    const juce::Rectangle<float> rd ((float) (cX + cR) * 0.5f - 54.0f, 808.0f, 108.0f, 36.0f);
    g.setColour (juce::Colour (0xff050806)); g.fillRoundedRectangle (rd, 4.0f);
    g.setColour (juce::Colour (0xff3a3d45)); g.drawRoundedRectangle (rd.reduced (0.5f), 4.0f, 1.0f);
    g.setColour (juce::Colour (0xff4fe08a));
    g.setFont (numberFont (20.0f));
    const double lv = fader.getValue();
    g.drawText (lv <= -79.5 ? juce::String (juce::CharPointer_UTF8 ("-\xe2\x88\x9e dB")) : juce::String (lv, 1) + " dB", rd, juce::Justification::centred, false);

    // ---- Barra inferior ----
    g.setColour (juce::Colour (0xff8a8e97));
    g.setFont (headFont (13.0f).withExtraKerningFactor (0.34f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, (float) barY, (float) kW, (float) (kH - barY)), juce::Justification::centred, false);
    g.setColour (juce::Colour (0xff5b5f68));
    g.setFont (labelFont (12.0f));
    g.drawText ("v2.6", juce::Rectangle<float> (16.0f, (float) barY, 100.0f, (float) (kH - barY)), juce::Justification::centredLeft, false);
    g.drawText ("click CH = nuevo canal", juce::Rectangle<float> ((float) kW - 216.0f, (float) barY, 200.0f, (float) (kH - barY)), juce::Justification::centredRight, false);
}
