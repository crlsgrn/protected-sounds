

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
