#include "PluginEditor.h"
#include "DarkLimiterFonts.h"

using namespace LimColours;

namespace
{
    constexpr int kW = 800, kH = 500;

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

    g.setColour (juce::Colour (0xff1b2230));
    g.fillRoundedRectangle (cx - 2.5f, top - 3.0f, 5.0f, bottom - top + 6.0f, 2.5f);
    g.setColour (cyan.withAlpha (0.85f));
    g.fillRoundedRectangle (cx - 2.5f, sliderPos, 5.0f, bottom + 3.0f - sliderPos, 2.5f);

    juce::Rectangle<float> cap (cx - 20.0f, sliderPos - (float) thumbHalf, 40.0f, (float) thumbHalf * 2.0f);
    juce::DropShadow (juce::Colours::black.withAlpha (0.6f), 8, { 0, 3 }).drawForRectangle (g, cap.toNearestInt());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeef1f6), cap.getX(), cap.getY(),
                                             juce::Colour (0xffaab1bf), cap.getX(), cap.getBottom(), false));
    g.fillRoundedRectangle (cap, 5.0f);
    g.setColour (juce::Colour (0xff0b0e14));
    g.fillRoundedRectangle (cap.getX() + 6.0f, cap.getCentreY() - 1.2f, cap.getWidth() - 12.0f, 2.4f, 1.2f);
}

void LimButton::paintButton (juce::Graphics& g, bool over, bool)
{
    auto b = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();
    g.setColour (on ? cyan.withAlpha (0.16f) : (over ? panel.brighter (0.1f) : panel));
    g.fillRoundedRectangle (b, b.getHeight() * 0.5f);
    g.setColour (on ? cyan : line);
    g.drawRoundedRectangle (b, b.getHeight() * 0.5f, 1.3f);
    const auto led = juce::Point<float> (b.getX() + 15.0f, b.getCentreY());
    if (on) { g.setColour (cyan.withAlpha (0.3f)); g.fillEllipse (led.x - 6.0f, led.y - 6.0f, 12.0f, 12.0f); }
    g.setColour (on ? cyan : juce::Colour (0xff3a4355));
    g.fillEllipse (led.x - 3.5f, led.y - 3.5f, 7.0f, 7.0f);
    g.setColour (on ? ink : grey);
    g.setFont (numberFont (14.0f).withExtraKerningFactor (0.08f));
    g.drawText (getButtonText(), b.withTrimmedLeft (24.0f), juce::Justification::centred, false);
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
    setupFader (thresholdFader, -10.0);
    setupFader (outputFader, -0.2);

    btnTruePeak.onClick = [this] { truePeakAttachment.setValueAsCompleteGesture (btnTruePeak.getToggleState() ? 1.0f : 0.0f); };
    btnIn.onClick       = [this] { bypassAttachment.setValueAsCompleteGesture (btnIn.getToggleState() ? 0.0f : 1.0f); };
    addAndMakeVisible (btnTruePeak);
    addAndMakeVisible (btnIn);

    truePeakAttachment.sendInitialUpdate();
    bypassAttachment.sendInitialUpdate();

    setSize (kW, kH);
    startTimerHz (30);
}

DarkLimiterAudioProcessorEditor::~DarkLimiterAudioProcessorEditor()
{
    thresholdFader.setLookAndFeel (nullptr);
    outputFader.setLookAndFeel (nullptr);
}

void DarkLimiterAudioProcessorEditor::resized()
{
    thresholdFader.setBounds (50, 96, 60, 340);
    outputFader.setBounds (690, 96, 60, 340);
    graphBox = { 190.0f, 78.0f, 420.0f, 220.0f };
    tilesBox = { 190.0f, 316.0f, 420.0f, 104.0f };
    btnTruePeak.setBounds (kW - 266, 14, 140, 30);
    btnIn.setBounds (kW - 116, 14, 90, 30);
}

void DarkLimiterAudioProcessorEditor::timerCallback()
{
    const float grTarget = processor.gainReductionDb.load();
    grSmoothed += (grTarget > grSmoothed ? 0.6f : 0.12f) * (grTarget - grSmoothed);
    const float pk = processor.outputPeakDb.load();
    peakSmoothed += (pk > peakSmoothed ? 0.8f : 0.10f) * (pk - peakSmoothed);
    lufsShown = processor.lufsShortTerm.load();

    grHist.push_back (grTarget);
    pkHist.push_back (pk);
    while (grHist.size() > 300) grHist.pop_front();
    while (pkHist.size() > 300) pkHist.pop_front();
    repaint();
}

void DarkLimiterAudioProcessorEditor::drawFaderScale (juce::Graphics& g, const juce::Slider& s, float minV, float maxV,
                                                      float tickStep, float labelStep, bool labelsOnLeft)
{
    const auto b = s.getBounds().toFloat();
    const float cx = b.getCentreX();
    const float top = b.getY() + (float) LimiterLookAndFeel::thumbHalf;
    const float bottom = b.getBottom() - (float) LimiterLookAndFeel::thumbHalf;
    const float dir = labelsOnLeft ? -1.0f : 1.0f;

    g.setFont (numberFont (13.0f));
    const int steps = (int) std::round ((maxV - minV) / tickStep);
    for (int i = 0; i <= steps; ++i)
    {
        const float v = maxV - (float) i * tickStep;
        const float y = top + (maxV - v) / (maxV - minV) * (bottom - top);
        const bool major = std::abs (std::fmod (std::abs (v) + 0.001f, labelStep)) < 0.01f;
        g.setColour (grey.withAlpha (major ? 0.9f : 0.45f));
        const float x0 = cx + dir * 26.0f, x1 = x0 + dir * (major ? 8.0f : 4.0f);
        g.drawLine (x0, y, x1, y, 1.0f);
        if (major)
        {
            const float tx = x1 + dir * 3.0f;
            g.setColour (grey);
            g.drawText (juce::String ((int) std::round (v)),
                        juce::Rectangle<float> (labelsOnLeft ? tx - 30.0f : tx, y - 8.0f, 30.0f, 16.0f),
                        labelsOnLeft ? juce::Justification::centredRight : juce::Justification::centredLeft, false);
        }
    }
}

void DarkLimiterAudioProcessorEditor::drawVMeter (juce::Graphics& g, juce::Rectangle<float> r, float value,
                                                  float minV, float maxV, bool fromTop, juce::Colour c)
{
    g.setColour (juce::Colour (0xff1b2230));
    g.fillRoundedRectangle (r, 2.0f);
    const float t = juce::jlimit (0.0f, 1.0f, (value - minV) / (maxV - minV));
    const float hgt = r.getHeight() * t;
    g.setColour (c);
    if (fromTop) g.fillRoundedRectangle (r.withHeight (hgt), 2.0f);
    else         g.fillRoundedRectangle (r.withTrimmedTop (r.getHeight() - hgt), 2.0f);
}

void DarkLimiterAudioProcessorEditor::drawGraph (juce::Graphics& g)
{
    const auto d = graphBox;
    g.setColour (panel);
    g.fillRoundedRectangle (d, 8.0f);
    g.setColour (line);
    g.drawRoundedRectangle (d.reduced (0.5f), 8.0f, 1.0f);

    auto gr = d.reduced (12.0f, 12.0f).withTrimmedLeft (22.0f);
    constexpr float lo = -24.0f, hi = 0.0f;
    auto yOf = [&] (float db) { return gr.getBottom() - (juce::jlimit (lo, hi, db) - lo) / (hi - lo) * gr.getHeight(); };

    g.setFont (numberFont (11.0f));
    for (float v = -24.0f; v <= 0.0f; v += 6.0f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.06f));
        g.drawHorizontalLine ((int) yOf (v), gr.getX(), gr.getRight());
        g.setColour (grey);
        g.drawText (juce::String ((int) v), juce::Rectangle<float> (d.getX() + 4.0f, yOf (v) - 7.0f, 26.0f, 14.0f),
                    juce::Justification::centredLeft, false);
    }

    const int n = 300;
    const float step = gr.getWidth() / (float) (n - 1);
    const float x0 = gr.getRight() - step * (float) ((int) pkHist.size() - 1);

    if (pkHist.size() > 1)
    {
        juce::Path p;
        p.startNewSubPath (x0, gr.getBottom());
        for (size_t i = 0; i < pkHist.size(); ++i)
            p.lineTo (x0 + step * (float) i, yOf (pkHist[i]));
        p.lineTo (gr.getRight(), gr.getBottom());
        p.closeSubPath();
        g.setGradientFill (juce::ColourGradient (cyan.withAlpha (0.55f), 0.0f, gr.getY(),
                                                 cyan.withAlpha (0.08f), 0.0f, gr.getBottom(), false));
        g.fillPath (p);
    }
    if (grHist.size() > 1)
    {
        juce::Path p;
        p.startNewSubPath (x0, gr.getY());
        for (size_t i = 0; i < grHist.size(); ++i)
            p.lineTo (x0 + step * (float) i, gr.getY() + juce::jlimit (0.0f, 12.0f, grHist[i]) / 12.0f * gr.getHeight() * 0.5f);
        p.lineTo (gr.getRight(), gr.getY());
        p.closeSubPath();
        g.setColour (pink.withAlpha (0.55f));
        g.fillPath (p);
    }

    // línea del techo
    const float ceilY = yOf ((float) outputFader.getValue());
    g.setColour (ink.withAlpha (0.7f));
    const float dash[] = { 5.0f, 4.0f };
    g.drawDashedLine (juce::Line<float> (gr.getX(), ceilY, gr.getRight(), ceilY), dash, 2, 1.0f);

    g.setColour (grey);
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.15f));
    g.drawText ("OUTPUT PEAK", juce::Rectangle<float> (gr.getX() + 4.0f, gr.getBottom() - 16.0f, 100.0f, 14.0f), juce::Justification::centredLeft, false);
    g.setColour (pink);
    g.drawText ("GAIN REDUCTION", juce::Rectangle<float> (gr.getRight() - 124.0f, gr.getY() + 2.0f, 120.0f, 14.0f), juce::Justification::centredRight, false);
}

void DarkLimiterAudioProcessorEditor::drawTile (juce::Graphics& g, juce::Rectangle<float> r, const juce::String& title,
                                                const juce::String& value, const juce::String& unit, juce::Colour c)
{
    g.setColour (panel);
    g.fillRoundedRectangle (r, 8.0f);
    g.setColour (line);
    g.drawRoundedRectangle (r.reduced (0.5f), 8.0f, 1.0f);
    g.setColour (grey);
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.18f));
    g.drawText (title, r.withTrimmedTop (10.0f).withHeight (16.0f), juce::Justification::centred, false);
    g.setColour (c);
    g.setFont (numberFont (38.0f));
    g.drawText (value, r.withTrimmedTop (30.0f).withHeight (44.0f), juce::Justification::centred, false);
    g.setColour (grey);
    g.setFont (labelFont (12.0f).withExtraKerningFactor (0.15f));
    g.drawText (unit, r.withTrimmedTop (76.0f).withHeight (18.0f), juce::Justification::centred, false);
}

void DarkLimiterAudioProcessorEditor::paint (juce::Graphics& g)
{
    const float w = (float) getWidth(), h = (float) getHeight();
    g.setGradientFill (juce::ColourGradient (bg.brighter (0.04f), 0.0f, 0.0f, bg, 0.0f, h, false));
    g.fillAll();

    // barra superior
    g.setColour (juce::Colour (0xff10141c));
    g.fillRect (0.0f, 0.0f, w, 58.0f);
    g.setColour (line);
    g.drawHorizontalLine (58, 0.0f, w);
    g.setColour (cyan);
    g.fillRoundedRectangle (24.0f, 20.0f, 4.0f, 20.0f, 2.0f);
    g.setColour (ink);
    g.setFont (titleFont (17.0f).withExtraKerningFactor (0.1f));
    g.drawText ("DARK LIMITER", 38, 16, 260, 28, juce::Justification::centredLeft);
    g.setColour (grey);
    g.setFont (labelFont (11.0f).withExtraKerningFactor (0.25f));
    g.drawText ("MASTERING LIMITER", 262, 16, 200, 28, juce::Justification::centredLeft);

    drawGraph (g);

    const bool valid = lufsShown > -69.0f;
    const float tw = (tilesBox.getWidth() - 20.0f) / 3.0f;
    drawTile (g, tilesBox.withWidth (tw), "SHORT TERM", valid ? juce::String (lufsShown, 1) : juce::String ("-inf"), "LUFS", cyan);
    drawTile (g, tilesBox.withWidth (tw).translated (tw + 10.0f, 0.0f), "GAIN REDUCTION", juce::String (grSmoothed, 1), "dB", pink);
    drawTile (g, tilesBox.withWidth (tw).translated ((tw + 10.0f) * 2.0f, 0.0f), "OUTPUT PEAK",
              peakSmoothed > -69.0f ? juce::String (peakSmoothed, 1) : juce::String ("-inf"), "dBFS",
              peakSmoothed > -0.05f ? pink : green);

    // faders
    drawFaderScale (g, thresholdFader, -20.0f, 0.0f, 2.0f, 4.0f, true);
    drawFaderScale (g, outputFader, -12.0f, 0.0f, 1.0f, 3.0f, false);

    const float tY = (float) thresholdFader.getY() + (float) LimiterLookAndFeel::thumbHalf;
    const float tH = (float) thresholdFader.getHeight() - 2.0f * (float) LimiterLookAndFeel::thumbHalf;
    drawVMeter (g, { (float) thresholdFader.getRight() + 18.0f, tY, 6.0f, tH }, grSmoothed, 0.0f, 12.0f, true, pink);
    drawVMeter (g, { (float) outputFader.getX() - 24.0f, tY, 6.0f, tH }, peakSmoothed, -24.0f, 0.0f, false, peakSmoothed > -0.05f ? pink : cyan);

    auto drawFaderTitle = [&] (const juce::Slider& s, const juce::String& name, const juce::String& value)
    {
        const float cx = (float) s.getBounds().getCentreX();
        g.setColour (grey);
        g.setFont (labelFont (12.0f).withExtraKerningFactor (0.25f));
        g.drawText (name, juce::Rectangle<float> (140.0f, 18.0f).withCentre ({ cx, 76.0f }), juce::Justification::centred, false);
        g.setColour (ink);
        g.setFont (numberFont (20.0f));
        g.drawText (value, juce::Rectangle<float> (120.0f, 24.0f).withCentre ({ cx, 456.0f }), juce::Justification::centred, false);
    };
    drawFaderTitle (thresholdFader, "THRESHOLD", juce::String (thresholdFader.getValue(), 1) + " dB");
    drawFaderTitle (outputFader, "OUTPUT", juce::String (outputFader.getValue(), 1) + " dB");

    g.setColour (juce::Colour (0xff4b5466));
    g.setFont (labelFont (10.0f).withExtraKerningFactor (0.3f));
    g.drawText ("DARK SHAPE STUDIOS", juce::Rectangle<float> (0.0f, h - 28.0f, w, 14.0f), juce::Justification::centred, false);
}
