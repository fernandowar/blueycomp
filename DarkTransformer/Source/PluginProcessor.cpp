#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::AudioProcessorValueTreeState::ParameterLayout DarkTransformerAudioProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "drive", 1 }, "Drive",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix",
        juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (-18.0f, 18.0f, 0.1f), 0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID { "bypass", 1 }, "Bypass", false));

    return layout;
}

DarkTransformerAudioProcessor::DarkTransformerAudioProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    driveParam  = apvts.getRawParameterValue ("drive");
    mixParam    = apvts.getRawParameterValue ("mix");
    outputParam = apvts.getRawParameterValue ("output");
    bypassParam = apvts.getRawParameterValue ("bypass");
}

bool DarkTransformerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

//==============================================================================
void DarkTransformerAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    blockSize = juce::jmax (1, samplesPerBlock);

    // Filtros de media banda FIR de fase lineal: latencia entera y sin giro de
    // fase, para que el MIX en paralelo no haga filtro peine
    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        2, kOversamplingOrder, juce::dsp::Oversampling<float>::filterHalfBandFIREquiripple, true, true);
    oversampling->initProcessing ((size_t) blockSize);
    latency = (int) std::lround (oversampling->getLatencyInSamples());
    setLatencySamples (latency);

    const double osRate = sr * (double) (1 << kOversamplingOrder);
    for (int ch = 0; ch < 2; ++ch)
        channels[(size_t) ch].prepare (sr, osRate, ch);

    dryBuffer.setSize (2, blockSize, false, true, false);
    driveGains.assign ((size_t) blockSize, 1.0f);
    for (auto& l : dryLine)
        l.assign ((size_t) latency + 1, 0.0f);
    gainLine.assign ((size_t) latency + 1, 1.0f);
    linePos = 0;

    driveGain.reset (sr, 0.05);
    driveGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    outputGain.reset (sr, 0.05);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    mixAmt.reset (sr, 0.05);
    mixAmt.setCurrentAndTargetValue (mixParam->load() * 0.01f);
    activeAmt.reset (sr, 0.02);
    activeAmt.setCurrentAndTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);
}

void DarkTransformerAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numCh = juce::jmin (buffer.getNumChannels(), 2);
    const int total = buffer.getNumSamples();
    if (numCh == 0 || total == 0 || oversampling == nullptr) return;

    driveGain.setTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    mixAmt.setTargetValue (mixParam->load() * 0.01f);
    activeAmt.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);

    const int lineLen = latency + 1;
    float peak = 0.0f;

    for (int start = 0; start < total; start += blockSize)
    {
        const int n = juce::jmin (blockSize, total - start);

        float* io[2] = { buffer.getWritePointer (0, start),
                         buffer.getWritePointer (numCh > 1 ? 1 : 0, start) };
        float* dry[2] = { dryBuffer.getWritePointer (0), dryBuffer.getWritePointer (1) };

        // 1) seco, DRIVE y corte de graves del transformador de entrada
        for (int i = 0; i < n; ++i)
        {
            const float g = driveGain.getNextValue();
            driveGains[(size_t) i] = g;
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float in = io[ch][i];
                dry[ch][i] = in;
                io[ch][i] = channels[(size_t) ch].processPre (in * g);
                peak = juce::jmax (peak, std::abs (in * g));
            }
        }

        // 2) núcleo, etapa de ganancia y trafo de salida a 4x
        {
            juce::dsp::AudioBlock<float> block (io, (size_t) numCh, (size_t) n);
            auto up = oversampling->processSamplesUp (block);
            for (size_t ch = 0; ch < up.getNumChannels(); ++ch)
            {
                auto* d = up.getChannelPointer (ch);
                auto& engine = channels[ch];
                for (size_t i = 0; i < up.getNumSamples(); ++i)
                    d[i] = engine.processOversampled (d[i]);
            }
            oversampling->processSamplesDown (block);
        }

        // 3) curva de profundidad, compensación, MIX alineado, OUTPUT y bypass
        for (int i = 0; i < n; ++i)
        {
            const int readPos = (linePos + 1) % lineLen;
            gainLine[(size_t) linePos] = driveGains[(size_t) i];
            const float invDrive = 1.0f / gainLine[(size_t) readPos];

            const float mix = mixAmt.getNextValue();
            const float out = outputGain.getNextValue();
            const float act = activeAmt.getNextValue();

            for (int ch = 0; ch < numCh; ++ch)
            {
                auto& line = dryLine[(size_t) ch];
                line[(size_t) linePos] = dry[ch][i];
                const float d = line[(size_t) readPos];

                float w = channels[(size_t) ch].processPost (io[ch][i]) * invDrive;
                w = std::isfinite (w) ? w : 0.0f;

                const float processed = (d + mix * (w - d)) * out;
                io[ch][i] = d + act * (processed - d);
            }
            linePos = readPos;
        }
    }

    ironLevelDb.store (activeAmt.getTargetValue() > 0.5f ? juce::Decibels::gainToDecibels (peak, -100.0f) : -100.0f);
}

//==============================================================================
juce::AudioProcessorEditor* DarkTransformerAudioProcessor::createEditor()
{
    return new DarkTransformerAudioProcessorEditor (*this);
}

void DarkTransformerAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void DarkTransformerAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkTransformerAudioProcessor();
}
