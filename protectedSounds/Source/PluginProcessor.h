/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
*/
class ProtectedSoundsAudioProcessor  : public juce::AudioProcessor, public juce::ValueTree::Listener
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif

{
public:
    //==============================================================================
    ProtectedSoundsAudioProcessor();
    ~ProtectedSoundsAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    void loadFile1();
    void loadFile2();

    void updateADSR();
    
    void limit(juce::AudioBuffer<float>& buffer, float threshold);

    juce::ADSR::Parameters& getADSRParams() { return mADSRParams; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; } //to make it public and use it in plugin editor 
    
private:
    juce::Synthesiser mSampler1;
    juce::Synthesiser mSampler2;
    
    const int mNumVoices { 3 };
    
    juce::dsp::Limiter<float> limiter;
    
    juce::ADSR::Parameters mADSRParams;
    juce::AudioBuffer<float> tempBuffer;

    
    juce::AudioFormatManager mFormatManager;
    juce::AudioFormatManager mFormatManager2;

    juce::AudioFormatReader* mFormatReader { nullptr };
    juce::AudioFormatReader* mFormatReader2 { nullptr };

    
    std::unique_ptr<juce::FileChooser> fileChooser;
    std::unique_ptr<juce::FileChooser> fileChooser2;


    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void valueTreePropertyChanged (juce::ValueTree& treeWhosePropertyHasChanged, const juce::Identifier& property) override;
    
    std::atomic<bool> sUpdate { false };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProtectedSoundsAudioProcessor)
};
