#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Frontal negro cepillado de la línea Dark (igual que Dark VCA), perillas
// estilo SSL, escalera de LEDs que muestra cuánto se está empujando el
// "hierro" y una barra inferior para cargar capturas .nam.
// Tipografías incluidas en el plugin: Michroma (títulos) y Barlow (números).
//==============================================================================
namespace DarkColours
{
    static const juce::Colour dot    { 0xfff0f1f4 };
    static const juce::Colour number { 0xffe4e6ec };
    static const juce::Colour title  { 0xffffffff };
    static const juce::Colour grey   { 0xff8d9098 };
    static const juce::Colour amber  { 0xffffb347 };
}

class DarkLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    static void paintKnob (juce::Graphics& g, juce::Point<float> centre, float radius, float proportion);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

// Botón rectangular gris (con LED, brillo naranja para "IN", o liso)
class DarkButton : public juce::ToggleButton
{
public:
    enum class Style { led, glow, plain };

    DarkButton (const juce::String& text, Style s, juce::Colour led = juce::Colours::transparentBlack)
        : ledColour (led), style (s)
    {
        setButtonText (text);
        setClickingTogglesState (s != Style::plain);
    }

    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
    juce::Colour ledColour;
    Style style;
};

//==============================================================================
class DarkTransformerAudioProcessorEditor : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit DarkTransformerAudioProcessorEditor (DarkTransformerAudioProcessor&);
    ~DarkTransformerAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void buildTexture();
    void chooseModel();
    void drawScale (juce::Graphics&, juce::Point<float> c, float dotRadius, float labelRadius,
                    const juce::StringArray& labels, float fontSize);
    void drawKnobTitle (juce::Graphics&, float cx, float y, const juce::String& name, const juce::String& unit);
    void drawScrew (juce::Graphics&, juce::Point<float> c);
    void drawLadder (juce::Graphics&);
    void drawModelBar (juce::Graphics&);

    DarkTransformerAudioProcessor& processor;

    DarkLookAndFeel laf;
    juce::Slider driveKnob, mixKnob, outputKnob;
    DarkButton btnIn { "IN", DarkButton::Style::glow };
    DarkButton btnLoad { "CARGAR", DarkButton::Style::plain };
    DarkButton btnDefault { "ORIGINAL", DarkButton::Style::plain };

    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment bypassAttachment;

    std::unique_ptr<juce::FileChooser> chooser;

    juce::Image texture;
    float levelSmoothed = -100.0f;
    juce::String shownModel, shownError;

    juce::Point<float> driveC, mixC, outC;
    juce::Rectangle<float> ladderBox, modelBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTransformerAudioProcessorEditor)
};
