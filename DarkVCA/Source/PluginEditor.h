#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética consola SSL E: panel azul marino texturizado, perillas crema con
// puntero oscuro, escalas de puntos con números, botones grises y escalera de
// LEDs de gain reduction (3 / 6 / 10 / 14 / 20 dB).
//==============================================================================
namespace SslColours
{
    static const juce::Colour dot    { 0xff9aa3bb };
    static const juce::Colour number { 0xffaab2c8 };
    static const juce::Colour title  { 0xffc4c8d4 };
    static const juce::Colour cream  { 0xffefede4 };
    static const juce::Colour amber  { 0xffffb347 };
}

class SslLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    static void paintKnob (juce::Graphics& g, juce::Point<float> centre, float radius, float proportion);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

// Botón rectangular gris estilo SSL, con LED (o brillo naranja para "IN")
class SslButton : public juce::ToggleButton
{
public:
    SslButton (const juce::String& text, juce::Colour led, bool glowWhenOn = false)
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
class DarkVcaAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit DarkVcaAudioProcessorEditor (DarkVcaAudioProcessor&);
    ~DarkVcaAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildTexture();
    void drawScale (juce::Graphics&, juce::Point<float> c, float dotRadius, float labelRadius,
                    const juce::StringArray& labels, float fontSize);
    void drawKnobTitle (juce::Graphics&, juce::Point<float> c, float labelRadius,
                        const juce::String& name, const juce::String& unit);
    void drawLadder (juce::Graphics&);

    DarkVcaAudioProcessor& processor;

    SslLookAndFeel laf;
    juce::Slider thresholdKnob, attackKnob, makeupKnob;
    SslButton btn100 { "100 ms", juce::Colour (0xff46e07a) };
    SslButton btnAuto { "AUTO", juce::Colour (0xff46e07a) };
    SslButton btnIn { "IN", juce::Colours::transparentBlack, true };

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment attackAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment makeupAttachment;
    juce::ParameterAttachment releaseAttachment;
    juce::ParameterAttachment bypassAttachment;

    juce::Image texture;
    float grSmoothed = 0.0f;

    juce::Point<float> thrC, atkC, mkC;
    juce::Rectangle<float> ladderBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkVcaAudioProcessorEditor)
};
