#include "PluginEditor.h"
#include "DarkTransformerData.h"

using namespace IronColours;

namespace
{
    constexpr int kW = 780, kH = 600;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::Michroma_ttf, DarkTransformerData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::BarlowSemiBold_ttf, DarkTransformerData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkTransformerData::BarlowMedium_ttf, DarkTransformerData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Point<float> polar (juce::Point<float> c, float angle, float r)
    {
        return { c.x + std::sin (angle) * r, c.y - std::cos (angle) * r };
    }

    // Texto grabado en metal: brillo abajo y tinta encima
    void engraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r,
                   juce::Justification j, juce::Colour ink)
    {
        g.setColour (juce::Colours::white.withAlpha (0.28f));
        g.drawText (text, r.translated (0.0f, 1.0f), j, false);
        g.setColour (ink);
        g.drawText (text, r, j, false);
    }

    // Escala del VU: -20 a +3 dBFS, espaciada por amplitud como un VU real
    constexpr float kVuMin = -20.0f, kVuMax = 3.0f;
    float vuPosition (float db)
    {
        const float a  = std::pow (10.0f, juce::jlimit (kVuMin, kVuMax + 1.0f, db) / 20.0f);
        const float a0 = std::pow (10.0f, kVuMin / 20.0f);
        const float a1 = std::pow (10.0f, kVuMax / 20.0f);
        return juce::jlimit (0.0f, 1.06f, (a - a0) / (a1 - a0));
    }
}

//==============================================================================
void IronLookAndFeel::paintKnob (juce::Graphics& g, juce::Point<float> c, float R, float proportion, juce::Colour capCol)
{
    const float pi = juce::MathConstants<float>::pi;
    const float angle = startAngle + proportion * (endAngle - startAngle);

    juce::Path body;
    body.addEllipse (c.x - R, c.y - R, R * 2.0f, R * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.75f), (int) (R * 0.45f),
                      { (int) (R * 0.06f), (int) (R * 0.14f) }).drawForPath (g, body);

    // Falda de baquelita negra
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff35322f), c.x - R * 0.5f, c.y - R * 0.7f,
                                             juce::Colour (0xff060606), c.x + R * 0.6f, c.y + R * 0.8f, true));
    g.fillPath (body);

    // Moleteado fino en el borde
    const int knurls = juce::jlimit (60, 160, (int) (R * 2.2f));
    for (int i = 0; i < knurls; ++i)
    {
        const float a = (float) i * 2.0f * pi / (float) knurls;
        const auto p1 = polar (c, a, R * 0.86f);
        const auto p2 = polar (c, a, R * 0.995f);
        const float light = 0.5f + 0.5f * std::cos (a + 0.8f);
        g.setColour (juce::Colour (0xff5a5650).withAlpha (0.18f + 0.3f * light));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 0.9f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (c.x - R * 0.86f, c.y - R * 0.86f, R * 1.72f, R * 1.72f, 1.2f);

    // Línea índice color crema sobre la falda
    const auto i1 = polar (c, angle, R * 0.66f);
    const auto i2 = polar (c, angle, R * 0.97f);
    g.setColour (cream);
    g.drawLine (i1.x, i1.y, i2.x, i2.y, juce::jmax (2.5f, R * 0.06f));

    // Tapa de color brillante
    const float cr = R * 0.56f;
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (c.x - cr - 2.0f, c.y - cr - 1.0f, (cr + 2.0f) * 2.0f, (cr + 2.0f) * 2.0f);

    juce::ColourGradient capGrad (capCol.brighter (0.55f), c.x - cr * 0.5f, c.y - cr * 0.6f,
                                  capCol.darker (0.7f), c.x + cr * 0.7f, c.y + cr * 0.9f, true);
    capGrad.addColour (0.45, capCol);
    g.setGradientFill (capGrad);
    g.fillEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f);

    // Reflejo especular
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f), c.x - cr * 0.35f, c.y - cr * 0.55f,
                                             juce::Colours::transparentWhite, c.x - cr * 0.05f, c.y - cr * 0.05f, true));
    g.fillEllipse (c.x - cr * 0.75f, c.y - cr * 0.85f, cr * 0.9f, cr * 0.7f);

    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawEllipse (c.x - cr, c.y - cr, cr * 2.0f, cr * 2.0f, 1.0f);
}

void IronLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                        float sliderPos, float, float, juce::Slider& s)
{
    const float size = (float) juce::jmin (width, height);
    const juce::Point<float> c ((float) x + (float) width * 0.5f, (float) y + (float) height * 0.5f);
    const auto cap = juce::Colour ((juce::uint32) (juce::int64) s.getProperties().getWithDefault ("cap", (juce::int64) capGrey.getARGB()));
    paintKnob (g, c, size * 0.5f, sliderPos, cap);
}

//==============================================================================
void IronButton::paintButton (juce::Graphics& g, bool over, bool down)
{
    auto b = getLocalBounds().toFloat();

    if (style == Style::lamp)
    {
        const bool on = getToggleState();
        const float r = juce::jmin (b.getHeight() * 0.5f - 12.0f, 14.0f);
        const juce::Point<float> c (b.getX() + r + 12.0f, b.getCentreY());

        // Bisel cromado
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e4da), c.x - r, c.y - r,
                                                 juce::Colour (0xff4b4740), c.x + r, c.y + r, false));
        g.fillEllipse (c.x - r - 3.0f, c.y - r - 3.0f, (r + 3.0f) * 2.0f, (r + 3.0f) * 2.0f);

        if (on)
        {
            g.setGradientFill (juce::ColourGradient (lamp.withAlpha (0.55f), c.x, c.y,
                                                     lamp.withAlpha (0.0f), c.x + r + 11.0f, c.y, true));
            g.fillEllipse (c.x - r - 11.0f, c.y - r - 11.0f, (r + 11.0f) * 2.0f, (r + 11.0f) * 2.0f);
        }
        g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xfffff1c2) : juce::Colour (0xff7a4a18), c.x - r * 0.3f, c.y - r * 0.4f,
                                                 on ? juce::Colour (0xffe0700a) : juce::Colour (0xff2a1606), c.x + r, c.y + r, true));
        g.fillEllipse (c.x - r, c.y - r, r * 2.0f, r * 2.0f);

        // Facetas de la joya
        g.setColour (juce::Colours::white.withAlpha (on ? 0.25f : 0.08f));
        for (int k = 0; k < 6; ++k)
        {
            const float a = (float) k * juce::MathConstants<float>::pi / 3.0f;
            const auto p = polar (c, a, r * 0.95f);
            g.drawLine (c.x, c.y, p.x, p.y, 0.8f);
        }
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.fillEllipse (c.x - r * 0.5f, c.y - r * 0.6f, r * 0.45f, r * 0.3f);

        g.setFont (titleFont (15.0f).withExtraKerningFactor (0.1f));
        engraved (g, getButtonText(), b.withTrimmedLeft (r * 2.0f + 30.0f), juce::Justification::centredLeft, engrave);
        return;
    }

    // Tecla color marfil
    b = b.reduced (2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 5, { 0, down ? 1 : 3 }).drawForRectangle (g, b.toNearestInt());
    const auto top    = down ? juce::Colour (0xffcfc4a6) : juce::Colour (0xfff4ecd4);
    const auto bottom = down ? juce::Colour (0xffb3a886) : juce::Colour (0xffcdbf9a);
    g.setGradientFill (juce::ColourGradient (over && ! down ? top.brighter (0.05f) : top, b.getX(), b.getY(),
                                             bottom, b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 2.0f);
    g.setColour (juce::Colour (0xff1a1610));
    g.drawRoundedRectangle (b, 2.0f, 1.2f);
    g.setColour (juce::Colours::white.withAlpha (0.6f));
    g.drawLine (b.getX() + 3.0f, b.getY() + 1.5f, b.getRight() - 3.0f, b.getY() + 1.5f, 1.0f);

    g.setColour (juce::Colour (0xff2a241a));
    g.setFont (numberFont (15.0f).withExtraKerningFactor (0.12f));
    g.drawText (getButtonText(), b.translated (0.0f, down ? 1.0f : 0.0f), juce::Justification::centred, false);
}

//==============================================================================
DarkTransformerAudioProcessorEditor::DarkTransformerAudioProcessorEditor (DarkTransformerAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      driveAttachment (p.apvts, "drive", driveKnob),
      mixAttachment (p.apvts, "mix", mixKnob),
      outputAttachment (p.apvts, "output", outputKnob),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v) { btnIn.setToggleState (v < 0.5f, juce::dontSendNotification); })
{
    auto setupKnob = [this] (juce::Slider& s, double resetValue, juce::Colour cap)
    {
        s.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setRotaryParameters (IronLookAndFeel::startAngle, IronLookAndFeel::endAngle, true);
        s.setDoubleClickReturnValue (true, resetValue);
        s.getProperties().set ("cap", (juce::int64) cap.getARGB());
        s.setLookAndFeel (&laf);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };
    setupKnob (driveKnob, 0.0, capRed);
    setupKnob (mixKnob, 100.0, capGrey);
    setupKnob (outputKnob, 0.0, capGrey);

    btnIn.onClick = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    btnLoad.onClick = [this] { chooseModel(); };
    btnDefault.onClick = [this] { processor.loadDefaultModel(); repaint(); };
    addAndMakeVisible (btnIn);
    addAndMakeVisible (btnLoad);
    addAndMakeVisible (btnDefault);

    bypassAttachment.sendInitialUpdate();

    buildTexture();
    setSize (kW, kH);
    startTimerHz (30);
}

DarkTransformerAudioProcessorEditor::~DarkTransformerAudioProcessorEditor()
{
    driveKnob.setLookAndFeel (nullptr);
    mixKnob.setLookAndFeel (nullptr);
    outputKnob.setLookAndFeel (nullptr);
}

void DarkTransformerAudioProcessorEditor::chooseModel()
{
    chooser = std::make_unique<juce::FileChooser> ("Elegir captura de transformador",
                                                   juce::File::getSpecialLocation (juce::File::userDocumentsDirectory),
                                                   "*.nam");
    chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                          [safe = juce::Component::SafePointer<DarkTransformerAudioProcessorEditor> (this)] (const juce::FileChooser& fc)
                          {
                              if (safe == nullptr) return;
                              const auto file = fc.getResult();
                              if (file != juce::File())
                                  safe->processor.loadModelFile (file);
                              safe->repaint();
                          });
}

//==============================================================================
void DarkTransformerAudioProcessorEditor::buildTexture()
{
    // Pintura martillada (hammertone) verde oscuro
    const int w = kW, h = kH;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (1957);

    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x)
        {
            const float n = (rng.nextFloat() - 0.5f) * 7.0f;
            const float vignette = 1.0f - 0.35f * (std::abs ((float) x / (float) w - 0.5f) + std::abs ((float) y / (float) h - 0.5f));
            const auto ch = [&] (juce::uint8 c) { return (juce::uint8) juce::jlimit (0, 255, (int) ((float) c * vignette + n)); };
            texture.setPixelAt (x, y, juce::Colour (ch (panel.getRed()), ch (panel.getGreen()), ch (panel.getBlue()), (juce::uint8) 255));
        }

    juce::Graphics g (texture);
    for (int i = 0; i < 1400; ++i) // "martillazos": manchas suaves claras y oscuras
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float r = 4.0f + rng.nextFloat() * 16.0f;
        const bool light = rng.nextBool();
        g.setGradientFill (juce::ColourGradient ((light ? juce::Colour (0xff6f8a82) : juce::Colours::black).withAlpha (0.07f), x, y,
                                                 juce::Colours::transparentBlack, x + r, y, true));
        g.fillEllipse (x - r, y - r, r * 2.0f, r * 2.0f);
    }
}

void DarkTransformerAudioProcessorEditor::resized()
{
    headerBox = { 20.0f, 18.0f, 740.0f, 74.0f };
    meterBox  = { 40.0f, 118.0f, 330.0f, 200.0f };
    plateBox  = { 430.0f, 392.0f, 310.0f, 140.0f };
    driveC = { 575.0f, 226.0f };
    mixC   = { 130.0f, 432.0f };
    outC   = { 300.0f, 432.0f };

    driveKnob.setBounds (juce::Rectangle<float> (0, 0, 172, 172).withCentre (driveC).toNearestInt());
    mixKnob.setBounds (juce::Rectangle<float> (0, 0, 96, 96).withCentre (mixC).toNearestInt());
    outputKnob.setBounds (juce::Rectangle<float> (0, 0, 96, 96).withCentre (outC).toNearestInt());

    btnIn.setBounds (634, 22, 110, 66);
    btnLoad.setBounds ((int) plateBox.getX() + 16, (int) plateBox.getBottom() - 50, 132, 36);
    btnDefault.setBounds ((int) plateBox.getRight() - 148, (int) plateBox.getBottom() - 50, 132, 36);
}

void DarkTransformerAudioProcessorEditor::timerCallback()
{
    // Balística de VU (~300 ms) y LED de pico sobre 0 dBFS
    const float level = processor.ironLevelDb.load();
    const float target = level > -60.0f ? vuPosition (level) : 0.0f;
    needle += 0.22f * (target - needle);
    peakHold = level > 0.0f ? 1.0f : peakHold * 0.9f;
    repaint (meterBox.expanded (4.0f).toNearestInt());

    if (processor.getModelName() != shownModel || processor.getLastError() != shownError)
    {
        shownModel = processor.getModelName();
        shownError = processor.getLastError();
        repaint();
    }
}

//==============================================================================
void DarkTransformerAudioProcessorEditor::drawScale (juce::Graphics& g, juce::Point<float> c, float tickIn, float tickOut,
                                                     float labelR, const juce::StringArray& labels, float fontSize)
{
    const int n = labels.size();
    g.setFont (numberFont (fontSize));
    for (int i = 0; i < n; ++i)
    {
        const float prop = n > 1 ? (float) i / (float) (n - 1) : 0.0f;
        const float a = IronLookAndFeel::startAngle + prop * (IronLookAndFeel::endAngle - IronLookAndFeel::startAngle);
        const auto p1 = polar (c, a, tickIn), p2 = polar (c, a, tickOut);
        g.setColour (cream);
        g.drawLine (p1.x, p1.y, p2.x, p2.y, 2.0f);
        g.drawText (labels[i], juce::Rectangle<float> (44.0f, fontSize + 8.0f).withCentre (polar (c, a, labelR)),
                    juce::Justification::centred, false);

        // marcas menores entre números
        if (i < n - 1)
        {
            const float am = a + 0.5f * (IronLookAndFeel::endAngle - IronLookAndFeel::startAngle) / (float) (n - 1);
            const auto q1 = polar (c, am, tickIn + 2.0f), q2 = polar (c, am, tickOut - 2.0f);
            g.setColour (creamDim.withAlpha (0.7f));
            g.drawLine (q1.x, q1.y, q2.x, q2.y, 1.2f);
        }
    }
}

void DarkTransformerAudioProcessorEditor::drawKnobTitle (juce::Graphics& g, float cx, float y,
                                                         const juce::String& name, const juce::String& unit)
{
    g.setColour (cream);
    g.setFont (titleFont (13.0f).withExtraKerningFactor (0.14f));
    g.drawText (name, juce::Rectangle<float> (200.0f, 20.0f).withCentre ({ cx, y }), juce::Justification::centred, false);
    if (unit.isNotEmpty())
    {
        g.setColour (creamDim);
        g.setFont (labelFont (12.0f).withExtraKerningFactor (0.1f));
        g.drawText (unit, juce::Rectangle<float> (80.0f, 14.0f).withCentre ({ cx, y + 16.0f }), juce::Justification::centred, false);
    }
}

void DarkTransformerAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    // Tornillo de bronce ranurado
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe2c27a), c.x - 4.0f, c.y - 4.0f,
                                             juce::Colour (0xff5c4318), c.x + 7.0f, c.y + 7.0f, false));
    g.fillEllipse (c.x - 7.5f, c.y - 7.5f, 15.0f, 15.0f);
    g.setColour (juce::Colour (0xff2a1d08));
    g.drawEllipse (c.x - 7.5f, c.y - 7.5f, 15.0f, 15.0f, 0.8f);
    g.drawLine (c.x - 5.0f, c.y - 1.5f, c.x + 5.0f, c.y + 1.5f, 1.8f);
}

void DarkTransformerAudioProcessorEditor::drawTransformerSymbol (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour col)
{
    // Símbolo eléctrico del transformador: dos bobinas y el núcleo de hierro
    juce::Path p;
    const int turns = 4;
    const float turnH = r.getHeight() / (float) turns;
    const float lx = r.getX() + r.getWidth() * 0.30f, rx = r.getRight() - r.getWidth() * 0.30f;
    for (int side = 0; side < 2; ++side)
    {
        const float x = side == 0 ? lx : rx;
        const float dir = side == 0 ? -1.0f : 1.0f;
        p.startNewSubPath (x, r.getY());
        for (int t = 0; t < turns; ++t)
        {
            const float y0 = r.getY() + turnH * (float) t;
            p.cubicTo (x + dir * turnH * 0.75f, y0, x + dir * turnH * 0.75f, y0 + turnH, x, y0 + turnH);
        }
    }
    g.setColour (col);
    g.strokePath (p, juce::PathStrokeType (2.2f));
    const float cx = r.getCentreX();
    g.drawLine (cx - 3.0f, r.getY() - 2.0f, cx - 3.0f, r.getBottom() + 2.0f, 2.2f);
    g.drawLine (cx + 3.0f, r.getY() - 2.0f, cx + 3.0f, r.getBottom() + 2.0f, 2.2f);
}

void DarkTransformerAudioProcessorEditor::drawHeader (juce::Graphics& g)
{
    const auto b = headerBox;
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 10, { 0, 4 }).drawForRectangle (g, b.toNearestInt());

    // Placa de cobre cepillada
    juce::ColourGradient cg (copperHi, b.getX(), b.getY(), copperLo, b.getX(), b.getBottom(), false);
    cg.addColour (0.45, copper);
    g.setGradientFill (cg);
    g.fillRoundedRectangle (b, 4.0f);

    juce::Random rng (77);
    for (int i = 0; i < 260; ++i)
    {
        const float x = b.getX() + rng.nextFloat() * b.getWidth();
        g.setColour ((rng.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (0.03f + rng.nextFloat() * 0.05f));
        g.drawLine (x, b.getY() + 2.0f, x + rng.nextFloat() * 2.0f, b.getBottom() - 2.0f, 0.7f);
    }

    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.drawLine (b.getX() + 4.0f, b.getY() + 1.0f, b.getRight() - 4.0f, b.getY() + 1.0f, 1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (b, 4.0f, 1.2f);

    // Remaches
    for (auto pt : { juce::Point<float> (b.getX() + 12.0f, b.getY() + 12.0f), { b.getX() + 12.0f, b.getBottom() - 12.0f },
                     { b.getRight() - 12.0f, b.getY() + 12.0f }, { b.getRight() - 12.0f, b.getBottom() - 12.0f } })
    {
        g.setGradientFill (juce::ColourGradient (copperHi.brighter (0.3f), pt.x - 2.0f, pt.y - 2.0f, copperLo.darker (0.4f), pt.x + 3.0f, pt.y + 3.0f, false));
        g.fillEllipse (pt.x - 4.0f, pt.y - 4.0f, 8.0f, 8.0f);
    }

    drawTransformerSymbol (g, { b.getX() + 34.0f, b.getY() + 17.0f, 46.0f, 40.0f }, engrave);

    g.setFont (titleFont (25.0f).withExtraKerningFactor (0.06f));
    engraved (g, "DARK TRANSFORMER", { b.getX() + 96.0f, b.getY() + 10.0f, 520.0f, 34.0f }, juce::Justification::centredLeft, engrave);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.3f));
    engraved (g, "NEURAL IRON SATURATION", { b.getX() + 98.0f, b.getY() + 46.0f, 400.0f, 16.0f }, juce::Justification::centredLeft, engrave.withAlpha (0.85f));
}

void DarkTransformerAudioProcessorEditor::drawMeter (juce::Graphics& g)
{
    const auto b = meterBox;
    // Marco de bronce y vidrio
    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 12, { 0, 5 }).drawForRectangle (g, b.toNearestInt());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffc9a66a), b.getX(), b.getY(),
                                             juce::Colour (0xff4a3614), b.getRight(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 8.0f);
    const auto face = b.reduced (9.0f);
    g.setColour (juce::Colour (0xff120d06));
    g.fillRoundedRectangle (face.expanded (1.5f), 5.0f);

    // Cara color crema iluminada desde abajo
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff2cf), face.getCentreX(), face.getBottom(),
                                             juce::Colour (0xffd9c79a), face.getCentreX(), face.getY() - 40.0f, true));
    g.fillRoundedRectangle (face, 4.0f);

    g.saveState();
    g.reduceClipRegion (face.toNearestInt());

    const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + 34.0f);
    const float arcR = face.getHeight() + 6.0f;
    const float halfSpan = 0.80f; // radianes a cada lado
    const auto angleFor = [halfSpan] (float pos) { return -halfSpan + 2.0f * halfSpan * pos; };

    // Arco principal y zona roja
    juce::Path arc, red;
    arc.addCentredArc (pivot.x, pivot.y, arcR - 26.0f, arcR - 26.0f, 0.0f, angleFor (0.0f), angleFor (vuPosition (0.0f)), true);
    red.addCentredArc (pivot.x, pivot.y, arcR - 26.0f, arcR - 26.0f, 0.0f, angleFor (vuPosition (0.0f)), angleFor (1.0f), true);
    g.setColour (juce::Colour (0xff1d1a14));
    g.strokePath (arc, juce::PathStrokeType (1.6f));
    g.setColour (juce::Colour (0xffc0261c));
    g.strokePath (red, juce::PathStrokeType (6.0f));

    struct Mark { float db; bool label; };
    const Mark marks[] = { { -20, true }, { -15, false }, { -10, true }, { -7, true }, { -5, true }, { -4, false },
                           { -3, true }, { -2, false }, { -1, true }, { 0, true }, { 1, false }, { 2, false }, { 3, true } };
    for (const auto& m : marks)
    {
        const float a = angleFor (vuPosition (m.db));
        const bool hot = m.db > 0.0f;
        const auto p1 = polar (pivot, a, arcR - 26.0f);
        const auto p2 = polar (pivot, a, arcR - (m.label ? 14.0f : 19.0f));
        g.setColour (hot ? juce::Colour (0xffc0261c) : juce::Colour (0xff1d1a14));
        g.drawLine (p1.x, p1.y, p2.x, p2.y, m.label ? 1.8f : 1.2f);
        if (m.label)
        {
            g.setFont (numberFont (14.0f));
            const juce::String txt = m.db > 0 ? "+" + juce::String ((int) m.db) : juce::String ((int) m.db);
            g.drawText (txt, juce::Rectangle<float> (34.0f, 16.0f).withCentre (polar (pivot, a, arcR - 3.0f)),
                        juce::Justification::centred, false);
        }
    }

    // Leyendas
    g.setColour (juce::Colour (0xff2a241a));
    g.setFont (titleFont (16.0f).withExtraKerningFactor (0.2f));
    g.drawText ("IRON", juce::Rectangle<float> (120.0f, 22.0f).withCentre ({ face.getCentreX(), face.getBottom() - 54.0f }),
                juce::Justification::centred, false);
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.setColour (juce::Colour (0xff6b5f45));
    g.drawText ("dBFS AL TRANSFORMADOR", juce::Rectangle<float> (220.0f, 14.0f).withCentre ({ face.getCentreX(), face.getBottom() - 34.0f }),
                juce::Justification::centred, false);

    // Aguja
    const float na = angleFor (juce::jlimit (-0.03f, 1.06f, needle));
    const auto tip = polar (pivot, na, arcR - 8.0f);
    const auto base = polar (pivot, na, 30.0f);
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.drawLine (base.x + 3.0f, base.y + 3.0f, tip.x + 3.0f, tip.y + 3.0f, 1.6f);
    g.setColour (juce::Colour (0xff141210));
    g.drawLine (base.x, base.y, tip.x, tip.y, 1.6f);

    // Tapa del pivote
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3b352c), pivot.x, pivot.y - 40.0f,
                                             juce::Colour (0xff0d0b08), pivot.x, pivot.y, false));
    g.fillEllipse (pivot.x - 52.0f, pivot.y - 48.0f, 104.0f, 96.0f);

    g.restoreState();

    // LED de pico
    const juce::Point<float> led (face.getRight() - 18.0f, face.getY() + 18.0f);
    if (peakHold > 0.05f)
    {
        g.setColour (juce::Colour (0xffff3a2a).withAlpha (0.35f * peakHold));
        g.fillEllipse (led.x - 11.0f, led.y - 11.0f, 22.0f, 22.0f);
    }
    g.setColour (juce::Colour (0xff4a120c).interpolatedWith (juce::Colour (0xffff3a2a), peakHold));
    g.fillEllipse (led.x - 5.5f, led.y - 5.5f, 11.0f, 11.0f);
    g.setColour (juce::Colour (0xff6b5f45));
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.2f));
    g.drawText ("PEAK", juce::Rectangle<float> (face.getRight() - 64.0f, led.y - 7.0f, 38.0f, 14.0f), juce::Justification::centredRight, false);

    // Reflejo del vidrio
    juce::Path glass;
    glass.startNewSubPath (face.getX(), face.getY());
    glass.lineTo (face.getX() + face.getWidth() * 0.55f, face.getY());
    glass.lineTo (face.getX(), face.getY() + face.getHeight() * 0.7f);
    glass.closeSubPath();
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.fillPath (glass);
}

void DarkTransformerAudioProcessorEditor::drawModelPlate (juce::Graphics& g)
{
    const auto b = plateBox;
    juce::DropShadow (juce::Colours::black.withAlpha (0.7f), 10, { 0, 4 }).drawForRectangle (g, b.toNearestInt());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1b1a18), b.getX(), b.getY(),
                                             juce::Colour (0xff0b0a09), b.getX(), b.getBottom(), false));
    g.fillRoundedRectangle (b, 4.0f);
    g.setColour (copper);
    g.drawRoundedRectangle (b.reduced (3.0f), 3.0f, 1.4f);

    for (auto pt : { juce::Point<float> (b.getX() + 12.0f, b.getY() + 12.0f), { b.getRight() - 12.0f, b.getY() + 12.0f } })
    {
        g.setGradientFill (juce::ColourGradient (copperHi, pt.x - 2.0f, pt.y - 2.0f, copperLo, pt.x + 3.0f, pt.y + 3.0f, false));
        g.fillEllipse (pt.x - 3.5f, pt.y - 3.5f, 7.0f, 7.0f);
    }

    g.setColour (creamDim);
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.3f));
    g.drawText ("CAPTURA", juce::Rectangle<float> (b.getX(), b.getY() + 10.0f, b.getWidth(), 14.0f), juce::Justification::centred, false);

    const bool neural = processor.isModelNeural();
    const auto tag = juce::Rectangle<float> (b.getX() + 16.0f, b.getY() + 34.0f, 48.0f, 26.0f);
    g.setColour (neural ? lamp.withAlpha (0.16f) : juce::Colour (0xff1f1e1b));
    g.fillRoundedRectangle (tag, 3.0f);
    g.setColour (neural ? lamp : creamDim);
    g.drawRoundedRectangle (tag, 3.0f, 1.0f);
    g.setFont (numberFont (14.0f).withExtraKerningFactor (0.1f));
    g.drawText (neural ? "NAM" : "DSP", tag, juce::Justification::centred, false);

    g.setColour (neural ? lamp : cream);
    g.setFont (numberFont (19.0f));
    g.drawFittedText (processor.getModelName(),
                      juce::Rectangle<float> (tag.getRight() + 12.0f, tag.getY(), b.getRight() - tag.getRight() - 28.0f, tag.getHeight()).toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.8f);

    if (processor.getLastError().isNotEmpty())
    {
        g.setColour (juce::Colour (0xffff6a5a));
        g.setFont (labelFont (12.0f));
        g.drawFittedText (processor.getLastError(),
                          juce::Rectangle<int> ((int) b.getX(), (int) b.getBottom() + 6, (int) b.getWidth(), 16),
                          juce::Justification::centredLeft, 1, 0.8f);
    }
}

//==============================================================================
void DarkTransformerAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.drawImageAt (texture, 0, 0);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawRect (getLocalBounds(), 2);

    drawScrew (g, { 22.0f, h - 22.0f });
    drawScrew (g, { w - 22.0f, h - 22.0f });

    drawHeader (g);
    drawMeter (g);

    drawScale (g, driveC, 92.0f, 102.0f, 118.0f, { "-12", "-6", "0", "+6", "+12", "+18", "+24" }, 16.0f);
    drawScale (g, mixC,   54.0f, 62.0f,  76.0f,  { "0", "25", "50", "75", "100" }, 13.0f);
    drawScale (g, outC,   54.0f, 62.0f,  76.0f,  { "-18", "-12", "-6", "0", "+6", "+12", "+18" }, 12.0f);

    drawKnobTitle (g, driveC.x, 352.0f, "DRIVE", "dB");
    drawKnobTitle (g, mixC.x,   516.0f, "MIX", "%");
    drawKnobTitle (g, outC.x,   516.0f, "OUTPUT", "dB");

    drawModelPlate (g);

    g.setColour (creamDim.withAlpha (0.8f));
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.3f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (44.0f, h - 30.0f, 260.0f, 16.0f),
                juce::Justification::centredLeft, false);
    g.setColour (creamDim.withAlpha (0.5f));
    g.drawText ("NAM CORE (MIT) - S. ATKINSON", juce::Rectangle<float> (w - 330.0f, h - 30.0f, 286.0f, 16.0f),
                juce::Justification::centredRight, false);
}
