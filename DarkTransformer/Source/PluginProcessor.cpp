#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "DarkTransformerData.h"

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

    loadDefaultModel();
    startTimerHz (4);
}

DarkTransformerAudioProcessor::~DarkTransformerAudioProcessor()
{
    stopTimer();
    delete pending.exchange (nullptr);
    delete retired.exchange (nullptr);
    delete active;
}

bool DarkTransformerAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in  = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    if (in != out) return false;
    return in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo();
}

//==============================================================================
void DarkTransformerAudioProcessor::installModel (std::unique_ptr<TransformerModel> model)
{
    modelName = model->getName();
    modelNeural = model->isNeural();

    if (currentRate > 0.0)
        model->prepare (currentRate, currentBlock);

    if (active == nullptr)
    {
        active = model.release(); // todavía no hay audio corriendo
    }
    else
    {
        const int newLatency = model->getLatencySamples();
        delete pending.exchange (model.release()); // si había otro esperando, nunca llegó al audio
        setLatencySamples (newLatency);
        return;
    }
    setLatencySamples (active->getLatencySamples());
}

bool DarkTransformerAudioProcessor::loadModelFile (const juce::File& file)
{
    if (! file.existsAsFile())
    {
        lastError = "No se encontró el archivo: " + file.getFileName();
        return false;
    }

    juce::String error;
    auto model = TransformerModel::fromNam (file.loadFileAsString(), file.getFileNameWithoutExtension(), error);
    if (model == nullptr)
    {
        lastError = error;
        return false;
    }

    lastError = {};
    apvts.state.setProperty ("modelPath", file.getFullPathName(), nullptr);
    installModel (std::move (model));
    return true;
}

void DarkTransformerAudioProcessor::loadDefaultModel()
{
    apvts.state.setProperty ("modelPath", juce::String(), nullptr);
    lastError = {};

#if DT_EMBEDDED_MODEL
    for (int i = 0; i < DarkTransformerData::namedResourceListSize; ++i)
    {
        const juce::String original (DarkTransformerData::originalFilenames[i]);
        if (! original.endsWithIgnoreCase (".nam"))
            continue;

        int size = 0;
        if (const char* data = DarkTransformerData::getNamedResource (DarkTransformerData::namedResourceList[i], size))
        {
            juce::String error;
            auto model = TransformerModel::fromNam (juce::String::fromUTF8 (data, size), DT_EMBEDDED_MODEL_NAME, error);
            if (model != nullptr)
            {
                installModel (std::move (model));
                return;
            }
            lastError = error;
        }
    }
#endif

    installModel (TransformerModel::makeFallback());
}

void DarkTransformerAudioProcessor::timerCallback()
{
    delete retired.exchange (nullptr);
}

//==============================================================================
void DarkTransformerAudioProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    currentRate = sr;
    currentBlock = juce::jmax (1, samplesPerBlock);

    // Aquí el audio está detenido: se puede ordenar sin carreras
    if (auto* p = pending.exchange (nullptr))
    {
        delete active;
        active = p;
    }
    delete retired.exchange (nullptr);

    if (active != nullptr)
    {
        active->prepare (sr, currentBlock);
        setLatencySamples (active->getLatencySamples());
    }

    dryBuffer.setSize (2, currentBlock, false, true, false);
    driveGains.assign ((size_t) currentBlock, 1.0f);

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
    if (numCh == 0 || total == 0) return;

    // ¿Hay un modelo nuevo? Se cambia solo si el anterior ya fue recogido
    if (pending.load() != nullptr && retired.load() == nullptr)
        if (auto* p = pending.exchange (nullptr))
        {
            retired.store (active);
            active = p;
        }

    if (active == nullptr) return;

    driveGain.setTargetValue (juce::Decibels::decibelsToGain (driveParam->load()));
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (outputParam->load()));
    mixAmt.setTargetValue (mixParam->load() * 0.01f);
    activeAmt.setTargetValue (bypassParam->load() > 0.5f ? 0.0f : 1.0f);

    float peak = 0.0f;

    // Se procesa en trozos del tamaño preparado (algunos hosts mandan bloques más grandes)
    for (int start = 0; start < total; start += currentBlock)
    {
        const int n = juce::jmin (currentBlock, total - start);

        float* io[2] = { buffer.getWritePointer (0, start),
                         buffer.getWritePointer (numCh > 1 ? 1 : 0, start) };
        float* dry[2] = { dryBuffer.getWritePointer (0), dryBuffer.getWritePointer (1) };

        // 1) copia del seco + DRIVE hacia el transformador
        for (int i = 0; i < n; ++i)
        {
            const float g = driveGain.getNextValue();
            driveGains[(size_t) i] = g;
            for (int ch = 0; ch < numCh; ++ch)
            {
                dry[ch][i] = io[ch][i];
                io[ch][i] *= g;
                peak = juce::jmax (peak, std::abs (io[ch][i]));
            }
        }

        // 2) el transformador (captura NAM o respaldo DSP)
        active->process (io, numCh, n);

        // 3) seco alineado con la latencia del camino húmedo
        active->delayDry (dry, numCh, n);

        // 4) compensación del DRIVE, MIX, OUTPUT y bypass sin clics
        for (int i = 0; i < n; ++i)
        {
            const float invDrive = 1.0f / driveGains[(size_t) i];
            const float mix = mixAmt.getNextValue();
            const float out = outputGain.getNextValue();
            const float act = activeAmt.getNextValue();
            for (int ch = 0; ch < numCh; ++ch)
            {
                const float d = dry[ch][i];
                const float w = io[ch][i] * invDrive;
                const float processed = (d + mix * (w - d)) * out;
                io[ch][i] = d + act * (processed - d);
            }
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
    auto xml = getXmlFromBinary (data, sizeInBytes);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return;

    const juce::String previousPath = apvts.state.getProperty ("modelPath").toString();
    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    const juce::String path = apvts.state.getProperty ("modelPath").toString();

    if (path == previousPath && (path.isNotEmpty() || active != nullptr))
        return;

    if (path.isEmpty() || ! loadModelFile (juce::File (path)))
        loadDefaultModel(); // si la captura externa ya no está, vuelve a la incluida
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DarkTransformerAudioProcessor();
}
