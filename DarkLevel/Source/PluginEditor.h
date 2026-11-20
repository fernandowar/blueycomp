#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Frontal negro cepillado estilo Bluey (bisel, tornillos, título grabado) con
// perillas estilo SSL (tapa gris metálica, puntero oscuro, escala de puntos
// blancos) y escalera horizontal de LEDs de gain reduction (3/6/10/14/20 dB).
// Tipografías incluidas en el plugin: Michroma (títulos) y Barlow (números).
//==============================================================================
namespace SslColours
{
    static const juce::Colour dot    { 0xfff0f1f4 };
    static const juce::Colour number { 0xffe4e6ec };
    static const juce::Colour title  { 0xffffffff };
    static const juce::Colour grey   { 0xff8d9098 };
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
class DarkLevelAudioProcessorEditor : public juce::AudioProcessorEditor,
                                      private juce::Timer
{
public:
    explicit DarkLevelAudioProcessorEditor (DarkLevelAudioProcessor&);
    ~DarkLevelAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildTexture();
    void drawScale (juce::Graphics&, juce::Point<float> c, float dotRadius, float labelRadius,
                    const juce::StringArray& labels, float fontSize);
    void drawKnobTitle (juce::Graphics&, float cx, float y, const juce::String& name, const juce::String& unit);
    void drawScrew (juce::Graphics&, juce::Point<float> c);
    void drawMeter (juce::Graphics&);

    DarkLevelAudioProcessor& processor;

    SslLookAndFeel laf;
    juce::Slider gainKnob;
    SslButton btnAuto { "AUTO", juce::Colour (0xff46e07a) };
    SslButton btnIn { "IN", juce::Colours::transparentBlack, true };

    juce::AudioProcessorValueTreeState::SliderAttachment gainAttachment;
    juce::ParameterAttachment autoAttachment;
    juce::ParameterAttachment bypassAttachment;

    juce::Image texture;
    float peakNow = -100.0f, holdDb = -100.0f;
    int holdAge = 0;

    juce::Point<float> knobC;
    juce::Rectangle<float> meterBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkLevelAudioProcessorEditor)
};
