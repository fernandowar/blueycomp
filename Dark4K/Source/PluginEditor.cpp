#include "PluginEditor.h"
#include "Dark4kFonts.h"

using namespace D4kColours;

namespace
{
    constexpr int kW = 960, kH = 940;
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
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::LiberationSans_Regular_ttf, Dark4kFontData::LiberationSans_Regular_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::LiberationSans_Bold_ttf, Dark4kFontData::LiberationSans_Bold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (Dark4kFontData::LiberationSans_Regular_ttf, Dark4kFontData::LiberationSans_Regular_ttfSize);
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
                                       float sliderPos, float, float, juce::Slider::SliderStyle, juce::Slider& s)
{
    if (s.isHorizontal())
    {
        const float cy = (float) y + (float) height * 0.5f;
        const float x0 = (float) x + 6.0f, x1 = (float) (x + width) - 6.0f;
        const juce::Rectangle<float> slot (x0 - 4.0f, cy - 2.5f, x1 - x0 + 8.0f, 5.0f);
        g.setColour (juce::Colour (0xff050506)); g.fillRoundedRectangle (slot.expanded (1.5f, 1.0f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), 0.0f, slot.getY(), juce::Colour (0xff2a2c31), 0.0f, slot.getBottom(), false));
        g.fillRoundedRectangle (slot, 2.0f);
        const float w = 11.0f, hh = 5.5f;
        const juce::Rectangle<float> capR (sliderPos - w * 0.5f, cy - hh, w, hh * 2.0f);
        juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 3, { 0, 2 }).drawForRectangle (g, capR.toNearestInt());
        g.setColour (juce::Colour (0xff0b0c0e)); g.fillRoundedRectangle (capR.expanded (1.0f), 3.5f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff80838a), 0.0f, capR.getY(), juce::Colour (0xff1b1c20), 0.0f, capR.getBottom(), false));
        g.fillRoundedRectangle (capR, 3.0f);
        const auto face = capR.reduced (1.2f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4d54), 0.0f, face.getY(), juce::Colour (0xff22242a), 0.0f, face.getBottom(), false));
        g.fillRoundedRectangle (face, 1.5f);
        g.setColour (juce::Colours::white.withAlpha (0.10f)); g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.42f), 2.0f);
        g.setColour (juce::Colour (0xfff4f4f6));
        g.fillRoundedRectangle (sliderPos - 0.6f, face.getY() + 1.5f, 1.2f, face.getHeight() - 3.0f, 0.6f);
        return;
    }
    const float cx = (float) x + (float) width * 0.5f;
    const float top = (float) y + (float) faderThumbHalf, bottom = (float) (y + height - faderThumbHalf);

    // ranura
    const juce::Rectangle<float> slot (cx - 3.0f, top - 8.0f, 6.0f, bottom - top + 16.0f);
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (slot.expanded (2.0f, 1.0f), 3.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff000000), slot.getX(), 0.0f, juce::Colour (0xff2a2c31), slot.getRight(), 0.0f, false));
    g.fillRoundedRectangle (slot, 3.0f);

    // tapa moderna: aluminio grafito con aro plateado, estrias y linea blanca (mismo lenguaje que las perillas)
    const float w = 40.0f, hh = (float) faderThumbHalf;
    const juce::Rectangle<float> capR (cx - w * 0.5f, sliderPos - hh, w, hh * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.65f), 9, { 0, 6 }).drawForRectangle (g, capR.toNearestInt());
    g.setColour (juce::Colour (0xff0b0c0e));
    g.fillRoundedRectangle (capR.expanded (1.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff80838a), 0.0f, capR.getY(), juce::Colour (0xff1b1c20), 0.0f, capR.getBottom(), false));
    g.fillRoundedRectangle (capR, 4.5f);
    const auto face = capR.reduced (2.5f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4d54), 0.0f, face.getY(), juce::Colour (0xff22242a), 0.0f, face.getBottom(), false));
    g.fillRoundedRectangle (face, 3.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.42f), 3.0f);
    for (int i = 0; i < 4; ++i)
    {
        const float off = 4.0f + 3.2f * (float) i;
        for (float yy : { face.getY() + off, face.getBottom() - off })
        {
            g.setColour (juce::Colours::black.withAlpha (0.55f));
            g.drawLine (face.getX() + 4.0f, yy, face.getRight() - 4.0f, yy, 1.0f);
            g.setColour (juce::Colours::white.withAlpha (0.12f));
            g.drawLine (face.getX() + 4.0f, yy + 1.0f, face.getRight() - 4.0f, yy + 1.0f, 1.0f);
        }
    }
    g.setColour (juce::Colours::white.withAlpha (0.20f));
    g.fillRect (face.getX() + 2.0f, capR.getCentreY() - 2.6f, face.getWidth() - 4.0f, 5.2f);
    g.setColour (juce::Colour (0xfff4f4f6));
    g.fillRoundedRectangle (face.getX() + 3.0f, capR.getCentreY() - 1.3f, face.getWidth() - 6.0f, 2.6f, 1.0f);
}

//==============================================================================
void D4kButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    if ((bool) getProperties().getWithDefault ("ssl", false))
    {
        // tecla fisica estilo SSL: pozo oscuro + tecla con cara superior y canto lateral
        auto well = getLocalBounds().toFloat().reduced (2.0f);
        g.setColour (juce::Colour (0xff0b0b0d).withAlpha (0.55f));
        g.fillRoundedRectangle (well, 7.0f);
        const float lift = down ? 1.0f : 2.5f;                 // altura de la tecla
        auto face = well.reduced (5.0f);
        face.setHeight (face.getHeight() - 2.5f);
        face = face.translated (0.0f, 2.5f - lift);
        // sombra proyectada dentro del pozo
        juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 7, { 0, (int) lift + 1 }).drawForRectangle (g, face.toNearestInt());
        // canto lateral (parte baja de la tecla)
        auto side = face.withHeight (face.getHeight() + lift);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a4b50), 0.0f, face.getCentreY(), juce::Colour (0xff232427), 0.0f, side.getBottom(), false));
        g.fillRoundedRectangle (side, 5.5f);
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawRoundedRectangle (side.reduced (0.5f), 5.5f, 1.0f);
        // cara superior
        const auto top = over ? juce::Colour (0xff9a9b9f) : juce::Colour (0xff909195);
        const auto bot = over ? juce::Colour (0xff808186) : juce::Colour (0xff77787d);
        g.setGradientFill (juce::ColourGradient (top, 0.0f, face.getY(), bot, 0.0f, face.getBottom(), false));
        g.fillRoundedRectangle (face, 5.0f);
        {
            juce::Graphics::ScopedSaveState ss (g);
            juce::Path clip; clip.addRoundedRectangle (face, 5.0f);
            g.reduceClipRegion (clip);
            for (float yy = face.getY() + 1.0f; yy < face.getBottom(); yy += 2.0f)
            {
                g.setColour (juce::Colours::black.withAlpha (0.03f));
                g.drawHorizontalLine ((int) yy, face.getX(), face.getRight());
            }
            // brillo suave superior y leve oscurecimiento en los bordes (cara ligeramente curva)
            g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.14f), 0.0f, face.getY(),
                                                     juce::Colours::transparentWhite, 0.0f, face.getY() + face.getHeight() * 0.45f, false));
            g.fillRect (face);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.07f), face.getX(), 0.0f,
                                                     juce::Colours::transparentBlack, face.getX() + 9.0f, 0.0f, false));
            g.fillRect (face);
            g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.07f), face.getRight(), 0.0f,
                                                     juce::Colours::transparentBlack, face.getRight() - 9.0f, 0.0f, false));
            g.fillRect (face);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawHorizontalLine ((int) (face.getY() + 0.5f), face.getX() + 5.0f, face.getRight() - 5.0f);
        g.setColour (juce::Colours::white.withAlpha (0.22f));
        g.drawRoundedRectangle (face.reduced (0.5f), 5.0f, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawHorizontalLine ((int) (face.getBottom() - 1.0f), face.getX() + 4.0f, face.getRight() - 4.0f);
        const auto r = face;
        // letra grabada
        g.setFont (numberFont (14.0f).withHorizontalScale (0.92f).withExtraKerningFactor (0.03f));
        g.setColour (juce::Colours::white.withAlpha (0.30f));
        g.drawText (getButtonText(), r.translated (0.0f, 1.0f), juce::Justification::centred, false);
        g.setColour (juce::Colour (0xff3a3b3f));
        g.drawText (getButtonText(), r, juce::Justification::centred, false);
        return;
    }
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

    if ((bool) getProperties().getWithDefault ("ref", false))
    {
        g.setColour (on ? juce::Colour (0xff4a2208) : juce::Colour (0xff4b4e55));
        g.setFont (headFont (lens.getHeight() > 24.0f ? 14.0f : 11.0f).withHorizontalScale (0.88f));
        g.drawText (getButtonText(), lens, juce::Justification::centred, false);
        return;
    }
    g.setColour (on ? juce::Colour (0xff3a1d05) : juce::Colour (0xff1b1c20));
    g.setFont (headFont (lens.getHeight() > 20.0f ? 16.0f : 13.5f).withExtraKerningFactor (0.02f));
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
    : AudioProcessorEditor (&p), processor (p)
{
    using M = std::vector<std::pair<double, juce::String>>;
    const M gainMarks { { -15, "-15" }, { -12, "12" }, { -9, "9" }, { -6, "6" }, { -3, "3" }, { 0, "0" },
                        { 3, "3" }, { 6, "6" }, { 9, "9" }, { 12, "12" }, { 15, "+15" } };
    const M qMarks { { 3.0, "3" }, { 2.5, "" }, { 2.0, "2" }, { 1.75, "" }, { 1.5, "1.5" }, { 1.25, "" }, { 1.0, "1" }, { 0.75, "" }, { 0.5, ".5" } };

    addKnob ("hpf", "", "Hz", white, 16.0, { { 16, "OUT" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 350, "350" } });
    addKnob ("lpf", "", "kHz", white, 22000.0, { { 3000, "3" }, { 5000, "5" }, { 8000, "8" }, { 12000, "12" }, { 22000, "OUT" } });
    addKnob ("hfg", "", "dB", red, 0.0, gainMarks);
    addKnob ("hff", "", "kHz", red, 8000.0, { { 1500, "1.5" }, { 2000, "2" }, { 5000, "5" }, { 8000, "8" }, { 10000, "10" }, { 14000, "14" }, { 16000, "16" } });
    addKnob ("hmfg", "", "dB", green, 0.0, gainMarks);
    addKnob ("hmff", "", "kHz", green, 3000.0, { { 600, ".6" }, { 800, ".8" }, { 1500, "1.5" }, { 3000, "3" }, { 4500, "4.5" }, { 6000, "6" }, { 7000, "7" } });
    addKnob ("hmfq", "", "Q", green, 1.5, qMarks);
    addKnob ("lmfg", "", "dB", blue, 0.0, gainMarks);
    addKnob ("lmff", "", "kHz", blue, 800.0, { { 200, ".2" }, { 300, ".3" }, { 800, ".8" }, { 1000, "1" }, { 1500, "1.5" }, { 2000, "2" }, { 2500, "2.5" } });
    addKnob ("lmfq", "", "Q", blue, 1.5, qMarks);
    addKnob ("lfg", "", "dB", brown, 0.0, gainMarks);
    addKnob ("lff", "", "Hz", brown, 100.0, { { 30, "30" }, { 50, "50" }, { 100, "100" }, { 200, "200" }, { 300, "300" }, { 400, "400" }, { 450, "450" } });
    addKnob ("intrim", "IN TRIM", "dB", white, 0.0, { { -20, "-20" }, { -10, "" }, { 0, "0" }, { 10, "" }, { 20, "+20" } });
    addKnob ("ratio", "RATIO", "", silver, 3.0, { { 1, "1" }, { 2, "2" }, { 3, "3" }, { 5, "5" }, { 8, "8" }, { 20, "20" }, { 100, juce::String::fromUTF8 ("\xe2\x88\x9e") } });
    addKnob ("threshold", "THRESHOLD", "", silver, 10.0,
             { { 10, "+10" }, { 5, "+5" }, { 0, "0" }, { -5, "-5" }, { -10, "-10" }, { -15, "-15" }, { -20, "-20" } });
    addKnob ("release", "RELEASE", "", silver, 0.4, { { 0.1, ".1" }, { 0.2, ".2" }, { 0.4, ".4" }, { 0.8, ".8" }, { 1.2, "1.2" } });
    addKnob ("gatethr", "THRESHOLD", "", green, -70.0, { { -70, "OFF" }, { -50, "-50" }, { -30, "-30" }, { -10, "-10" }, { 0, "0" } });
    addKnob ("gaterange", "RANGE", "", green, 0.0, { { 0, "0" }, { 10, "10" }, { 20, "20" }, { 30, "30" }, { 40, "40" } });
    addKnob ("gaterel", "RELEASE", "", green, 0.2, { { 0.1, ".1" }, { 0.2, ".2" }, { 0.4, ".4" }, { 1, "1" }, { 2, "2" }, { 4, "4" } });

    const juce::Colour none = juce::Colours::transparentBlack;
    addToggle ("bypass", "PWR", none, true, true);
    addToggle ("solo", "SOLO", none, true).getProperties().set ("ref", true);
    addToggle ("cut", "CUT", none, true).getProperties().set ("ref", true);
    addToggle ("phase", juce::String::fromUTF8 ("\xc3\x98"), none, true);
    addToggle ("filtin", "IN", none, true);
    addToggle ("eqin", "IN", none, true);
    addToggle ("hfbell", "BELL", none, true);
    addToggle ("lfbell", "BELL", none, true);
    addToggle ("dynin", "IN", none, true);

    attackBtn.setClickingTogglesState (false);
    attackBtn.setName ("attack");
    attackBtn.getProperties().set ("ssl", true);
    addAndMakeVisible (attackBtn);
    {
        auto* param = processor.apvts.getParameter ("attack");
        attackAtt = std::make_unique<juce::ParameterAttachment> (*param, [this] (float v)
        {
            const int idx = juce::jlimit (0, 2, (int) std::lround (v));
            static const char* names[3] = { "FAST", "MED", "SLOW" };
            attackBtn.setButtonText (names[idx]);
            attackBtn.repaint();
        });
        attackBtn.onClick = [this]
        {
            const int cur = juce::jlimit (0, 2, (int) std::lround (processor.apvts.getParameter ("attack")->convertFrom0to1 (processor.apvts.getParameter ("attack")->getValue())));
            attackAtt->setValueAsCompleteGesture ((float) ((cur + 1) % 3));
        };
        attackAtt->sendInitialUpdate();
    }

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
    constexpr int darkH = 268, barY = 894;
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
    auto place = [this] (const char* id, float cx, float cy, float R0, float mul = 1.20f)
    {
        const float R = R0 * mul;   // tamano general de las perillas
        for (auto& k : knobs)
            if (k->slider.getName() == id)
            {
                k->slider.setBounds (juce::Rectangle<float> (R * 2.0f, R * 2.0f).withCentre ({ cx, cy }).toNearestInt());
                const bool big = R0 >= 30.0f;
                k->scaleR = R * 1.18f + (big ? 3.0f : 0.0f);
                k->labelR = R * 1.50f + (big ? 5.0f : 0.0f);
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
    // Posiciones copiadas de la referencia (Waves SSL), escaladas a esta ventana
    btn ("filtin", 150, 12, 60, 30);
    place ("lpf", 68, 68, 21, 1.30f);  place ("hpf", 180, 112, 21, 1.30f);
    btn ("hfbell", 150, 208, 60, 28);
    place ("hfg", 68, 240, 21, 1.30f); place ("hff", 179, 298, 21, 1.30f);
    place ("hmfg", 68, 367, 21, 1.30f); place ("hmff", 179, 422, 21, 1.30f); place ("hmfq", 68, 472, 21, 1.30f);
    btn ("eqin", 127, 520, 64, 30);
    place ("lmfg", 68, 593, 21, 1.30f); place ("lmff", 179, 636, 21, 1.30f); place ("lmfq", 68, 698, 21, 1.30f);
    place ("lff", 179, 778, 21, 1.30f); place ("lfg", 68, 827, 21, 1.30f);
    btn ("lfbell", 150, 840, 60, 28);

    // Centro
    btn ("bypass", 266, 14, 58, 32);
    chDisplay = { 372.0f, 27.0f, 216.0f, 32.0f };
    place ("intrim", 437, 108, 23, 1.30f);
    btn ("phase", 504, 86, 56, 36);
    btn ("solo", 400, 284, 68, 62);
    btn ("cut", 492, 284, 68, 62);
    fader.setBounds (cX + 80, 372, 80, 430);
    inMeterBox  = { 262.0f, 100.0f, 90.0f, 774.0f };
    outMeterBox = { 609.0f, 100.0f, 90.0f, 774.0f };

    // Derecha: compresor + puerta (posiciones copiadas de la referencia)
    attackBtn.setBounds (846, 80, 88, 46);
    place ("ratio", 780, 118, 21, 1.50f);
    place ("threshold", 890, 194, 21, 1.50f);
    place ("release", 780, 258, 21, 1.50f);
    btn ("dynin", 766, 366, 36, 50);
    place ("gatethr", 890, 393, 21, 1.50f);
    place ("gaterange", 781, 478, 21, 1.50f);
    place ("gaterel", 781, 596, 21, 1.50f);
    ladderBox = { 846.0f, 494.0f, 98.0f, 92.0f };
    eqScreen = { 724.0f, 764.0f, 222.0f, 100.0f };
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
    repaint (juce::Rectangle<int> (420, 828, 120, 48));
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
            g.setFont (headFont (k.fontSize + 1.0f));
            g.drawText (m.second, juce::Rectangle<float> (40.0f, 14.0f).withCentre (q), juce::Justification::centred, false);
        }
    }
    float y = c.y + k.labelR * 0.95f + 9.0f;
    if (k.title.isNotEmpty())
    {
        g.setColour (juce::Colour (0xfff2f3f5));
        g.setFont (headFont (11.5f).withExtraKerningFactor (0.03f));
        g.drawText (k.title, juce::Rectangle<float> (110.0f, 16.0f).withCentre ({ c.x, y }), juce::Justification::centred, false);
        y += 13.0f;
    }
    if (k.unit == "Q")
    {
        // iconos de campana: estrecha (izq) y ancha (der)
        auto bell = [&] (float cx, float cy, float halfW, float hgt, float sig)
        {
            juce::Path p;
            for (int i = 0; i <= 24; ++i)
            {
                const float t = -1.0f + 2.0f * (float) i / 24.0f;
                const float px = cx + t * halfW, py = cy - hgt * std::exp (-(t * t) / (sig * sig));
                if (i == 0) p.startNewSubPath (px, py); else p.lineTo (px, py);
            }
            g.setColour (juce::Colour (0xffc3c6cd));
            g.strokePath (p, juce::PathStrokeType (1.2f));
        };
        const float iy = c.y + k.labelR * 0.95f + 8.0f;
        bell (c.x - k.labelR * 0.55f, iy, 6.0f, 6.0f, 0.35f);
        bell (c.x + k.labelR * 0.55f, iy, 8.0f, 5.0f, 0.75f);
    }
    else if (k.unit.isNotEmpty())
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
    g.setFont (headFont (15.0f).withExtraKerningFactor (0.04f));
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
    for (double v : { 10.0, 8.0, 4.0, 2.0, -2.5, -7.5, -15.0, -25.0, -35.0, -45.0 })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (v) * (bottom - top);
        g.setColour (juce::Colour (0xff6b6e75));
        g.drawLine (cx - 31.0f, y, cx - 26.0f, y, 1.0f);
        g.drawLine (cx + 26.0f, y, cx + 31.0f, y, 1.0f);
    }
    g.setFont (headFont (17.0f));
    for (auto t : { T { 12, "12" }, T { 6, "6" }, T { 0, "0" }, T { -5, "5" }, T { -10, "10" }, T { -20, "20" }, T { -30, "30" },
                    T { -40, "40" }, T { -50, "50" }, T { -80, "\xe2\x88\x9e" } })
    {
        const float y = bottom - (float) fader.valueToProportionOfLength (t.v) * (bottom - top);
        g.setColour (juce::Colour (0xff3a3c42));
        g.drawLine (cx - 38.0f, y, cx - 26.0f, y, 1.4f);
        g.drawLine (cx + 26.0f, y, cx + 38.0f, y, 1.4f);
        g.setColour (juce::Colour (0xff34363c));
        g.drawText (juce::CharPointer_UTF8 (t.t), juce::Rectangle<float> (cx + 42.0f, y - 10.0f, 40.0f, 20.0f), juce::Justification::centredLeft, false);
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
    for (float y : { 14.0f, 872.0f })
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
    auto label = [&] (const juce::String& t, float x, float y, float size = 14.0f)
    {
        g.setColour (size >= 16.0f ? juce::Colour (0xfff2f3f5) : juce::Colour (0xffd9dce2));
        g.setFont (headFont (size).withExtraKerningFactor (0.02f));
        g.drawText (t, juce::Rectangle<float> (x, y - 11.0f, 150.0f, 22.0f), juce::Justification::centredLeft, false);
    };
    // ---- Izquierda ----
    // lineas escalonadas como la referencia: izquierda -> derecha con quiebre diagonal
    auto stepLine = [&] (float yL, float yR, float xA, float xB)
    {
        juce::Path p;
        p.startNewSubPath (10.0f, yL); p.lineTo (xA, yL); p.lineTo (xB, yR); p.lineTo (240.0f, yR);
        g.setColour (juce::Colour (0xff0a0b0d)); g.strokePath (p, juce::PathStrokeType (1.4f));
        g.setColour (juce::Colours::white.withAlpha (0.20f)); g.strokePath (p, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.6f));
    };
    label ("FILTERS", 10.0f, 174.0f, 18.0f);
    groove (191.0f, 6.0f, 244.0f);
    label ("HF", 10.0f, 300.0f);
    stepLine (315.0f, 359.0f, 117.0f, 142.0f);
    label ("HMF", 10.0f, 430.0f);
    label ("EQ", 10.0f, 537.0f, 18.0f);
    groove (537.0f, 50.0f, 122.0f);
    label ("LMF", 10.0f, 654.0f);
    stepLine (765.0f, 723.0f, 108.0f, 136.0f);
    label ("LF", 10.0f, 792.0f);

    // ---- Derecha ----
    label ("COMPRESS", 726.0f, 30.0f, 17.0f);
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
    // DYN + llave que abarca el gate (como la referencia)
    label ("DYN", 719.0f, 391.0f, 20.0f);
    {
        juce::Path p;
        p.startNewSubPath (806.0f, 391.0f); p.lineTo (824.0f, 391.0f); p.lineTo (863.0f, 337.0f);
        p.lineTo (951.0f, 337.0f); p.lineTo (951.0f, 539.0f); p.lineTo (941.0f, 539.0f);
        g.setColour (juce::Colour (0xff0a0b0d)); g.strokePath (p, juce::PathStrokeType (1.4f));
        g.setColour (juce::Colours::white.withAlpha (0.20f));
        g.strokePath (p, juce::PathStrokeType (1.0f), juce::AffineTransform::translation (0.0f, 1.6f));
    }
    label ("GATE", 758.0f, 684.0f, 15.0f);
    drawLadder (g, ladderBox);

    {
        const juce::Rectangle<float> badge (722.0f, 722.0f, 226.0f, 30.0f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.fillRoundedRectangle (badge.translated (0.0f, 2.0f), 3.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffd8dadd), 0.0f, badge.getY(), juce::Colour (0xffa4a7ad), 0.0f, badge.getBottom(), false));
        g.fillRoundedRectangle (badge, 3.0f);
        g.setColour (juce::Colour (0xff1c1d20)); g.drawRoundedRectangle (badge, 3.0f, 1.0f);
        g.setFont (headFont (14.0f).withExtraKerningFactor (0.06f));
        g.drawText (juce::String (juce::CharPointer_UTF8 ("DARK SHAPE \xe2\x80\xa2 4000 E")), badge, juce::Justification::centred, false);
    }
    drawEqScreen (g, eqScreen);

    for (auto& k : knobs) drawKnobScale (g, *k);

    // ---- Centro ----
    {
        const auto& r = chDisplay;
        g.setColour (juce::Colour (0xff050806)); g.fillRoundedRectangle (r, 4.0f);
        g.setColour (juce::Colour (0xff3a3d45)); g.drawRoundedRectangle (r.reduced (0.5f), 4.0f, 1.0f);
        const juce::String num = juce::String (processor.getChannel()).paddedLeft ('0', 2);
        const juce::Font numF = numberFont (24.0f).withExtraKerningFactor (0.06f);
        const juce::Font txtF = numberFont (16.0f).withExtraKerningFactor (0.16f);
        auto widthOf = [] (const juce::Font& f, const juce::String& s)
        { juce::GlyphArrangement ga; ga.addLineOfText (f, s, 0.0f, 0.0f); return ga.getBoundingBox (0, -1, true).getWidth(); };
        const float wn = widthOf (numF, num), wt = widthOf (txtF, "CHANNEL"), gap = 16.0f;
        const float x0 = r.getCentreX() - (wn + gap + wt) * 0.5f;
        g.setColour (juce::Colour (0xffe0343a)); g.setFont (numF);
        g.drawText (num, juce::Rectangle<float> (x0 - 2.0f, r.getY(), wn + 6.0f, r.getHeight()), juce::Justification::centredLeft, false);
        g.setColour (juce::Colour (0xff4fe08a)); g.setFont (txtF);
        g.drawText ("CHANNEL", juce::Rectangle<float> (x0 + wn + gap, r.getY(), wt + 8.0f, r.getHeight()), juce::Justification::centredLeft, false);
    }

    // placa del logo (hundida en el panel oscuro)
    {
        const juce::Rectangle<float> pl ((float) cX + 12.0f, 188.0f, (float) (cR - cX) - 24.0f, 72.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (pl.translated (0.0f, 1.5f).expanded (1.0f), 7.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff111317), 0.0f, pl.getY(), juce::Colour (0xff1c1f25), 0.0f, pl.getBottom(), false));
        g.fillRoundedRectangle (pl, 6.0f);
        g.setColour (juce::Colour (0xff050608)); g.drawRoundedRectangle (pl.reduced (0.5f), 6.0f, 1.2f);
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawLine (pl.getX() + 6.0f, pl.getBottom() + 1.0f, pl.getRight() - 6.0f, pl.getBottom() + 1.0f, 1.0f);
        g.setFont (titleFont (27.0f).withExtraKerningFactor (0.1f));
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText ("DARK 4K", pl.withHeight (44.0f).translated (0.0f, 8.0f), juce::Justification::centred, false);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff4f5f7), 0.0f, pl.getY() + 10.0f, juce::Colour (0xff9fa3ac), 0.0f, pl.getY() + 50.0f, false));
        g.drawText ("DARK 4K", pl.withHeight (44.0f).translated (0.0f, 6.0f), juce::Justification::centred, false);
        g.setColour (juce::Colour (0xff4fe08a).withAlpha (0.55f));
        g.fillRect (pl.getCentreX() - 56.0f, pl.getY() + 51.0f, 112.0f, 1.0f);
        g.setColour (juce::Colour (0xff8d919a));
        g.setFont (headFont (11.5f).withExtraKerningFactor (0.30f));
        g.drawText ("CHANNEL STRIP", pl.withTrimmedTop (53.0f), juce::Justification::centred, false);
    }

    drawMeters (g, inMeterBox, inLvl, inHold, "INPUT");
    drawMeters (g, outMeterBox, outLvl, outHold, "OUTPUT");

    drawFaderScale (g);
g.setColour (juce::Colour (0xff3a3c42));
    g.setFont (headFont (13.0f));
    g.drawText ("OUT TRIM", juce::Rectangle<float> ((float) (cX + cR) * 0.5f - 52.0f, 812.0f, 104.0f, 18.0f), juce::Justification::centred, false);
    const juce::Rectangle<float> rd ((float) (cX + cR) * 0.5f - 52.0f, 834.0f, 104.0f, 36.0f);
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
    g.drawText ("v3.8", juce::Rectangle<float> (16.0f, (float) barY, 100.0f, (float) (kH - barY)), juce::Justification::centredLeft, false);
    g.drawText ("click CH = nuevo canal", juce::Rectangle<float> ((float) kW - 216.0f, (float) barY, 200.0f, (float) (kH - barY)), juce::Justification::centredRight, false);
}
