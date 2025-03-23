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
    return 1;
}

int ProtectedSoundsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ProtectedSoundsAudioProcessor::setCurrentProgram(int index)
{
}

const juce::String ProtectedSoundsAudioProcessor::getProgramName(int index)
{
    return {};
}

void ProtectedSoundsAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
}

void ProtectedSoundsAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSampler1.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2.setCurrentPlaybackSampleRate(sampleRate);
    mSampler1Clean.setCurrentPlaybackSampleRate(sampleRate);
    mSampler1Excited.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2Clean.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2Excited.setCurrentPlaybackSampleRate(sampleRate);
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    updateADSR();
    
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    
    filter.prepare(spec);
    filter.setType(juce::dsp::StateVariableTPTFilterType::bandpass); // Puedes cambiar a lowpass, highpass, etc.
    filter.setCutoffFrequency(filterFrequency);
    filter.setResonance(filterResonance);
    
    limiter.prepare(spec);
}

void ProtectedSoundsAudioProcessor::releaseResources()
{
}

bool ProtectedSoundsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

/*
void ProtectedSoundsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    DBG(loopStartPosition);
    DBG(loopEndPosition);
    DBG(currentPosition);
    
    
    // Track sample position
    //static double currentSamplePosition = 0.0;
    double sampleDuration = static_cast<double>(buffer.getNumSamples()) / getSampleRate();
    
    // Procesar mensajes MIDI y loop
    juce::MidiBuffer processedMidi; // Nuevo buffer MIDI que contendrá todos los mensajes
    
    // Copiar los mensajes MIDI originales
    for (const auto metadata : midiMessages)
    {
        processedMidi.addEvent(metadata.getMessage(), metadata.samplePosition);
        
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            
            isNotePlaying.store(true);
            currentNoteNumber.store(message.getNoteNumber());
            //currentSamplePosition = 0.0;  // Reset position on new note
            currentSamplePosition = loopEnabled ? loopStartPosition.load() : 0.0;

        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentNoteNumber.load())
        {
            isNotePlaying.store(false);
            currentNoteNumber.store(-1);
            currentSamplePosition = 0.0;
        }
    }
 
    // Limpiar buffers
    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
        tempBuffer.clear(i, 0, buffer.getNumSamples());
    }
 
    if (sUpdate) {
        updateADSR();
    }
    

    // Procesar loop y añadir mensajes de loop si es necesario
    if (isNotePlaying.load())
    {
        currentSamplePosition += sampleDuration;
        
        if (loopEnabled.load() && currentSamplePosition >= loopEndPosition.load())
        {
            currentSamplePosition = loopStartPosition.load();
            
            int currentNote = currentNoteNumber.load();
            processedMidi.addEvent(juce::MidiMessage::noteOff(1, currentNote, (uint8_t)64), 0);
            processedMidi.addEvent(juce::MidiMessage::noteOn(1, currentNote, (uint8_t)127), 1);
        }
    }

    // Obtener el valor de mezcla
    float mixAmount = *apvts.getRawParameterValue("MixAmount") / 100.0f;

    // Procesar señal limpia (Sampler1)
    mSampler1.renderNextBlock(tempBuffer, processedMidi, 0, buffer.getNumSamples());
    tempBuffer.applyGain(1.0f - mixAmount);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }

    tempBuffer.clear();

    // Procesar señal excitada (Sampler2)
    mSampler2.renderNextBlock(tempBuffer, processedMidi, 0, buffer.getNumSamples());
    tempBuffer.applyGain(mixAmount);
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }

    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    limiter.process(context);
}
*/

void ProtectedSoundsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    DBG("---------------");
    DBG("Loop Start (samples): " << loopStartPosition.load());
    DBG("Loop End (samples): " << loopEndPosition.load());
    DBG("Current Position (samples): " << currentSamplePosition.load());
    
    juce::MidiBuffer processedMidi;
    
    // Copiar los mensajes MIDI originales
    for (const auto metadata : midiMessages)
    {
        processedMidi.addEvent(metadata.getMessage(), metadata.samplePosition);
        
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            isNotePlaying.store(true);
            currentNoteNumber.store(message.getNoteNumber());
            
            if (loopEnabled.load()) {
                // Establecer la posición directamente a loopStartPosition
                int64_t startPos = loopStartPosition.load();
                DBG("Setting position to loop start: " << startPos);
                currentSamplePosition.store(startPos);
            } else {
                DBG("Setting position to 0");
                currentSamplePosition.store(0);
            }
            
            DBG("Current position after note on: " << currentSamplePosition.load());
        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentNoteNumber.load())
        {
            if (!loopEnabled.load()) {
                isNotePlaying.store(false);
                currentNoteNumber.store(-1);
                currentSamplePosition.store(0);
                DBG("Note Off - Resetting position");
            }
        }
    }

    // Limpiar buffers
    buffer.clear();
    tempBuffer.clear();
 
    if (sUpdate) {
        updateADSR();
    }

    if (isNotePlaying.load())
    {
        DBG("Processing loop...");
        DBG("Current position before: " << currentSamplePosition.load());
        
        int64_t newPosition = currentSamplePosition.load() + buffer.getNumSamples();
        DBG("New position would be: " << newPosition);
        
        if (loopEnabled.load())
        {
            int64_t loopEnd = loopEndPosition.load();
            int64_t loopStart = loopStartPosition.load();
            
            if (newPosition >= loopEnd)
            {
                DBG("Loop point reached!");
                currentSamplePosition.store(loopStart);
                
                int currentNote = currentNoteNumber.load();
                processedMidi.addEvent(juce::MidiMessage::noteOff(1, currentNote, (uint8_t)64), 0);
                processedMidi.addEvent(juce::MidiMessage::noteOn(1, currentNote, (uint8_t)127), 1);
            }
            else
            {
                DBG("Updating position to: " << newPosition);
                currentSamplePosition.store(newPosition);
            }
        }
        else
        {
            currentSamplePosition.store(newPosition);
        }
        
        DBG("Final position: " << currentSamplePosition.load());
    }

    // Obtener el valor de mezcla
    float mixAmount = *apvts.getRawParameterValue("MixAmount") / 100.0f;

    /*
    // Buffer para señal limpia
    juce::AudioBuffer<float> cleanBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    cleanBuffer.clear();
    
    // Buffer para señal saturada
    juce::AudioBuffer<float> satBuffer(buffer.getNumChannels(), buffer.getNumSamples());
    satBuffer.clear();*/
    
    juce::AudioBuffer<float> clean1Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> excited1Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> clean2Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    juce::AudioBuffer<float> excited2Buffer(buffer.getNumChannels(), buffer.getNumSamples());
    
    clean1Buffer.clear();
    excited1Buffer.clear();
    clean2Buffer.clear();
    excited2Buffer.clear();
    
    // Procesar primer par de sonidos
    mSampler1Clean.renderNextBlock(clean1Buffer, processedMidi, 0, buffer.getNumSamples());
    mSampler1Excited.renderNextBlock(excited1Buffer, processedMidi, 0, buffer.getNumSamples());
        
        // Aplicar ganancia según la mezcla para el primer par
    clean1Buffer.applyGain(1.0f - mixAmount);
    excited1Buffer.applyGain(mixAmount);
        
        // Procesar segundo par de sonidos
    mSampler2Clean.renderNextBlock(clean2Buffer, processedMidi, 0, buffer.getNumSamples());
    mSampler2Excited.renderNextBlock(excited2Buffer, processedMidi, 0, buffer.getNumSamples());
        
        // Aplicar ganancia según la mezcla para el segundo par
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

    
    /*
    // Procesar señal limpia
    mSampler1.renderNextBlock(cleanBuffer, processedMidi, 0, buffer.getNumSamples());
    cleanBuffer.applyGain(1.0f - mixAmount);

    // Procesar señal saturada
    mSampler2.renderNextBlock(satBuffer, processedMidi, 0, buffer.getNumSamples());
    satBuffer.applyGain(mixAmount);

    // Mezclar en el buffer principal
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.addFrom(channel, 0, cleanBuffer, channel, 0, buffer.getNumSamples());
        buffer.addFrom(channel, 0, satBuffer, channel, 0, buffer.getNumSamples());
    }
*/
    // Limiter
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    limiter.process(context);
}

bool ProtectedSoundsAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* ProtectedSoundsAudioProcessor::createEditor()
{
    return new ProtectedSoundsAudioProcessorEditor(*this);
}

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
            
            // Inicializar puntos de loop
            loopStartPosition.store(0.0);
            loopEndPosition.store(audioLength.load() * 1000.0); // Convertir a milisegundos
            
            // Si el editor existe, actualizar los rangos de los sliders
            if (auto* editor = dynamic_cast<ProtectedSoundsAudioProcessorEditor*>(getActiveEditor()))
            {
                editor->loopStartSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setValue(audioLength.load() * 1000.0, juce::dontSendNotification);
            }
        }
    }
}
/*
void ProtectedSoundsAudioProcessor::setLoopPoints(double startMs, double endMs)
{
    // Convertir de milisegundos a segundos
    double startSec = startMs / 1000.0;
    double endSec = endMs / 1000.0;
    
    // Asegurar que los puntos de loop estén dentro de los límites del audio
    loopStartPosition.store(juce::jlimit(0.0, audioLength.load(), startSec));
    loopEndPosition.store(juce::jlimit(loopStartPosition.load(), audioLength.load(), endSec));
    
    if(loopEnabled.load())
        currentPosition = loopStartPosition.load() * getSampleRate();
}*/

void ProtectedSoundsAudioProcessor::setLoopPoints(int64_t startSamples, int64_t endSamples)
{
    loopStartPosition.store(startSamples);
    loopEndPosition.store(endSamples);
    DBG("Loop points set - Start: " << startSamples << " End: " << endSamples);
}

/*
void ProtectedSoundsAudioProcessor::loadProtectedSoundPair(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            audioLength.store(cleanReader->lengthInSamples / cleanReader->sampleRate);
            
            waveForm.setSize(1, (int)cleanReader->lengthInSamples);
            cleanReader->read(&waveForm, 0, (int)cleanReader->lengthInSamples, 0, true, false);
            fileName = soundName;

            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            mSampler1.clearSounds();
            mSampler2.clearSounds();
            
            mSampler1.addSound(new juce::SamplerSound(soundName + "_clean",
                                                     *cleanReader, range, 60, 0.1, 0.1, 10.0));
            mSampler2.addSound(new juce::SamplerSound(soundName + "_excited",
                                                     *excitedReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
            
            // Establecer puntos de loop por defecto
            // Inicializar puntos de loop
            loopStartPosition.store(0.0);
            loopEndPosition.store(audioLength.load() * 1000.0); // Convertir a milisegundos
            
            // Si el editor existe, actualizar los rangos de los sliders
            if (auto* editor = dynamic_cast<ProtectedSoundsAudioProcessorEditor*>(getActiveEditor()))
            {
                editor->loopStartSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setValue(audioLength.load() * 1000.0, juce::dontSendNotification);
                editor->repaint();
            }
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
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler2.clearSounds();
            mSampler2.addSound(new juce::SamplerSound("Sample", *reader, range, 60, 0.1, 0.1, 10.0));
            updateADSR();
        }
    }
}


void ProtectedSoundsAudioProcessor::loadProtectedSoundPairForSampler1(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            // Para el primer sampler, solo actualizamos mSampler1
            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            mSampler1.clearSounds();
            mSampler1.addSound(new juce::SamplerSound(soundName + "_clean",
                                                     *cleanReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
        }
    }
}

void ProtectedSoundsAudioProcessor::loadProtectedSoundPairForSampler2(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            // Para el segundo sampler, solo actualizamos mSampler2
            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            mSampler2.clearSounds();
            mSampler2.addSound(new juce::SamplerSound(soundName + "_excited",
                                                     *excitedReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
        }
    }
}
*/

void ProtectedSoundsAudioProcessor::loadSoundPairForSelector1(const juce::String& soundName)
{
    auto [cleanStream, excitedStream] = soundsManager.loadSoundPair(soundName);
    
    if (cleanStream != nullptr && excitedStream != nullptr)
    {
        auto cleanReader = mFormatManager.createReaderFor(std::move(cleanStream));
        auto excitedReader = mFormatManager.createReaderFor(std::move(excitedStream));

        if (cleanReader != nullptr && excitedReader != nullptr)
        {
            audioLength.store(cleanReader->lengthInSamples / cleanReader->sampleRate);
            
            waveForm.setSize(1, (int)cleanReader->lengthInSamples);
            cleanReader->read(&waveForm, 0, (int)cleanReader->lengthInSamples, 0, true, false);
            fileName = soundName;

            juce::BigInteger range;
            range.setRange(0, 128, true);
            
            mSampler1Clean.clearSounds();
            mSampler1Excited.clearSounds();
            
            mSampler1Clean.addSound(new juce::SamplerSound(soundName + "_clean",
                                                     *cleanReader, range, 60, 0.1, 0.1, 10.0));
            mSampler1Excited.addSound(new juce::SamplerSound(soundName + "_excited",
                                                     *excitedReader, range, 60, 0.1, 0.1, 10.0));
            
            updateADSR();
            
            // Establecer puntos de loop por defecto
            loopStartPosition.store(0);
            loopEndPosition.store(static_cast<int64_t>(audioLength.load() * getSampleRate()));
            
            // Si el editor existe, actualizar los rangos de los sliders
            if (auto* editor = dynamic_cast<ProtectedSoundsAudioProcessorEditor*>(getActiveEditor()))
            {
                editor->loopStartSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setRange(0.0, audioLength.load() * 1000.0, 1.0);
                editor->loopEndSlider.setValue(audioLength.load() * 1000.0, juce::dontSendNotification);
                editor->repaint();
            }
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

    for (int i = 0; i < mSampler1.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler1.getSound(i).get()))
        {
            sound->setEnvelopeParameters(mADSRParams);
        }
    }
    
    for (int i = 0; i < mSampler2.getNumSounds(); ++i)
    {
        if (auto sound = dynamic_cast<juce::SamplerSound*>(mSampler2.getSound(i).get()))
        {
            sound->setEnvelopeParameters(mADSRParams2);
        }
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

juce::StringArray ProtectedSoundsAudioProcessor::getAvailableSounds() const
{
    return soundsManager.getAvailableSounds();
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
    parameters.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("MixAmount", 1),
        "Mix",
        0.0f,   // min
        100.0f, // max
        50.0f   // default
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
