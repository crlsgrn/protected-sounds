#pragma once
#include <JuceHeader.h>
#include "ProtectedSoundsManager.h"

class ProtectedSoundsAudioProcessor : public juce::AudioProcessor,
                                    public juce::ValueTree::Listener
#if JucePlugin_Enable_ARA
    , public juce::AudioProcessorARAExtension
#endif
{
public:
    ProtectedSoundsAudioProcessor();
    ~ProtectedSoundsAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
#endif

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // Custom methods
    void loadProtectedSound1(const juce::String& soundName);
    void loadProtectedSound2(const juce::String& soundName);
    juce::StringArray getAvailableSounds() const;
    void updateADSR();
    
    // Getters
    juce::ADSR::Parameters& getADSRParams() { return mADSRParams; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    double getAudioLength() const { return audioLength.load(); }
    
    // Transport and loop control
    juce::AudioTransportSource transportSource;
    double getCurrentPosition() const { return transportSource.getCurrentPosition(); }
    double getLengthInSeconds() const { return transportSource.getLengthInSeconds(); }
    void setLoopPoints(double startMs, double endMs);
    void setLoopEnabled(bool shouldLoop);
    bool isLooping() const { return loopEnabled.load(); }

private:
    juce::Synthesiser mSampler1;
    juce::Synthesiser mSampler2;
    const int mNumVoices { 3 };
    
    // Audio processing
    juce::dsp::Limiter<float> limiter;
    juce::ADSR::Parameters mADSRParams;
    juce::ADSR::Parameters mADSRParams2;
    juce::AudioBuffer<float> tempBuffer;
    
    // Format handling
    juce::AudioFormatManager mFormatManager;
    juce::AudioFormatManager mFormatManager2;
    juce::AudioFormatReader* mFormatReader { nullptr };
    juce::AudioFormatReader* mFormatReader2 { nullptr };
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioFormatReader* currentReader { nullptr };
    
    // Thread handling
    juce::TimeSliceThread readAheadThread{"Audio File Reader"};
    
    // Parameters and state
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
    
    std::atomic<bool> sUpdate { false };
    ProtectedSoundsManager soundsManager;
    
    // Loop control
    std::atomic<double> loopStartPosition { 0.0 };
    std::atomic<double> loopEndPosition { 0.0 };
    std::atomic<bool> loopEnabled { false };
    std::atomic<double> audioLength { 0.0 };
    
    std::atomic<int> currentNoteNumber { -1 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProtectedSoundsAudioProcessor)
};
