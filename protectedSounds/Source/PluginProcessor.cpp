

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
    mFormatManager.registerBasicFormats();
    mFormatManager2.registerBasicFormats();
    
    limiter.setThreshold(0.0f);
    limiter.setRelease(100.0f);

    apvts.state.addListener(this);
    
    for(int i = 0; i < mNumVoices; i++) {
        mSampler1.addVoice(new juce::SamplerVoice());
        mSampler2.addVoice(new juce::SamplerVoice());
    }

    readAheadThread.startThread(juce::Thread::Priority::normal);
}

ProtectedSoundsAudioProcessor::~ProtectedSoundsAudioProcessor()
{
    apvts.state.removeListener(this);
    mFormatReader = nullptr;
    mFormatReader2 = nullptr;
    transportSource.setSource(nullptr);
    readAheadThread.stopThread(500);
    readerSource.reset();
}

void ProtectedSoundsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    transportSource.prepareToPlay(samplesPerBlock, sampleRate);
    mSampler1.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2.setCurrentPlaybackSampleRate(sampleRate);
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    updateADSR();
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    limiter.prepare(spec);
}

void ProtectedSoundsAudioProcessor::releaseResources()
{
    transportSource.releaseResources();
}

// En PluginProcessor.cpp
void ProtectedSoundsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                               juce::MidiBuffer& midiMessages)
{
    
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            currentNoteNumber.store(message.getNoteNumber());
        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentNoteNumber.load())
        {
            currentNoteNumber.store(-1);
        }
    }
    
    
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // Limpiar buffers
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) {
        buffer.clear(i, 0, buffer.getNumSamples());
        tempBuffer.clear(i, 0, buffer.getNumSamples());
    }

    // Procesar transport source y actualizar loop
    if (transportSource.isPlaying())
    {
        juce::AudioSourceChannelInfo info(buffer);
        transportSource.getNextAudioBlock(info);

        if (loopEnabled.load())
        {
            double currentPos = transportSource.getCurrentPosition();
            if (currentPos >= loopEndPosition.load())
            {
                transportSource.setPosition(loopStartPosition.load());
                
                // Para el sampler, necesitamos detener y reiniciar las notas
                for (int i = 0; i < mSampler1.getNumVoices(); ++i)
                {
                    if (auto* voice = dynamic_cast<juce::SamplerVoice*>(mSampler1.getVoice(i)))
                    {
                        if (voice->isVoiceActive())
                        {
                            // Crear un nuevo mensaje MIDI note off y note on para reiniciar
                            int noteNumber = 60; // O el número de nota actual
                            if (currentNoteNumber.load() >= 0)
                            {
                                noteNumber = currentNoteNumber.load();
                            }
                            
                            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, noteNumber, (uint8_t)0);
                            juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, noteNumber, (uint8_t)127);
                            
                            // Agregar los mensajes al buffer MIDI
                            midiMessages.addEvent(noteOff, 0);
                            midiMessages.addEvent(noteOn, 1);
                        }
                    }
                }
            }
        }
    }

    // Renderizar sampler
    mSampler1.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

    // Aplicar limiter
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    limiter.process(context);
}
/*
void ProtectedSoundsAudioProcessor::loadProtectedSound1(const juce::String& soundName)
{
    auto inputStream = soundsManager.loadSound(soundName);
    
    if (inputStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            mFormatManager.createReaderFor(std::move(inputStream))
        );

        if (reader != nullptr)
        {
            audioLength.store(reader->lengthInSamples / reader->sampleRate);
            
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler1.clearSounds();
            
            mSampler1.addSound(new juce::SamplerSound(
                soundName,
                *reader,
                range,
                60,    // root note
                0.1,   // attack time
                0.1,   // release time
                10.0   // maximum sample length
            ));
            
            updateADSR();
            
            loopStartPosition.store(0.0);
            loopEndPosition.store(audioLength.load());
        }
    }
}


void ProtectedSoundsAudioProcessor::loadProtectedSound1(const juce::File& file)
{
    if (auto* reader = mFormatManager.createReaderFor(file))
    {
        auto newSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(newSource.get(), 32768, &readAheadThread, reader->sampleRate);
        
        loopStartPosition = 0.0;
        loopEndPosition = transportSource.getLengthInSeconds();
        
        juce::BigInteger range;
        range.setRange(0, 128, true);
        mSampler1.clearSounds();
        
        mSampler1.addSound(new juce::SamplerSound(
            "audioprueba",
            *reader,
            range,
            60,    // root note
            0.1,   // attack time
            0.1,   // release time
            10.0   // maximum sample length
        ));
        
        updateADSR();
    }
}
*/

void ProtectedSoundsAudioProcessor::loadProtectedSound1(const juce::String& soundName)
{
    auto inputStream = soundsManager.loadSound(soundName);
    
    if (inputStream != nullptr)
    {
        // Limpiar recursos existentes
        transportSource.setSource(nullptr);
        readerSource.reset();
        
        // Crear un único reader
        currentReader = mFormatManager.createReaderFor(std::move(inputStream));

        if (currentReader != nullptr)
        {
            audioLength.store(currentReader->lengthInSamples / currentReader->sampleRate);
            
            // Configurar transport source
            readerSource = std::make_unique<juce::AudioFormatReaderSource>(currentReader, true);
            transportSource.setSource(readerSource.get(), 32768, &readAheadThread,
                                   currentReader->sampleRate);

            // Configurar sampler con el mismo reader
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler1.clearSounds();
            
            mSampler1.addSound(new juce::SamplerSound(
                soundName,
                *currentReader,  // Usar el mismo reader
                range,
                60,    // root note
                0.1,   // attack time
                0.1,   // release time
                10.0   // maximum sample length
            ));
            
            updateADSR();
            
            loopStartPosition.store(0.0);
            loopEndPosition.store(audioLength.load());
        }
    }
}

void ProtectedSoundsAudioProcessor::loadProtectedSound2(const juce::String& soundName)
{
    auto inputStream = soundsManager.loadSound(soundName);
    
    if (inputStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            mFormatManager2.createReaderFor(std::move(inputStream))
        );

        if (reader != nullptr)
        {
            audioLength.store(reader->lengthInSamples / reader->sampleRate);
            
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler2.clearSounds();
            
            mSampler2.addSound(new juce::SamplerSound(
                soundName,
                *reader,
                range,
                60,    // root note
                0.1,   // attack time
                0.1,   // release time
                10.0   // maximum sample length
            ));
            
            updateADSR();
            
            loopStartPosition.store(0.0);
            loopEndPosition.store(audioLength.load());
        }
    }
}





juce::StringArray ProtectedSoundsAudioProcessor::getAvailableSounds() const
{
    return soundsManager.getAvailableSounds();
}


void ProtectedSoundsAudioProcessor::setLoopPoints(double startMs, double endMs)
{
    double startSec = startMs / 1000.0;
    double endSec = endMs / 1000.0;
    
    loopStartPosition.store(juce::jlimit(0.0, audioLength.load(), startSec));
    loopEndPosition.store(juce::jlimit(loopStartPosition.load(), audioLength.load(), endSec));
}

void ProtectedSoundsAudioProcessor::setLoopEnabled(bool shouldLoop)
{
    loopEnabled.store(shouldLoop);
}

juce::AudioProcessorValueTreeState::ParameterLayout ProtectedSoundsAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Attack", 1), "Attack", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Attack2", 1), "Attack2", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Decay", 1), "Decay", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Decay2", 1), "Decay2", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Sustain", 1), "Sustain", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Sustain2", 1), "Sustain2", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Release", 1), "Release", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID("Release2", 1), "Release2", 0.0f, 5.0f, 0.0f));

    return { parameters.begin(), parameters.end() };
}

void ProtectedSoundsAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                                           const juce::Identifier& property)
{
    sUpdate = true;
}

//==============================================================================
void ProtectedSoundsAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void ProtectedSoundsAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr) {
        if (xmlState->hasTagName(apvts.state.getType())) {
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
        }
    }
}


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

int ProtectedSoundsAudioProcessor::getNumPrograms()
{
    return 1;   // No usamos programas
}

int ProtectedSoundsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ProtectedSoundsAudioProcessor::setCurrentProgram(int index)
{
    // No hacemos nada ya que no usamos programas
}

const juce::String ProtectedSoundsAudioProcessor::getProgramName(int index)
{
    return {};
}

void ProtectedSoundsAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
    // No hacemos nada ya que no usamos programas
}

bool ProtectedSoundsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ProtectedSoundsAudioProcessor::createEditor()
{
    return new ProtectedSoundsAudioProcessorEditor(*this);
}

bool ProtectedSoundsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    #if JucePlugin_IsMidiEffect
        juce::ignoreUnused (layouts);
        return true;
    #else
        if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
            && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
            return false;

        #if ! JucePlugin_IsSynth
            if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
                return false;
        #endif

        return true;
    #endif
}

void ProtectedSoundsAudioProcessor::updateADSR()
{
    mADSRParams.attack = apvts.getRawParameterValue("Attack")->load();
    mADSRParams2.attack = apvts.getRawParameterValue("Attack2")->load();
    mADSRParams.decay = apvts.getRawParameterValue("Decay")->load();
    mADSRParams2.decay = apvts.getRawParameterValue("Decay2")->load();
    mADSRParams.sustain = apvts.getRawParameterValue("Sustain")->load();
    mADSRParams2.sustain = apvts.getRawParameterValue("Sustain2")->load();
    mADSRParams.release = apvts.getRawParameterValue("Release")->load();
    mADSRParams2.release = apvts.getRawParameterValue("Release2")->load();

    for (int i = 0; i < mSampler1.getNumSounds(); ++i) {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler1.getSound(i).get())) {
            sound->setEnvelopeParameters(mADSRParams);
        }
    }
    
    for (int i = 0; i < mSampler2.getNumSounds(); ++i) {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler2.getSound(i).get())) {
            sound->setEnvelopeParameters(mADSRParams2);
        }
    }
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProtectedSoundsAudioProcessor();
}
