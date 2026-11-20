#include "PluginEditor.h"
#include "BlueyFonts.h"

using namespace BlueyColours;

namespace
{
    constexpr int kW = 740, kH = 480;

    // Panel azul de la derecha (como el 1176 Rev A)
    constexpr float kBlueX = 460.0f, kBlueW = 244.0f;
    constexpr float kBlueCx = kBlueX + kBlueW * 0.5f;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    // Michroma: títulos anchos. Barlow: números y textos pequeños.
    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (BlueyFontData::Michroma_ttf, BlueyFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (BlueyFontData::BarlowSemiBold_ttf, BlueyFontData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (BlueyFontData::BarlowMedium_ttf, BlueyFontData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Point<float> polar (juce::Point<float> c, float angle, float r)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }
}

//==============================================================================
void BlueyLookAndFeel::paint1176Knob (juce::Graphics& g, juce::Point<float> c, float R, float proportion)
{
    const float pi = juce::MathConstants<float>::pi;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    // Sombra proyectada
    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.55f), (int) (R * 0.4f),
                      { 0, (int) (R * 0.14f) }).drawForPath (g, body);

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
// Botón de encendido: lámpara roja como la del 1176 Rev A
void PowerButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (3.0f);
    const bool on = getToggleState();
    const auto c = b.getCentre();
    const float r = b.getWidth() * 0.5f;

    // Aro metálico
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f2f4), c.x - r, c.y - r,
                                             juce::Colour (0xff6c7077), c.x + r, c.y + r, false));
    g.fillEllipse (b);

    // Lente
    const auto lens = b.reduced (r * 0.28f);
    if (on)
    {
        g.setColour (juce::Colour (0xffff3b30).withAlpha (0.30f));
        g.fillEllipse (lens.expanded (r * 0.45f));
    }
    g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xffff8a80) : juce::Colour (0xff7a2a2a),
                                             c.x - r * 0.2f, c.y - r * 0.3f,
                                             on ? juce::Colour (0xffc4150c) : juce::Colour (0xff2c0b0b),
                                             c.x + r * 0.5f, c.y + r * 0.6f, true));
    g.fillEllipse (lens);
    g.setColour (juce::Colours::black.withAlpha (over ? 0.55f : 0.4f));
    g.drawEllipse (lens, 1.0f);
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
    setSize (kW, kH);
    startTimerHz (30);
}

BlueyCompAudioProcessorEditor::~BlueyCompAudioProcessorEditor()
{
    compressKnob.setLookAndFeel (nullptr);
    outputKnob.setLookAndFeel (nullptr);
}

void BlueyCompAudioProcessorEditor::buildTexture()
{
    const int w = kW, h = kH;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (1176);

    // Frontal plateado cepillado + panel azul a la derecha (1176 Rev A)
    float rowOffset = 0.0f;
    for (int y = 0; y < h; ++y)
    {
        rowOffset = rowOffset * 0.6f + (rng.nextFloat() - 0.5f) * 5.0f;
        const float vignette = 1.0f - 0.10f * std::abs ((float) y / (float) h - 0.5f);
        const float t = (float) y / (float) h;
        for (int x = 0; x < w; ++x)
        {
            const float n = (rng.nextFloat() - 0.5f) * 5.0f;
            const bool blue = (float) x >= kBlueX && (float) x < kBlueX + kBlueW;
            float r, g, b;
            if (blue)
            {
                const float k = 1.0f + (rowOffset + n) * 0.012f;
                r = (30.0f - 8.0f * t) * k;
                g = (108.0f - 24.0f * t) * k;
                b = (196.0f - 36.0f * t) * k;
            }
            else
            {
                const float v = (200.0f + rowOffset * 1.2f + n) * vignette;
                r = v; g = v + 1.0f; b = v + 3.0f;
            }
            texture.setPixelAt (x, y, juce::Colour ((juce::uint8) juce::jlimit (0, 255, (int) r),
                                                    (juce::uint8) juce::jlimit (0, 255, (int) g),
                                                    (juce::uint8) juce::jlimit (0, 255, (int) b), (juce::uint8) 255));
        }
    }

    juce::Graphics g (texture);
    for (int i = 0; i < 200; ++i) // vetas del cepillado
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float len = 80.0f + rng.nextFloat() * 320.0f;
        const bool dark = rng.nextBool();
        g.setColour ((dark ? juce::Colours::black : juce::Colours::white).withAlpha (0.015f + rng.nextFloat() * 0.03f));
        g.drawLine (x, y, x + len, y + (rng.nextFloat() - 0.5f) * 1.5f, 0.8f);
    }

    // Bordes del panel azul: sombra a la izquierda y brillo a la derecha
    g.setColour (juce::Colour (0xff06223f).withAlpha (0.7f));
    g.fillRect (kBlueX - 2.0f, 0.0f, 2.0f, (float) h);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.fillRect (kBlueX - 3.0f, 0.0f, 1.0f, (float) h);
    g.setColour (juce::Colour (0xff06223f).withAlpha (0.7f));
    g.fillRect (kBlueX + kBlueW, 0.0f, 1.5f, (float) h);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.fillRect (kBlueX + kBlueW + 1.5f, 0.0f, 1.0f, (float) h);
}

void BlueyCompAudioProcessorEditor::resized()
{
    bigC = { 150.0f, 262.0f };
    outC = { 360.0f, 262.0f };

    compressKnob.setBounds (juce::Rectangle<float> (0, 0, 170, 170).withCentre (bigC).toNearestInt());
    outputKnob.setBounds (juce::Rectangle<float> (0, 0, 100, 100).withCentre (outC).toNearestInt());
    powerButton.setBounds ((int) kBlueCx - 20, 22, 40, 40);

    meterArea = { kBlueCx - 100.0f, 158.0f, 200.0f, 14.0f };
}

void BlueyCompAudioProcessorEditor::timerCallback()
{
    const float target = processor.gainReductionDb.load();
    grSmoothed += (target > grSmoothed ? 0.5f : 0.10f) * (target - grSmoothed);
    repaint (juce::Rectangle<float> (kBlueX, 80.0f, kBlueW, 220.0f).toNearestInt());
}

void BlueyCompAudioProcessorEditor::drawKnobScale (juce::Graphics& g, juce::Point<float> c, float tickStart,
                                                   float majorLen, float minorLen, float labelRadius,
                                                   const juce::StringArray& majors, int minorsBetween, float fontSize)
{
    const int n = majors.size();
    const int steps = (n - 1) * (minorsBetween + 1);
    g.setFont (numberFont (fontSize));

    for (int s = 0; s <= steps; ++s)
    {
        const float prop = (float) s / (float) steps;
        const float a = BlueyLookAndFeel::startAngle + prop * (BlueyLookAndFeel::endAngle - BlueyLookAndFeel::startAngle);
        const bool major = (s % (minorsBetween + 1)) == 0;

        const auto p1 = polar (c, a, tickStart);
        const auto p2 = polar (c, a, tickStart + (major ? majorLen : minorLen));
        g.setColour (ink);
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
    g.setColour (juce::Colours::black.withAlpha (0.25f));
    g.fillEllipse (c.x - 8.0f, c.y - 7.0f, 16.0f, 16.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfff0f1f3), c.x - 4.0f, c.y - 4.0f,
                                             juce::Colour (0xff80848a), c.x + 8.0f, c.y + 8.0f, false));
    g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff3a3d42));
    g.drawEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f, 0.8f);
    g.setColour (juce::Colour (0xff2a2d31));
    g.drawLine (c.x - 5.0f, c.y + 2.0f, c.x + 5.0f, c.y - 2.0f, 2.0f);
}

void BlueyCompAudioProcessorEditor::drawBarMeter (juce::Graphics& g)
{
    // Título del medidor
    g.setColour (paleBlue);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.14f));
    g.drawText ("GAIN REDUCTION  dB",
                juce::Rectangle<float> (meterArea.getX(), meterArea.getY() - 28.0f, meterArea.getWidth(), 16.0f),
                juce::Justification::centred, false);

    // Ventana negra hundida
    const auto well = meterArea.expanded (6.0f, 5.0f);
    g.setColour (juce::Colour (0xff070708));
    g.fillRoundedRectangle (well, 4.0f);
    g.setColour (juce::Colour (0xff0a2a4a));
    g.drawRoundedRectangle (well.reduced (0.5f), 4.0f, 1.4f);

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
    g.setFont (numberFont (12.0f));
    for (int v = 0; v <= 20; v += 2)
    {
        const float x = meterArea.getX() + (float) v / maxDb * meterArea.getWidth();
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillRect (x - 0.5f, well.getBottom() + 3.0f, 1.0f, 4.0f);
        if (v % 4 == 0)
            g.drawText (juce::String (v),
                        juce::Rectangle<float> (26.0f, 14.0f).withCentre ({ x, well.getBottom() + 17.0f }),
                        juce::Justification::centred, false);
    }

    // Lectura digital (ventana oscura sobre el panel azul)
    const juce::Rectangle<float> readBox (kBlueCx - 52.0f, well.getBottom() + 34.0f, 104.0f, 36.0f);
    g.setColour (juce::Colour (0xff070708));
    g.fillRoundedRectangle (readBox, 4.0f);
    g.setColour (juce::Colour (0xff0a2a4a));
    g.drawRoundedRectangle (readBox.reduced (0.5f), 4.0f, 1.4f);
    g.setColour (grSmoothed > 0.05f ? ledOn : juce::Colour (0xff6b5646));
    g.setFont (numberFont (22.0f));
    g.drawText (juce::String (grSmoothed, 1) + " dB", readBox, juce::Justification::centred, false);
}

void BlueyCompAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.drawImageAt (texture, 0, 0);

    // Bisel del frontal
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.drawRoundedRectangle (8.0f, 8.0f, w - 16.0f, h - 16.0f, 6.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawRoundedRectangle (9.0f, 9.0f, w - 16.0f, h - 16.0f, 6.0f, 1.0f);
    drawScrew (g, { 26.0f, 26.0f });
    drawScrew (g, { 26.0f, h - 26.0f });
    drawScrew (g, { w - 20.0f, 26.0f });
    drawScrew (g, { w - 20.0f, h - 26.0f });

    // Medidor
    drawBarMeter (g);

    // Título sobre el panel azul (como "76A LIMITING AMPLIFIER")
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.fillRect (kBlueCx - 100.0f, 366.0f, 200.0f, 1.0f);
    g.setFont (titleFont (20.0f).withExtraKerningFactor (0.06f));
    g.setColour (juce::Colour (0xff031a33).withAlpha (0.55f));
    g.drawText ("BLUEY COMP", juce::Rectangle<float> (kBlueX, 383.0f, kBlueW, 34.0f), juce::Justification::centred, false);
    g.setColour (juce::Colours::white);
    g.drawText ("BLUEY COMP", juce::Rectangle<float> (kBlueX - 1.0f, 382.0f, kBlueW, 34.0f), juce::Justification::centred, false);
    g.setColour (paleBlue);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("FET LIMITING AMPLIFIER", juce::Rectangle<float> (kBlueX, 418.0f, kBlueW, 18.0f),
                juce::Justification::centred, false);

    // Perillas: escalas grabadas y nombres (tipografía Michroma)
    drawKnobScale (g, bigC, 78.5f, 9.5f, 5.0f, 102.0f,
                   { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" }, 1, 17.0f);
    g.setColour (ink);
    g.setFont (titleFont (15.0f).withExtraKerningFactor (0.1f));
    g.drawText ("COMPRESS", juce::Rectangle<float> (240.0f, 26.0f).withCentre ({ bigC.x, 418.0f }),
                juce::Justification::centred, false);

    drawKnobScale (g, outC, 46.5f, 7.5f, 3.6f, 68.0f, { "-12", "-6", "0", "+6", "+12" }, 1, 13.0f);
    g.setColour (ink);
    g.setFont (titleFont (15.0f).withExtraKerningFactor (0.1f));
    g.drawText ("OUTPUT", juce::Rectangle<float> (240.0f, 26.0f).withCentre ({ outC.x, 418.0f }),
                juce::Justification::centred, false);

    g.setColour (juce::Colour (0xff4f545c));
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, h - 36.0f, kBlueX, 16.0f),
                juce::Justification::centred, false);
}
