#include "PluginProcessor.h"
#include "PluginEditor.h"

ProtectedSoundsAudioProcessor::ProtectedSoundsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
    : AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
    ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{
    // Registrar formatos de audio básicos
    mFormatManager.registerBasicFormats();
    mFormatManager2.registerBasicFormats();
    
    // Configurar limitador
    limiter.setThreshold(0.0f);
    limiter.setRelease(100.0f);

    // Escuchar cambios en parámetros
    apvts.state.addListener(this);
    
    // Inicializar voces para todos los samplers
    for(int i = 0; i < mNumVoices; i++) {
        mSampler1Clean.addVoice(new juce::SamplerVoice());
        mSampler1Excited.addVoice(new juce::SamplerVoice());
        mSampler2Clean.addVoice(new juce::SamplerVoice());
        mSampler2Excited.addVoice(new juce::SamplerVoice());
    }
}

ProtectedSoundsAudioProcessor::~ProtectedSoundsAudioProcessor()
{
    apvts.state.removeListener(this);
    mFormatReader = nullptr;
    mFormatReader2 = nullptr;
}

// ============================================================================
// MÉTODOS OBLIGATORIOS DEL AudioProcessor
// ============================================================================

const juce::String ProtectedSoundsAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool ProtectedSoundsAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool ProtectedSoundsAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool ProtectedSoundsAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double ProtectedSoundsAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int ProtectedSoundsAudioProcessor::getNumPrograms() { return 1; }
int ProtectedSoundsAudioProcessor::getCurrentProgram() { return 0; }
void ProtectedSoundsAudioProcessor::setCurrentProgram(int index) {}
const juce::String ProtectedSoundsAudioProcessor::getProgramName(int index) { return {}; }
void ProtectedSoundsAudioProcessor::changeProgramName(int index, const juce::String& newName) {}

// ============================================================================
// PREPARACIÓN Y CONFIGURACIÓN
// ============================================================================

void ProtectedSoundsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    // Configurar sample rate para todos los samplers
    mSampler1Clean.setCurrentPlaybackSampleRate(sampleRate);
    mSampler1Excited.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2Clean.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2Excited.setCurrentPlaybackSampleRate(sampleRate);
    
    // Preparar buffer temporal
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    
    updateADSR();
    
    // Configurar DSP (filtro y limitador)
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    filter.setCutoffFrequency(filterFrequency);
    filter.setResonance(filterResonance);
    
    limiter.prepare(spec);
}

void ProtectedSoundsAudioProcessor::releaseResources() {}

bool ProtectedSoundsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    return true;
}

// ============================================================================
// PROCESAMIENTO DE AUDIO PRINCIPAL
// ============================================================================

void ProtectedSoundsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Buffer para procesar mensajes MIDI (incluyendo loops artificiales)
    juce::MidiBuffer processedMidi;
    
    // ========================================================================
    // PROCESAMIENTO DE MENSAJES MIDI Y GESTIÓN DEL LOOP
    // ========================================================================
    
    // Copiar mensajes MIDI originales y procesar note on/off
    for (const auto metadata : midiMessages)
    {
        processedMidi.addEvent(metadata.getMessage(), metadata.samplePosition);
        auto message = metadata.getMessage();
        
        if (message.isNoteOn())
        {
            isNotePlaying.store(true);
            currentNoteNumber.store(message.getNoteNumber());
            
            // Establecer posición inicial según si el loop está habilitado
            if (loopEnabled.load()) {
                currentSamplePosition.store(loopStartPosition.load());
            } else {
                currentSamplePosition.store(0);
            }
        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentNoteNumber.load())
        {
            // Solo detener si el loop no está habilitado
            if (!loopEnabled.load()) {
                isNotePlaying.store(false);
                currentNoteNumber.store(-1);
                currentSamplePosition.store(0);
            }
        }
    }

    // Limpiar buffers
    buffer.clear();
    tempBuffer.clear();
 
    // Actualizar ADSR si es necesario
    if (sUpdate) {
        updateADSR();
    }

    // ========================================================================
    // LÓGICA DEL LOOP - GESTIÓN DE POSICIÓN Y REINICIO
    // ========================================================================
    
    if (isNotePlaying.load())
    {
        int64_t newPosition = currentSamplePosition.load() + buffer.getNumSamples();
        
        if (loopEnabled.load())
        {
            int64_t loopEnd = loopEndPosition.load();
            int64_t loopStart = loopStartPosition.load();
            
            // Si llegamos al final del loop, reiniciar
            if (newPosition >= loopEnd)
            {
                currentSamplePosition.store(loopStart);
                
                // Enviar note-off y note-on para reiniciar el sample
                int currentNote = currentNoteNumber.load();
                processedMidi.addEvent(juce::MidiMessage::noteOff(1, currentNote, (uint8_t)64), 0);
                processedMidi.addEvent(juce::MidiMessage::noteOn(1, currentNote, (uint8_t)127), 1);
            }
            else
            {
                currentSamplePosition.store(newPosition);
            }
        }
        else
        {
            currentSamplePosition.store(newPosition);
        }
    }

    // ========================================================================
    // PROCESAMIENTO DE AUDIO - MEZCLA DE SAMPLERS
    // ========================================================================
    
    // Obtener parámetro de mezcla (0-100%)
    float mixAmount = *apvts.getRawParameterValue("MixAmount") / 100.0f;
    
    // Crear buffers separados para cada sampler
    juce::AudioBuffer<float> clean1Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> excited1Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> clean2Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> excited2Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    
    clean1Buffer.clear();
    excited1Buffer.clear();
    clean2Buffer.clear();
    excited2Buffer.clear();
    
    // Renderizar cada sampler por separado
    mSampler1Clean.renderNextBlock(clean1Buffer, processedMidi, 0, buffer.getNumSamples());
    mSampler1Excited.renderNextBlock(excited1Buffer, processedMidi, 0, buffer.getNumSamples());
    mSampler2Clean.renderNextBlock(clean2Buffer, processedMidi, 0, buffer.getNumSamples());
    mSampler2Excited.renderNextBlock(excited2Buffer, processedMidi, 0, buffer.getNumSamples());
    
    // Aplicar crossfading entre versiones clean y excited
    clean1Buffer.applyGain(1.0f - mixAmount);
    excited1Buffer.applyGain(mixAmount);
    clean2Buffer.applyGain(1.0f - mixAmount);
    excited2Buffer.applyGain(mixAmount);

    // Mezclar todos los buffers en el buffer principal
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.addFrom(channel, 0, clean1Buffer, channel, 0, buffer.getNumSamples());
        buffer.addFrom(channel, 0, excited1Buffer, channel, 0, buffer.getNumSamples());
        buffer.addFrom(channel, 0, clean2Buffer, channel, 0, buffer.getNumSamples());
        buffer.addFrom(channel, 0, excited2Buffer, channel, 0, buffer.getNumSamples());
    }

    // Aplicar limitador final
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    limiter.process(context);
}

// ============================================================================
// GESTIÓN DE EDITOR
// ============================================================================

bool ProtectedSoundsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ProtectedSoundsAudioProcessor::createEditor()
{
    return new ProtectedSoundsAudioProcessorEditor(*this);
}

// ============================================================================
// PERSISTENCIA DE ESTADO
// ============================================================================

void ProtectedSoundsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ProtectedSoundsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

// ============================================================================
// CARGA DE SAMPLES
// ============================================================================

void ProtectedSoundsAudioProcessor::loadSoundPairForSelector1(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            // Almacenar información del audio
            audioLength.store(cleanReader->lengthInSamples / cleanReader->sampleRate);
            
            // Crear waveform para visualización
            waveForm.setSize(1, (int)cleanReader->lengthInSamples);
            cleanReader->read(&waveForm, 0, (int)cleanReader->lengthInSamples, 0, true, false);
            fileName = soundName;

            // Configurar rango MIDI (todas las notas)
            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            // Cargar samples en los samplers
            mSampler1Clean.clearSounds();
            mSampler1Excited.clearSounds();
            
            mSampler1Clean.addSound(new juce::SamplerSound(soundName + "_clean",
                                                     *cleanReader, range, 60, 0.1, 0.1, 10.0));
            mSampler1Excited.addSound(new juce::SamplerSound(soundName + "_excited",
                                                     *excitedReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
            
            // Configurar puntos de loop por defecto (todo el audio)
            loopStartPosition.store(0);
            loopEndPosition.store(static_cast<int64_t>(audioLength.load() * getSampleRate()));
            
            // Actualizar editor si existe
            updateEditorLoopSliders();
        }
    }
}

void ProtectedSoundsAudioProcessor::loadSoundPairForSelector2(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            mSampler2Clean.clearSounds();
            mSampler2Excited.clearSounds();
            
            mSampler2Clean.addSound(new juce::SamplerSound(soundName + "_clean",
                                                     *cleanReader, range, 60, 0.1, 0.1, 10.0));
            mSampler2Excited.addSound(new juce::SamplerSound(soundName + "_excited",
                                                     *excitedReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
        }
    }
}

// ============================================================================
// CONFIGURACIÓN DE LOOP
// ============================================================================

void ProtectedSoundsAudioProcessor::setLoopPoints(int64_t startSamples, int64_t endSamples)
{
    // Asegurar orden correcto
    if (startSamples > endSamples) {
        std::swap(startSamples, endSamples);
    }
    
    // Limitar a rango válido
    int64_t maxSamples = static_cast<int64_t>(audioLength.load() * getSampleRate());
    startSamples = juce::jlimit<int64_t>(0, maxSamples - 1, startSamples);
    endSamples = juce::jlimit<int64_t>(startSamples + 1, maxSamples, endSamples);
    
    loopStartPosition.store(startSamples);
    loopEndPosition.store(endSamples);
}

// ============================================================================
// ACTUALIZACIÓN DE PARÁMETROS
// ============================================================================

void ProtectedSoundsAudioProcessor::updateADSR()
{
    // Actualizar parámetros ADSR para ambos grupos
    mADSRParams.attack = apvts.getRawParameterValue("Attack")->load();
    mADSRParams2.attack = apvts.getRawParameterValue("Attack2")->load();
    mADSRParams.decay = apvts.getRawParameterValue("Decay")->load();
    mADSRParams2.decay = apvts.getRawParameterValue("Decay2")->load();
    mADSRParams.sustain = apvts.getRawParameterValue("Sustain")->load();
    mADSRParams2.sustain = apvts.getRawParameterValue("Sustain2")->load();
    mADSRParams.release = apvts.getRawParameterValue("Release")->load();
    mADSRParams2.release = apvts.getRawParameterValue("Release2")->load();

    // Aplicar a todos los sounds del primer grupo
    for (int i = 0; i < mSampler1Clean.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler1Clean.getSound(i).get()))
            sound->setEnvelopeParameters(mADSRParams);
    }
    for (int i = 0; i < mSampler1Excited.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler1Excited.getSound(i).get()))
            sound->setEnvelopeParameters(mADSRParams);
    }
    
    // Aplicar a todos los sounds del segundo grupo
    for (int i = 0; i < mSampler2Clean.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler2Clean.getSound(i).get()))
            sound->setEnvelopeParameters(mADSRParams2);
    }
    for (int i = 0; i < mSampler2Excited.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler2Excited.getSound(i).get()))
            sound->setEnvelopeParameters(mADSRParams2);
    }
}

void ProtectedSoundsAudioProcessor::setFilterFrequency(float frequency)
{
    filterFrequency = frequency;
    filter.setCutoffFrequency(filterFrequency);
}

void ProtectedSoundsAudioProcessor::setFilterResonance(float resonance)
{
    filterResonance = resonance;
    filter.setResonance(filterResonance);
}

// ============================================================================
// UTILIDADES
// ============================================================================

juce::StringArray ProtectedSoundsAudioProcessor::getAvailableSounds() const
{
    return soundsManager.getAvailableSounds();
}

void ProtectedSoundsAudioProcessor::updateEditorLoopSliders()
{
    if (auto* editor = dynamic_cast<ProtectedSoundsAudioProcessorEditor*>(getActiveEditor()))
    {
        editor->loopStartSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
        editor->loopEndSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
        editor->loopEndSlider.setValue(audioLength.load() * 1000.0, juce::dontSendNotification);
        editor->repaint();
    }
}

// ============================================================================
// CREACIÓN DE PARÁMETROS
// ============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout ProtectedSoundsAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    // Parámetros ADSR para primer grupo
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Attack", 1), "Attack", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Decay", 1), "Decay", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Sustain", 1), "Sustain", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Release", 1), "Release", 0.0f, 5.0f, 0.0f));
    
    // Parámetros ADSR para segundo grupo
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Attack2", 1), "Attack2", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Decay2", 1), "Decay2", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Sustain2", 1), "Sustain2", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Release2", 1), "Release2", 0.0f, 5.0f, 0.0f));
    
    // Parámetros de filtro
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FilterFreq", 1),
        "Filter Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f),
        1000.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("FilterRes", 1),
        "Filter Resonance",
        juce::NormalisableRange<float>(0.1f, 1.0f, 0.01f),
        0.7f));
    
    // Parámetro de mezcla
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MixAmount", 1),
        "Mix",
        0.0f,   // 0% = solo clean
        100.0f, // 100% = solo excited
        50.0f   // 50% = mezcla equilibrada
    ));

    return { parameters.begin(), parameters.end() };
}

void ProtectedSoundsAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                                          const juce::Identifier& property)
{
    sUpdate = true;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProtectedSoundsAudioProcessor();
}
