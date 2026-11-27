#pragma once

#include "PluginProcessor.h"
#include "Dark4kDsp.h"
#include <vector>

//==============================================================================
// Frontal estilo canal SSL 4000: EQ a la izquierda, fader y medidores al centro y el Dark VCA
// (compresor) a la derecha. Perillas SSL con tapa de color; botones grises con LED.
//==============================================================================
namespace D4kColours
{
    static const juce::Colour dot    { 0xfff0f1f4 };
    static const juce::Colour number { 0xffe4e6ec };
    static const juce::Colour title  { 0xffffffff };
    static const juce::Colour grey   { 0xff8d9098 };
    static const juce::Colour amber  { 0xffffb347 };
    static const juce::Colour red    { 0xffd8343f };
    static const juce::Colour green  { 0xff2faa7c };
    static const juce::Colour blue   { 0xff3f7fd0 };
    static const juce::Colour brown  { 0xff1b1c20 };
    static const juce::Colour white  { 0xffdcdde1 };
    static const juce::Colour silver { 0xffb4b7be };
}

class D4kLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;
    static constexpr int faderThumbHalf = 20;

    static void paintKnob (juce::Graphics& g, juce::Point<float> centre, float radius, float proportion, juce::Colour cap);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
    void drawLinearSlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           juce::Slider::SliderStyle, juce::Slider&) override;
    int getSliderThumbRadius (juce::Slider&) override { return faderThumbHalf; }
};

class D4kButton : public juce::ToggleButton
{
public:
    D4kButton (const juce::String& text, juce::Colour led, bool glowWhenOn = false)
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
class Dark4kAudioProcessorEditor : public juce::AudioProcessorEditor,
                                   private juce::Timer
{
public:
    explicit Dark4kAudioProcessorEditor (Dark4kAudioProcessor&);
    ~Dark4kAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    struct Knob
    {
        juce::Slider slider;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        juce::String title, unit;
        std::vector<std::pair<double, juce::String>> marks;
        float scaleR = 0, labelR = 0, fontSize = 10.0f;
    };
    struct Toggle
    {
        std::unique_ptr<D4kButton> button;
        std::unique_ptr<juce::ParameterAttachment> att;
    };

    Knob& addKnob (const juce::String& id, const juce::String& title, const juce::String& unit,
                   juce::Colour cap, double reset, std::vector<std::pair<double, juce::String>> marks, bool discrete = false);
    D4kButton& addToggle (const juce::String& id, const juce::String& text, juce::Colour led, bool glow = false, bool invert = false);

    void timerCallback() override;
    void buildTexture();
    void drawScrew (juce::Graphics&, juce::Point<float> c);
    void drawKnobScale (juce::Graphics&, Knob&);
    void drawMeters (juce::Graphics&, juce::Rectangle<float> r, const float* levels, const float* holds, const juce::String& title);
    void drawLadder (juce::Graphics&, juce::Rectangle<float>);
    void drawFaderScale (juce::Graphics&);
    void drawEqScreen (juce::Graphics&, juce::Rectangle<float>);
    static float cLx() { return 238.0f * 1.5f; }
    static float cW() { return (395.0f - 238.0f) * 1.5f; }

    Dark4kAudioProcessor& processor;
    D4kLookAndFeel laf;

    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<std::unique_ptr<Toggle>> toggles;
    juce::Slider fader;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> faderAtt;
    juce::ParameterAttachment releaseAtt;
    D4kButton btnAuto { "AUTO", juce::Colours::transparentBlack, true };

    juce::Image texture;
    float inLvl[2] { -100, -100 }, outLvl[2] { -100, -100 }, inHold[2] { -100, -100 }, outHold[2] { -100, -100 };
    int holdAge[4] { 0, 0, 0, 0 };
    float grSmoothed = 0.0f;
    float clipAge[4] { 0, 0, 0, 0 };

    juce::Rectangle<float> inMeterBox, outMeterBox, ladderBox, chDisplay, eqScreen;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Dark4kAudioProcessorEditor)
};
