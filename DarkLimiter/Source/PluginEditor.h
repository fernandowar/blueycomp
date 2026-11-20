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
    static const juce::Colour number { 0xffe4e6ec };
    static const juce::Colour title  { 0xffffffff };
    static const juce::Colour grey   { 0xff8d9098 };
    static const juce::Colour amber  { 0xffffb347 };
}

class LimiterLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr int thumbHalf = 15; // mitad de la altura de la tapa del fader

    int getSliderThumbRadius (juce::Slider&) override { return thumbHalf; }

    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
};

// Botón rectangular gris con LED, o brillo naranja (para "IN")
class LimButton : public juce::ToggleButton
{
public:
    LimButton (const juce::String& text, juce::Colour led, bool glowWhenOn = false)
        : ledColour (led), glow (glowWhenOn)
    {
        setButtonText (text);
        setClickingTogglesState (true);
    }

    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    juce::Colour ledColour;
    bool glow;
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
    void buildTexture();
    void drawScrew (juce::Graphics&, juce::Point<float> c);
    void drawFaderScale (juce::Graphics&, const juce::Slider&, float minV, float maxV,
                         float tickStep, float labelStep, bool labelsOnLeft);
    void drawDisplay (juce::Graphics&);
    void drawBar (juce::Graphics&, juce::Rectangle<float> area, int segs, float value, float minV, float maxV,
                  bool greenToRed, const juce::StringArray& labels, const juce::Array<float>& labelValues);

    DarkLimiterAudioProcessor& processor;

    LimiterLookAndFeel laf;
    juce::Slider thresholdFader, outputFader;
    LimButton btnTruePeak { "TRUE PEAK", juce::Colour (0xff46e07a) };
    LimButton btnIn { "IN", juce::Colours::transparentBlack, true };

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment truePeakAttachment;
    juce::ParameterAttachment bypassAttachment;

    juce::Image texture;
    float grSmoothed = 0.0f, peakSmoothed = -100.0f, lufsShown = -100.0f;
    int historyTick = 0;
    std::deque<float> history; // LUFS cada 0.5 s (60 s)

    juce::Rectangle<float> displayBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLimiterAudioProcessorEditor)
};
