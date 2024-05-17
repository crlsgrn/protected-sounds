/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
                       )
#endif
{
    mFormatManager.registerBasicFormats();
    for(int i = 0; i<mNumVoices; i++){
        mSampler.addVoice(new juce::SamplerVoice());
    }
}

ProtectedSoundsAudioProcessor::~ProtectedSoundsAudioProcessor()
{
    mFormatReader = nullptr;
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
    mSampler.setCurrentPlaybackSampleRate(sampleRate);
    
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
    

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
    
    mSampler.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());


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

void ProtectedSoundsAudioProcessor::loadFile()
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
        DBG("Selected file: " + chosenFile.getFullPathName());

        // Intenta crear un AudioFormatReader para el archivo seleccionado
        std::unique_ptr<juce::AudioFormatReader> audioReader(mFormatManager.createReaderFor(chosenFile));

        if (audioReader != nullptr && audioReader->sampleRate > 0)
        {
            juce::BigInteger range;
            range.setRange(0, 128, true);

            // Crea un SamplerSound con el AudioFormatReader válido
            mSampler.addSound(new juce::SamplerSound("Sample", *audioReader, range, 60, 0.1, 0.1, 10.0));
            //mSampler.addSound(new juce::SamplerSound(<#const String &name#>, <#AudioFormatReader &source#>, <#const BigInteger &midiNotes#>, <#int midiNoteForNormalPitch#>, <#double attackTimeSecs#>, <#double releaseTimeSecs#>, <#double maxSampleLengthSeconds#>))
            
        }
        else
        {
            DBG("Error: Failed to create AudioFormatReader or invalid sample rate.");
        }
    });
}

void ProtectedSoundsAudioProcessor::updateADSR(){
    
    for (int i = 0; i < mSampler.getNumSounds(); ++i){
        DBG("NUM SOUNDS" << mSampler.getNumSounds());

        
        if(auto sound = dynamic_cast<juce::SamplerSound*>(mSampler.getSound(i).get())){
            sound->setEnvelopeParameters(mADSRParams);
        }
    }
}


//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ProtectedSoundsAudioProcessor();
}

