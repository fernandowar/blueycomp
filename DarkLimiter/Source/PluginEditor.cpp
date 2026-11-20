#include "PluginEditor.h"
#include "DarkLimiterFonts.h"

using namespace LimColours;

namespace
{
    constexpr int kW = 780, kH = 560;

    juce::Typeface::Ptr loadTypeface (const char* data, int size)
    {
        return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
    }

    juce::Font titleFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLimiterFontData::Michroma_ttf, DarkLimiterFontData::Michroma_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font numberFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLimiterFontData::BarlowSemiBold_ttf, DarkLimiterFontData::BarlowSemiBold_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }

    juce::Font labelFont (float size)
    {
        static juce::Typeface::Ptr tf = loadTypeface (DarkLimiterFontData::BarlowMedium_ttf, DarkLimiterFontData::BarlowMedium_ttfSize);
        return juce::Font (juce::FontOptions (tf).withHeight (size));
    }
}

//==============================================================================
void LimiterLookAndFeel::drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                                           float sliderPos, float, float, juce::Slider::SliderStyle,
                                           juce::Slider&)
{
    const float cx = (float) x + (float) width * 0.5f;
    const float top = (float) y + (float) thumbHalf, bottom = (float) (y + height - thumbHalf);

    // Ranura
    juce::Rectangle<float> slot (cx - 4.0f, top - 4.0f, 8.0f, bottom - top + 8.0f);
    g.setColour (juce::Colour (0xff050506));
    g.fillRoundedRectangle (slot, 4.0f);
    g.setColour (juce::Colour (0xff3a3d42));
    g.drawRoundedRectangle (slot.reduced (0.5f), 4.0f, 1.0f);

    // Tramo activo (de abajo hasta la tapa)
    g.setColour (juce::Colour (0xffff9a2e).withAlpha (0.55f));
    g.fillRoundedRectangle (juce::Rectangle<float> (cx - 1.5f, sliderPos, 3.0f, bottom + 4.0f - sliderPos), 1.5f);

    // Tapa metálica
    const float capW = 54.0f, capH = (float) thumbHalf * 2.0f;
    juce::Rectangle<float> cap (cx - capW * 0.5f, sliderPos - capH * 0.5f, capW, capH);

    juce::DropShadow (juce::Colours::black.withAlpha (0.8f), 9, { 0, 4 }).drawForRectangle (g, cap.toNearestInt());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffcfd2d8), cap.getX(), cap.getY(),
                                             juce::Colour (0xff6d7077), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 4.0f);
    g.setColour (juce::Colour (0xff101113));
    g.drawRoundedRectangle (cap, 4.0f, 1.3f);

    // surcos
    g.setColour (juce::Colour (0xff3b3e44).withAlpha (0.7f));
    for (int i = -2; i <= 2; ++i)
        g.drawLine (cap.getX() + 6.0f, cap.getCentreY() + (float) i * 4.5f, cap.getRight() - 6.0f, cap.getCentreY() + (float) i * 4.5f, 1.0f);
    // línea central oscura (marca)
    g.setColour (juce::Colour (0xff111216));
    g.fillRoundedRectangle (cap.getX() + 4.0f, cap.getCentreY() - 1.8f, capW - 8.0f, 3.6f, 1.8f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.drawLine (cap.getX() + 6.0f, cap.getY() + 2.0f, cap.getRight() - 6.0f, cap.getY() + 2.0f, 1.0f);
}

//==============================================================================
void LimButton::paintButton (juce::Graphics& g, bool over, bool)
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
        const auto ledC = juce::Point<float> (b.getX() + 13.0f, b.getCentreY());
        if (on)
        {
            g.setColour (ledColour.withAlpha (0.3f));
            g.fillEllipse (ledC.x - 6.0f, ledC.y - 6.0f, 12.0f, 12.0f);
        }
        g.setColour (on ? ledColour : juce::Colour (0xff2c2e33));
        g.fillEllipse (ledC.x - 3.5f, ledC.y - 3.5f, 7.0f, 7.0f);
        textArea = textArea.withTrimmedLeft (18.0f);
    }
    g.setColour (juce::Colour (0xff15171b));
    g.setFont (numberFont (16.0f).withExtraKerningFactor (0.06f));
    g.drawText (getButtonText(), textArea, juce::Justification::centred, false);
}

//==============================================================================
DarkLimiterAudioProcessorEditor::DarkLimiterAudioProcessorEditor (DarkLimiterAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      thresholdAttachment (p.apvts, "threshold", thresholdFader),
      outputAttachment (p.apvts, "ceiling", outputFader),
      truePeakAttachment (*p.apvts.getParameter ("truepeak"),
                          [this] (float v) { btnTruePeak.setToggleState (v > 0.5f, juce::dontSendNotification); }),
      bypassAttachment (*p.apvts.getParameter ("bypass"),
                        [this] (float v) { btnIn.setToggleState (v < 0.5f, juce::dontSendNotification); })
{
    auto setupFader = [this] (juce::Slider& s, double resetValue)
    {
        s.setSliderStyle (juce::Slider::LinearVertical);
        s.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        s.setDoubleClickReturnValue (true, resetValue);
        s.setLookAndFeel (&laf);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };
    setupFader (thresholdFader, -4.0);
    setupFader (outputFader, -0.3);

    btnTruePeak.onClick = [this] { truePeakAttachment.setValueAsCompleteGesture (btnTruePeak.getToggleState() ? 1.0f : 0.0f); };
    btnIn.onClick       = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    addAndMakeVisible (btnTruePeak);
    addAndMakeVisible (btnIn);

    truePeakAttachment.sendInitialUpdate();
    bypassAttachment.sendInitialUpdate();

    buildTexture();
    setSize (kW, kH);
    startTimerHz (30);
}

DarkLimiterAudioProcessorEditor::~DarkLimiterAudioProcessorEditor()
{
    thresholdFader.setLookAndFeel (nullptr);
    outputFader.setLookAndFeel (nullptr);
}

void DarkLimiterAudioProcessorEditor::buildTexture()
{
    const int w = kW, h = kH;
    texture = juce::Image (juce::Image::ARGB, w, h, false);
    juce::Random rng (2026);

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
    for (int i = 0; i < 180; ++i)
    {
        const float x = rng.nextFloat() * (float) w, y = rng.nextFloat() * (float) h;
        const float len = 80.0f + rng.nextFloat() * 320.0f;
        g.setColour (juce::Colours::white.withAlpha (0.015f + rng.nextFloat() * 0.035f));
        g.drawLine (x, y, x + len, y + (rng.nextFloat() - 0.5f) * 1.5f, 0.8f);
    }
}

void DarkLimiterAudioProcessorEditor::resized()
{
    thresholdFader.setBounds (112, 104, 60, 336);
    outputFader.setBounds (608, 104, 60, 336);
    displayBox = { 238.0f, 96.0f, 304.0f, 352.0f };

    btnTruePeak.setBounds (kW / 2 - 66, 462, 132, 34);
    btnIn.setBounds (kW - 150, 28, 96, 36);
}

void DarkLimiterAudioProcessorEditor::timerCallback()
{
    const float grTarget = processor.gainReductionDb.load();
    grSmoothed += (grTarget > grSmoothed ? 0.6f : 0.12f) * (grTarget - grSmoothed);

    const float pk = processor.outputPeakDb.load();
    peakSmoothed += (pk > peakSmoothed ? 0.8f : 0.10f) * (pk - peakSmoothed);

    lufsShown = processor.lufsShortTerm.load();

    if (++historyTick >= 15) // cada 0.5 s
    {
        historyTick = 0;
        history.push_back (lufsShown);
        while (history.size() > 120) history.pop_front();
    }
    repaint (displayBox.expanded (4.0f).toNearestInt());
}

void DarkLimiterAudioProcessorEditor::drawScrew (juce::Graphics& g, juce::Point<float> c)
{
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a6d72), c.x - 3.0f, c.y - 3.0f,
                                             juce::Colour (0xff1c1d20), c.x + 8.0f, c.y + 8.0f, false));
    g.fillEllipse (c.x - 8.0f, c.y - 8.0f, 16.0f, 16.0f);
    g.setColour (juce::Colour (0xff0b0b0c));
    g.drawLine (c.x - 5.0f, c.y + 2.0f, c.x + 5.0f, c.y - 2.0f, 2.0f);
}

void DarkLimiterAudioProcessorEditor::drawFaderScale (juce::Graphics& g, const juce::Slider& s, float minV, float maxV,
                                                      float tickStep, float labelStep, bool labelsOnLeft)
{
    const auto b = s.getBounds().toFloat();
    const float cx = b.getCentreX();
    const float top = b.getY() + (float) LimiterLookAndFeel::thumbHalf;
    const float bottom = b.getBottom() - (float) LimiterLookAndFeel::thumbHalf;
    const float dir = labelsOnLeft ? -1.0f : 1.0f;

    g.setFont (numberFont (14.0f));
    const int steps = (int) std::round ((maxV - minV) / tickStep);
    for (int i = 0; i <= steps; ++i)
    {
        const float v = maxV - (float) i * tickStep;
        const float y = top + (maxV - v) / (maxV - minV) * (bottom - top);
        const bool major = std::abs (std::fmod (std::abs (v) + 0.001f, labelStep)) < 0.01f;

        g.setColour (juce::Colour (0xfff0f1f4).withAlpha (major ? 0.95f : 0.55f));
        const float x0 = cx + dir * 38.0f, x1 = x0 + dir * (major ? 12.0f : 7.0f);
        g.drawLine (x0, y, x1, y, major ? 1.6f : 1.0f);

        if (major)
        {
            g.setColour (number);
            const float tx = x1 + dir * 4.0f;
            g.drawText (juce::String ((int) std::round (v)),
                        juce::Rectangle<float> (labelsOnLeft ? tx - 38.0f : tx, y - 9.0f, 38.0f, 18.0f),
                        labelsOnLeft ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
        }
    }
}

void DarkLimiterAudioProcessorEditor::drawBar (juce::Graphics& g, juce::Rectangle<float> area, int segs, float value,
                                               float minV, float maxV, bool greenToRed,
                                               const juce::StringArray& labels, const juce::Array<float>& labelValues)
{
    g.setColour (juce::Colour (0xff070708));
    g.fillRoundedRectangle (area.expanded (5.0f, 4.0f), 3.0f);
    g.setColour (juce::Colour (0xff2f3238));
    g.drawRoundedRectangle (area.expanded (5.0f, 4.0f).reduced (0.5f), 3.0f, 1.0f);

    const float segW = area.getWidth() / (float) segs;
    for (int i = 0; i < segs; ++i)
    {
        const float segV = minV + ((float) i + 0.5f) / (float) segs * (maxV - minV);
        const bool on = segV < value;
        juce::Colour col;
        if (greenToRed)
        {
            const float t = (float) i / (float) (segs - 1);
            col = t < 0.7f ? juce::Colour (0xff3ddc6a) : (t < 0.92f ? juce::Colour (0xffffd23a) : juce::Colour (0xffff3a3a));
        }
        else
            col = juce::Colour (0xffff9a2e);

        const auto r = juce::Rectangle<float> (area.getX() + segW * (float) i + 0.5f, area.getY(), segW - 1.5f, area.getHeight());
        g.setColour (on ? col : col.withMultipliedBrightness (0.2f).withMultipliedSaturation (0.6f));
        g.fillRoundedRectangle (r, 1.2f);
    }

    g.setFont (numberFont (12.0f));
    g.setColour (number);
    for (int i = 0; i < labels.size(); ++i)
    {
        const float x = area.getX() + (labelValues[i] - minV) / (maxV - minV) * area.getWidth();
        g.drawText (labels[i], juce::Rectangle<float> (36.0f, 14.0f).withCentre ({ x, area.getBottom() + 15.0f }),
                    juce::Justification::centred, false);
    }
}

void DarkLimiterAudioProcessorEditor::drawDisplay (juce::Graphics& g)
{
    const auto d = displayBox;
    g.setColour (juce::Colour (0xff08090b));
    g.fillRoundedRectangle (d, 8.0f);
    g.setColour (juce::Colour (0xff383b42));
    g.drawRoundedRectangle (d.reduced (0.5f), 8.0f, 1.4f);

    // LUFS de corto plazo
    g.setColour (grey);
    g.setFont (titleFont (11.0f).withExtraKerningFactor (0.14f));
    g.drawText ("SHORT TERM", juce::Rectangle<float> (d.getX(), d.getY() + 12.0f, d.getWidth(), 16.0f),
                juce::Justification::centred, false);

    const bool valid = lufsShown > -69.0f;
    g.setColour (valid ? amber : juce::Colour (0xff6b5030));
    g.setFont (numberFont (60.0f));
    g.drawText (valid ? juce::String (lufsShown, 1) : juce::String ("-inf"),
                juce::Rectangle<float> (d.getX(), d.getY() + 28.0f, d.getWidth(), 64.0f), juce::Justification::centred, false);
    g.setColour (grey);
    g.setFont (titleFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("LUFS", juce::Rectangle<float> (d.getX(), d.getY() + 92.0f, d.getWidth(), 16.0f),
                juce::Justification::centred, false);

    // Historial (60 s)
    const juce::Rectangle<float> gr (d.getX() + 18.0f, d.getY() + 122.0f, d.getWidth() - 36.0f, 82.0f);
    g.setColour (juce::Colour (0xff101216));
    g.fillRect (gr);
    constexpr float lo = -30.0f, hi = -6.0f;
    g.setFont (numberFont (10.0f));
    for (float v = -24.0f; v <= -12.0f; v += 6.0f)
    {
        const float y = gr.getBottom() - (v - lo) / (hi - lo) * gr.getHeight();
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawHorizontalLine ((int) y, gr.getX(), gr.getRight());
        g.setColour (grey.withAlpha (0.8f));
        g.drawText (juce::String ((int) v), juce::Rectangle<float> (gr.getX() + 2.0f, y - 12.0f, 24.0f, 12.0f),
                    juce::Justification::centredLeft, false);
    }
    if (history.size() > 1)
    {
        juce::Path p;
        const float step = gr.getWidth() / 119.0f;
        const float x0 = gr.getRight() - step * (float) (history.size() - 1);
        bool started = false;
        for (size_t i = 0; i < history.size(); ++i)
        {
            const float v = juce::jlimit (lo, hi, history[i]);
            const float x = x0 + step * (float) i;
            const float y = gr.getBottom() - (v - lo) / (hi - lo) * gr.getHeight();
            if (! started) { p.startNewSubPath (x, y); started = true; }
            else p.lineTo (x, y);
        }
        g.setColour (amber.withAlpha (0.9f));
        g.strokePath (p, juce::PathStrokeType (1.6f));
    }

    // Gain reduction
    g.setColour (juce::Colour (0xffd5d8df));
    g.setFont (titleFont (11.0f).withExtraKerningFactor (0.14f));
    g.drawText ("GAIN REDUCTION  dB", juce::Rectangle<float> (d.getX(), d.getY() + 220.0f, d.getWidth(), 16.0f),
                juce::Justification::centred, false);
    drawBar (g, { d.getX() + 22.0f, d.getY() + 242.0f, d.getWidth() - 44.0f, 14.0f }, 24, grSmoothed, 0.0f, 12.0f, false,
             { "0", "3", "6", "9", "12" }, { 0.0f, 3.0f, 6.0f, 9.0f, 12.0f });

    // Pico de salida
    g.setColour (juce::Colour (0xffd5d8df));
    g.setFont (titleFont (11.0f).withExtraKerningFactor (0.14f));
    g.drawText ("OUTPUT PEAK  dBFS", juce::Rectangle<float> (d.getX(), d.getY() + 286.0f, d.getWidth(), 16.0f),
                juce::Justification::centred, false);
    drawBar (g, { d.getX() + 22.0f, d.getY() + 308.0f, d.getWidth() - 44.0f, 14.0f }, 30, peakSmoothed, -30.0f, 0.0f, true,
             { "-30", "-20", "-10", "-3", "0" }, { -30.0f, -20.0f, -10.0f, -3.0f, 0.0f });
}

void DarkLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();

    g.drawImageAt (texture, 0, 0);

    g.setColour (juce::Colours::white.withAlpha (0.08f));
    g.drawRoundedRectangle (8.0f, 8.0f, w - 16.0f, h - 16.0f, 6.0f, 1.2f);
    drawScrew (g, { 24.0f, 24.0f });
    drawScrew (g, { w - 24.0f, 24.0f });
    drawScrew (g, { 24.0f, h - 24.0f });
    drawScrew (g, { w - 24.0f, h - 24.0f });

    // Título grabado
    g.setFont (titleFont (24.0f).withExtraKerningFactor (0.08f));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawText ("DARK LIMITER", 51, 29, 360, 34, juce::Justification::centredLeft);
    g.setColour (title);
    g.drawText ("DARK LIMITER", 50, 28, 360, 34, juce::Justification::centredLeft);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.2f));
    g.drawText ("MASTERING LIMITER", 52, 64, 300, 16, juce::Justification::centredLeft);

    drawDisplay (g);

    // Faders: escalas y nombres
    drawFaderScale (g, thresholdFader, -20.0f, 0.0f, 2.0f, 4.0f, true);
    drawFaderScale (g, outputFader, -12.0f, 0.0f, 1.0f, 3.0f, false);

    auto drawFaderTitle = [&] (const juce::Slider& s, const juce::String& name, const juce::String& value)
    {
        const float cx = (float) s.getBounds().getCentreX();
        g.setColour (title);
        g.setFont (titleFont (14.0f).withExtraKerningFactor (0.1f));
        g.drawText (name, juce::Rectangle<float> (180.0f, 22.0f).withCentre ({ cx, 466.0f }), juce::Justification::centred, false);
        g.setColour (amber);
        g.setFont (numberFont (20.0f));
        g.drawText (value, juce::Rectangle<float> (120.0f, 24.0f).withCentre ({ cx, 492.0f }), juce::Justification::centred, false);
    };
    drawFaderTitle (thresholdFader, "THRESHOLD", juce::String (thresholdFader.getValue(), 1) + " dB");
    drawFaderTitle (outputFader, "OUTPUT", juce::String (outputFader.getValue(), 1) + " dB");

    g.setColour (juce::Colour (0xff6c7078));
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, h - 36.0f, w, 16.0f), juce::Justification::centred, false);
}
