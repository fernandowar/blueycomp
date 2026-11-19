#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética: azul marino oscuro, perillas negras con anillo naranja, power cyan
//==============================================================================
namespace BlueyColours
{
    static const juce::Colour orange   { 0xffff5a1f };
    static const juce::Colour cyan     { 0xff22c7e8 };
    static const juce::Colour label    { 0xff8d93a8 };
    static const juce::Colour bgTop    { 0xff2b3459 };
    static const juce::Colour bgBottom { 0xff1a1f36 };
}

class BlueyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    // Dibuja una perilla; también la usan los controles fijos decorativos
    static void paintKnob (juce::Graphics& g, juce::Rectangle<float> bounds, float proportion);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

class PowerButton : public juce::ToggleButton
{
public:
    PowerButton() { setClickingTogglesState (true); }
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
};

//==============================================================================
class BlueyCompAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit BlueyCompAudioProcessorEditor (BlueyCompAudioProcessor&);
    ~BlueyCompAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    BlueyCompAudioProcessor& processor;

    BlueyLookAndFeel laf;
    juce::Slider compressKnob;
    PowerButton powerButton;

    juce::AudioProcessorValueTreeState::SliderAttachment compressAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;

    float grSmoothed = 0.0f;

    juce::Rectangle<float> bigKnobArea, bottomPanel, grilleArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueyCompAudioProcessorEditor)
};
