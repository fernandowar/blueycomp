#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética 1176 Rev A "blue stripe": frontal plateado cepillado con panel azul superior
// (título + medidor de barras), perillas negras estriadas con tapa de aluminio y
// escalas grabadas en negro.
//==============================================================================
namespace BlueyColours
{
    static const juce::Colour white    { 0xffe8e8e6 };
    static const juce::Colour grey     { 0xff9a9da3 };
    static const juce::Colour amber    { 0xffffb347 };
    static const juce::Colour ledOn    { 0xffff7a2f };
    static const juce::Colour ledOff   { 0xff15181d };
    static const juce::Colour cyan     { 0xff22c7e8 };
    static const juce::Colour ink      { 0xff15181d }; // texto grabado sobre el frontal plateado
    static const juce::Colour paleBlue { 0xffd2e6fa }; // texto claro sobre el panel azul
}

class BlueyLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    // Perilla tipo 1176: cuerpo negro estriado + tapa de aluminio cepillado + anillo
    static void paint1176Knob (juce::Graphics& g, juce::Point<float> centre, float radius, float proportion);

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
    void buildTexture();
    void drawKnobScale (juce::Graphics&, juce::Point<float> c, float tickStart, float majorLen, float minorLen,
                        float labelRadius, const juce::StringArray& majors, int minorsBetween, float fontSize,
                        juce::Colour colour);
    void drawBarMeter (juce::Graphics&);
    void drawScrew (juce::Graphics&, juce::Point<float> c);

    BlueyCompAudioProcessor& processor;

    BlueyLookAndFeel laf;
    juce::Slider compressKnob, outputKnob;
    PowerButton powerButton;

    juce::AudioProcessorValueTreeState::SliderAttachment compressAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment bypassAttachment;

    juce::Image texture;
    float grSmoothed = 0.0f;

    juce::Point<float> bigC, outC;
    juce::Rectangle<float> meterArea;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (BlueyCompAudioProcessorEditor)
};
