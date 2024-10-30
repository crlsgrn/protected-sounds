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
}

bool ProtectedSoundsAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void ProtectedSoundsAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    
    // Track sample position
    static double currentSamplePosition = 0.0;
    double sampleDuration = static_cast<double>(buffer.getNumSamples()) / getSampleRate();
    
    for (const auto metadata : midiMessages)
    {
        auto message = metadata.getMessage();
        if (message.isNoteOn())
        {
            isNotePlaying.store(true);
            currentNoteNumber.store(message.getNoteNumber());
            currentSamplePosition = 0.0;  // Reset position on new note
        }
        else if (message.isNoteOff() && message.getNoteNumber() == currentNoteNumber.load())
        {
            isNotePlaying.store(false);
            currentNoteNumber.store(-1);
            currentSamplePosition = 0.0;
        }
    }

    for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    {
        buffer.clear(i, 0, buffer.getNumSamples());
        tempBuffer.clear(i, 0, buffer.getNumSamples());
    }

    if (sUpdate) {
        updateADSR();
    }

    // Update position and check for loop
    if (isNotePlaying.load())
    {
        currentSamplePosition += sampleDuration;
        
        if (loopEnabled.load() && currentSamplePosition >= loopEndPosition.load())
        {
            currentSamplePosition = loopStartPosition.load();
            
            int currentNote = currentNoteNumber.load();
            juce::MidiMessage noteOff = juce::MidiMessage::noteOff(1, currentNote, (uint8_t)64);
            juce::MidiMessage noteOn = juce::MidiMessage::noteOn(1, currentNote, (uint8_t)127);
            
            midiMessages.addEvent(noteOff, 0);
            midiMessages.addEvent(noteOn, 1);
        }
    }

    mSampler1.renderNextBlock(tempBuffer, midiMessages, 0, buffer.getNumSamples());
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }

    tempBuffer.clear();

    mSampler2.renderNextBlock(tempBuffer, midiMessages, 0, buffer.getNumSamples());
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }

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

void ProtectedSoundsAudioProcessor::setLoopPoints(double startMs, double endMs)
{
    // Convertir de milisegundos a segundos
    double startSec = startMs / 1000.0;
    double endSec = endMs / 1000.0;
    
    // Asegurar que los puntos de loop estén dentro de los límites del audio
    loopStartPosition.store(juce::jlimit(0.0, audioLength.load(), startSec));
    loopEndPosition.store(juce::jlimit(loopStartPosition.load(), audioLength.load(), endSec));
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
