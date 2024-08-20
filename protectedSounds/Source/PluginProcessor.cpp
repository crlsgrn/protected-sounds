/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ProtectedSoundsManager.h"

//==============================================================================
ProtectedSoundsAudioProcessor::ProtectedSoundsAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ), apvts(*this, nullptr, "Parameters", createParameters())
#endif
{

    mFormatManager.registerBasicFormats();
    mFormatManager2.registerBasicFormats();
    
    limiter.setThreshold(0.0f);  // 0 dB
    limiter.setRelease(100.0f);  // Release time in milliseconds

    //we need to register the value tree listener and associate to ur audio processor value tree state
    apvts.state.addListener(this);
    
    for(int i = 0; i<mNumVoices; i++){
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

//==============================================================================
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
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int ProtectedSoundsAudioProcessor::getCurrentProgram()
{
    return 0;
}

void ProtectedSoundsAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String ProtectedSoundsAudioProcessor::getProgramName (int index)
{
    return {};
}

void ProtectedSoundsAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void ProtectedSoundsAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    mSampler1.setCurrentPlaybackSampleRate(sampleRate);
    mSampler2.setCurrentPlaybackSampleRate(sampleRate);
    tempBuffer.setSize(getTotalNumOutputChannels(), samplesPerBlock);
    updateADSR();
    
    // Prepare the limiter
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = samplesPerBlock;
    spec.numChannels = getTotalNumOutputChannels();
    limiter.prepare(spec);
    
}

void ProtectedSoundsAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool ProtectedSoundsAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void ProtectedSoundsAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i){
        buffer.clear (i, 0, buffer.getNumSamples());
        tempBuffer.clear (i, 0, tempBuffer.getNumSamples());
    }
    
    
    if(sUpdate){
        updateADSR();
    }
    
    mSampler1.renderNextBlock(tempBuffer, midiMessages, 0, buffer.getNumSamples());
    
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }
    
    tempBuffer.clear();
    
    mSampler2.renderNextBlock(tempBuffer, midiMessages, 0, buffer.getNumSamples());

    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        buffer.addFrom(channel, 0, tempBuffer, channel, 0, buffer.getNumSamples());
    }
    
    juce::dsp::AudioBlock<float> audioBlock(buffer);
    juce::dsp::ProcessContextReplacing<float> context(audioBlock);
    limiter.process(context);
}

//==============================================================================
bool ProtectedSoundsAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* ProtectedSoundsAudioProcessor::createEditor()
{
    return new ProtectedSoundsAudioProcessorEditor (*this);
    
}

//==============================================================================
void ProtectedSoundsAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void ProtectedSoundsAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

void ProtectedSoundsAudioProcessor::loadFile1()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Choose an audio file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.mp3", // Filtra archivos por formato WAV o MP3
        true); // Permitir selección de archivos

    constexpr auto fileChooserFlags = juce::FileBrowserComponent::openMode |
                                      juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File chosenFile(chooser.getResult());
        DBG("Selected file for sampler1: " + chosenFile.getFullPathName());

        // Intenta crear un AudioFormatReader para el archivo seleccionado
        std::unique_ptr<juce::AudioFormatReader> audioReader(mFormatManager.createReaderFor(chosenFile));

        if (audioReader != nullptr && audioReader->sampleRate > 0)
        {
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler1.clearSounds();

            // Crea un SamplerSound con el AudioFormatReader válido
            mSampler1.addSound(new juce::SamplerSound("Sample", *audioReader, range, 60, 0.1, 0.1, 10.0));
            //mSampler.addSound(new juce::SamplerSound(const String &name, AudioFormatReader &source, const BigInteger &midiNotes, int midiNoteForNormalPitch, double attackTimeSecs, double releaseTimeSecs, double maxSampleLengthSeconds))
            updateADSR();

        }
        else
        {
            DBG("Error: Failed to create AudioFormatReader or invalid sample rate.");
        }
    });
    
}

void ProtectedSoundsAudioProcessor::loadFile2()
{
    fileChooser2 = std::make_unique<juce::FileChooser>(
        "Choose an audio file",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.mp3", // Filtra archivos por formato WAV o MP3
        true); // Permitir selección de archivos

    constexpr auto fileChooserFlags = juce::FileBrowserComponent::openMode |
                                      juce::FileBrowserComponent::canSelectFiles;

    fileChooser2->launchAsync(fileChooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File chosenFile(chooser.getResult());
        DBG("Selected file for sampler2: " + chosenFile.getFullPathName());

        // Intenta crear un AudioFormatReader para el archivo seleccionado
        std::unique_ptr<juce::AudioFormatReader> audioReader(mFormatManager2.createReaderFor(chosenFile));

        if (audioReader != nullptr && audioReader->sampleRate > 0)
        {
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler2.clearSounds();

            // Crea un SamplerSound con el AudioFormatReader válido
            mSampler2.addSound(new juce::SamplerSound("Sample", *audioReader, range, 60, 0.1, 0.1, 10.0));
            //mSampler.addSound(new juce::SamplerSound(const String &name, AudioFormatReader &source, const BigInteger &midiNotes, int midiNoteForNormalPitch, double attackTimeSecs, double releaseTimeSecs, double maxSampleLengthSeconds))
            updateADSR();

        }
        else
        {
            DBG("Error: Failed to create AudioFormatReader or invalid sample rate.");
        }
    });
    
}

void ProtectedSoundsAudioProcessor::loadProtectedSound1(const juce::String& soundName)
{
    auto inputStream = soundsManager.loadSound(soundName);
    
    
    if (inputStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(mFormatManager.createReaderFor(std::move(inputStream)));

        if (reader != nullptr)
        {
            juce::BigInteger range;
            range.setRange(0, 128, true);
            mSampler1.clearSounds();
            mSampler1.addSound(new juce::SamplerSound(soundName, *reader, range, 60, 0.1, 0.1, 10.0));
            updateADSR();
        }
    }

    
}

void ProtectedSoundsAudioProcessor::loadProtectedSound2(const juce::String& soundName)
{
    auto inputStream = soundsManager.loadSound(soundName);
    if (inputStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatReader> reader(mFormatManager2.createReaderFor(std::move(inputStream)));
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


juce::StringArray ProtectedSoundsAudioProcessor::getAvailableSounds() const
{
    return soundsManager.getAvailableSounds();
}


void ProtectedSoundsAudioProcessor::updateADSR(){
    
    mADSRParams.attack = apvts.getRawParameterValue("Attack")->load();
    mADSRParams2.attack = apvts.getRawParameterValue("Attack2")->load();
    mADSRParams.decay = apvts.getRawParameterValue("Decay")->load();
    mADSRParams2.decay = apvts.getRawParameterValue("Decay2")->load();
    mADSRParams.sustain = apvts.getRawParameterValue("Sustain")->load();
    mADSRParams2.sustain = apvts.getRawParameterValue("Sustain2")->load();
    mADSRParams.release = apvts.getRawParameterValue("Release")->load();
    mADSRParams2.release = apvts.getRawParameterValue("Release2")->load();

    
    for (int i = 0; i < mSampler1.getNumSounds(); ++i){
        
        if(auto sound = dynamic_cast<juce::SamplerSound*>(mSampler1.getSound(i).get())){
            sound->setEnvelopeParameters(mADSRParams);
        }
    }
    
    for (int i = 0; i < mSampler2.getNumSounds(); ++i){
        
        if(auto sound = dynamic_cast<juce::SamplerSound*>(mSampler2.getSound(i).get())){
            sound->setEnvelopeParameters(mADSRParams2);
        }
    }
    
}

void limit(juce::AudioBuffer<float>& buffer, float threshold){
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
    {
        float* samples = buffer.getWritePointer(channel);
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
        {
            if (samples[sample] > threshold)
                samples[sample] = threshold;
        }
    }
}



juce::AudioProcessorValueTreeState::ParameterLayout ProtectedSoundsAudioProcessor::createParameters(){
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;
    
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Attack",1) , "Attack", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Attack2",1) , "Attack2", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Decay",1) , "Decay", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Decay2",1) , "Decay2", 0.0f, 5.0f, 2.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Sustain",1) , "Sustain", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Sustain2",1) , "Sustain2", 0.0f, 1.0f, 1.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Release",1) , "Release", 0.0f, 5.0f, 0.0f));
    parameters.push_back(std::make_unique<juce::AudioParameterFloat> (juce::ParameterID("Release2",1) , "Release2", 0.0f, 5.0f, 0.0f));
    

    return { parameters.begin(), parameters.end() };
}

void ProtectedSoundsAudioProcessor::valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property){
    
    sUpdate = true;
    
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProtectedSoundsAudioProcessor();
}

