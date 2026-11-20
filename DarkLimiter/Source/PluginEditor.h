#pragma once

#include "PluginProcessor.h"
#include <deque>

//==============================================================================
// Frontal negro cepillado (como Bluey / Dark VCA): dos faders (THRESHOLD de entrada y
// OUTPUT = techo), pantalla central con LUFS de corto plazo, historial, medidores de
// gain reduction y pico de salida, y botones TRUE PEAK / IN.
//==============================================================================
namespace LimColours
{
    static const juce::Colour bg     { 0xff0c0f15 };
    static const juce::Colour panel  { 0xff131822 };
    static const juce::Colour line   { 0xff262d3a };
    static const juce::Colour ink    { 0xffe6eaf2 };
    static const juce::Colour grey   { 0xff7d8697 };
    static const juce::Colour cyan   { 0xff2fd0ff };
    static const juce::Colour pink   { 0xffff4f8b };
    static const juce::Colour green  { 0xff3ddc8a };
}

class LimiterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr int thumbHalf = 11;
    int getSliderThumbRadius (juce::Slider&) override { return thumbHalf; }
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

// Botón plano estilo Ozone (píldora con acento cian)
class LimButton : public juce::ToggleButton
{
public:
    explicit LimButton (const juce::String& t) { setButtonText (t); setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

//==============================================================================
class DarkLimiterAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit DarkLimiterAudioProcessorEditor (DarkLimiterAudioProcessor&);
    ~DarkLimiterAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawFaderScale (juce::Graphics&, const juce::Slider&, float minV, float maxV,
                         float tickStep, float labelStep, bool labelsOnLeft);
    void drawGraph (juce::Graphics&);
    void drawVMeter (juce::Graphics&, juce::Rectangle<float> r, float value, float minV, float maxV, bool fromTop, juce::Colour c);
    void drawTile (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title, const juce::String& value,
                   const juce::String& unit, juce::Colour c);

    DarkLimiterAudioProcessor& processor;

    LimiterLookAndFeel laf;
    juce::Slider thresholdFader, outputFader;
    LimButton btnTruePeak { "TRUE PEAK" };
    LimButton btnUnlink { "UNLINK" };
    LimButton btnIn { "ON" };

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment truePeakAttachment;
    juce::ParameterAttachment unlinkAttachment;
    juce::ParameterAttachment bypassAttachment;

    float grSmoothed = 0.0f, peakSmoothed = -100.0f, lufsShown = -100.0f;
    std::deque<float> grHist, pkHist; // 30 Hz, ~10 s

    juce::Rectangle<float> graphBox, tilesBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLimiterAudioProcessorEditor)
};
