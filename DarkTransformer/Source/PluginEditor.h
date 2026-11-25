#pragma once

#include "PluginProcessor.h"

//==============================================================================
// GUI de la línea Dark (familia de Dark Limiter / Dark Level): fondo grafito,
// tarjetas con borde fino, botones píldora con LED y acento propio en ámbar
// cobrizo. Perilla grande de DRIVE con arco de valor, pantalla de armónicos
// en vivo según el DRIVE y medidor del nivel que entra al hierro.
//==============================================================================
namespace TrColours
{
    static const juce::Colour bgTop  { 0xff181b21 };
    static const juce::Colour bgBot  { 0xff0c0d10 };
    static const juce::Colour panel  { 0xff14171c };
    static const juce::Colour screen { 0xff0a0c10 };
    static const juce::Colour line   { 0xff2a2f38 };
    static const juce::Colour track  { 0xff252a32 };
    static const juce::Colour ink    { 0xffeef1f6 };
    static const juce::Colour grey   { 0xff8a92a0 };
    static const juce::Colour dim    { 0xff4b5466 };
    static const juce::Colour accent { 0xffff9a3c };
    static const juce::Colour cream  { 0xffffe2bf };
}

class TransformerLookAndFeel : public juce::LookAndFeel_V4
{
public:
    static constexpr float startAngle = juce::MathConstants<float>::pi * 1.25f;
    static constexpr float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    void drawRotarySlider (juce::Graphics&, int x, int y, int width, int height,
                           float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                           juce::Slider&) override;
};

// Botón píldora con LED (igual que en Dark Limiter, con el acento ámbar)
class TrButton : public juce::ToggleButton
{
public:
    explicit TrButton (const juce::String& label = {}) { setButtonText (label); }
    void paintButton (juce::Graphics&, bool isMouseOver, bool isButtonDown) override;
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
    void analyseHarmonics();
    void drawCard (juce::Graphics&, juce::Rectangle<float> r, const juce::String& title);
    void drawReadout (juce::Graphics&, juce::Point<float> centre, const juce::String& value, const juce::String& name, float size);
    void drawHarmonics (juce::Graphics&);
    void drawMeter (juce::Graphics&);

    DarkTransformerAudioProcessor& processor;

    TransformerLookAndFeel laf;
    juce::Slider driveKnob, mixKnob, outputKnob;
    TrButton btnIn { "IN" };

    juce::AudioProcessorValueTreeState::SliderAttachment driveAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment mixAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment outputAttachment;
    juce::ParameterAttachment bypassAttachment;

    // Pantalla de armónicos: se mide el motor con un seno de 1 kHz a -18 dBFS + DRIVE
    iron::Channel analyser;
    std::array<float, 8> harmonicsDb {}; // índice 2..7 = H2..H7 en dBc
    float analysedDrive = -999.0f;
    int analyseFrames = 0;

    float level = -100.0f, peakHold = -100.0f;
    int peakHoldFrames = 0;

    juce::Rectangle<float> driveCard, sideCard, harmBox, meterBox;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DarkTransformerAudioProcessorEditor)
};
