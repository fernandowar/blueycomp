#pragma once

#include "PluginProcessor.h"

//==============================================================================
// Estética "hierro vintage": frontal verde martillado (hammertone), placa de
// cobre con el título grabado, VU analógico de aguja para el nivel que entra
// al transformador, perillas de baquelita con tapa de color, lámpara de
// piloto para el IN y una placa con las 6 voces (transformadores y preamps).
// Tipografías incluidas en el plugin: Michroma (títulos) y Barlow (números).
//==============================================================================
namespace IronColours
{
    static const juce::Colour cream     { 0xffeee4c8 };
    static const juce::Colour creamDim  { 0xffb9ad8e };
    static const juce::Colour copperHi  { 0xffd89a5c };
    static const juce::Colour copper    { 0xffb8743a };
    static const juce::Colour copperLo  { 0xff6e4020 };
    static const juce::Colour engrave   { 0xff2a170a };
    static const juce::Colour panel     { 0xff22302d };
    static const juce::Colour capRed    { 0xffb3261e };
    static const juce::Colour capGrey   { 0xff7d8584 };
    static const juce::Colour lamp      { 0xffffa02a };
}

class IronLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    // Perilla de baquelita negra con tapa de color brillante
    static void paintKnob (juce::Graphics& g, juce::Point<float> centre, float radius,
                           float proportion, juce::Colour capColour);

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

// Lámpara de piloto (IN) o tecla marfil con LED para elegir la voz
class IronButton : public juce::ToggleButton
{
public:
    enum class Style { lamp, key };

    IronButton (const juce::String& text = {}, Style s = Style::key) : style (s)
    {
        setButtonText (text);
        setClickingTogglesState (s == Style::lamp);
    }

    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;

private:
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
    void drawScale (juce::Graphics&, juce::Point<float> c, float tickInner, float tickOuter, float labelRadius,
                    const juce::StringArray& labels, float fontSize);
    void drawKnobTitle (juce::Graphics&, float cx, float y, const juce::String& name, const juce::String& unit);
    void drawScrew (juce::Graphics&, juce::Point<float> c);
    void drawHeader (juce::Graphics&);
    void drawTransformerSymbol (juce::Graphics&, juce::Rectangle<float> area, juce::Colour colour);
    void drawMeter (juce::Graphics&);
    void drawVoicePlate (juce::Graphics&);

    DarkTransformerAudioProcessor& processor;

    IronLookAndFeel laf;
    juce::Slider driveKnob, mixKnob, outputKnob;
    IronButton btnIn { "IN", IronButton::Style::lamp };
    std::array<IronButton, iron::numVoices> voiceKeys;

    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment bypassAttachment;
    juce::ParameterAttachment voiceAttachment;

    juce::Image texture;
    float needle = 0.0f;   // posición de la aguja (0..1)
    float peakHold = 0.0f; // LED de pico sobre 0 dBFS (decae)

    juce::Point<float> driveC, mixC, outC;
    juce::Rectangle<float> headerBox, meterBox, plateBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTransformerAudioProcessorEditor)
};
