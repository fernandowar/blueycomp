#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout BlueyCompAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "compress", 1 }, "Compress",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.35f));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "power", 1 }, "Power", true));

    return layout;
}

BlueyCompAudioProcessor::BlueyCompAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    compressParam = apvts.getRawParameterValue ("compress");
    powerParam    = apvts.getRawParameterValue ("power");
}

bool BlueyCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

void BlueyCompAudioProcessor::prepareToPlay (double sr, int)
{
    sampleRate = sr;
    attackCoeff  = 1.0f - std::exp (-1.0f / (0.001f * kAttackMs  * (float) sr));
    releaseCoeff = 1.0f - std::exp (-1.0f / (0.001f * kReleaseMs * (float) sr));
    envDb = 0.0f;

    driveDb.reset (sr, 0.03);
    driveDb.setCurrentAndTargetValue (compressParam->load() * kMaxDriveDb);
    mixPower.reset (sr, 0.02);
    mixPower.setCurrentAndTargetValue (powerParam->load() > 0.5f ? 1.0f : 0.0f);
}

void BlueyCompAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = buffer.getNumChannels();
    const int numSamples = buffer.getNumSamples();
    const float compress = compressParam->load();

    driveDb.setTargetValue (compress * kMaxDriveDb);
    mixPower.setTargetValue (powerParam->load() > 0.5f ? 1.0f : 0.0f);

    const float slope = 1.0f - 1.0f / kRatio;
    float peakGr = 0.0f;

    for (int i = 0; i < numSamples; ++i)
    {
        const float dDb = driveDb.getNextValue();
        const float power = mixPower.getNextValue();
        const float drive = juce::Decibels::decibelsToGain (dDb);

        // Detector enlazado (stereo link): el canal más fuerte manda
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = juce::jmax (peak, std::abs (buffer.getReadPointer (ch)[i]));

        const float levelDb = juce::Decibels::gainToDecibels (peak * drive, -120.0f);
        const float over = levelDb - kThreshDb;

        // Curva de ganancia con rodilla suave
        float targetGr = 0.0f;
        if (2.0f * over > kKneeDb)
            targetGr = slope * over;
        else if (2.0f * over > -kKneeDb)
        {
            const float x = over + kKneeDb * 0.5f;
            targetGr = slope * x * x / (2.0f * kKneeDb);
        }

        // Ataque / release (en dB, estilo FET)
        const float coeff = targetGr > envDb ? attackCoeff : releaseCoeff;
        envDb += coeff * (targetGr - envDb);
        peakGr = juce::jmax (peakGr, envDb);

        // Compensación automática de nivel: el knob sube entrada y baja salida
        const float makeupDb = -0.65f * dDb;
        const float totalGain = juce::Decibels::decibelsToGain (dDb - envDb + makeupDb);

        for (int ch = 0; ch < numCh; ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            const float dry = data[i];
            float wet = dry * totalGain;

            // Color FET suave, proporcional al knob (a 0 es transparente)
            wet += compress * 0.12f * (std::tanh (wet) - wet);

            data[i] = dry + power * (wet - dry);
        }
    }

    gainReductionDb.store (peakGr);
}

juce::AudioProcessorEditor* BlueyCompAudioProcessor::createEditor()
{
    return new BlueyCompAudioProcessorEditor (*this);
}

void BlueyCompAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void BlueyCompAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new BlueyCompAudioProcessor();
}
