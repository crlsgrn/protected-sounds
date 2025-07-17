#pragma once
#include <JuceHeader.h>
#include "ProtectedSoundsManager.h"

class ProtectedSoundsAudioProcessor : public juce::AudioProcessor,
                                    public juce::ValueTree::Listener
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

    void loadProtectedSound1(const juce::String& soundName);
    void loadProtectedSound2(const juce::String& soundName);
    void loadProtectedSoundPair(const juce::String& soundName);
    void loadProtectedSoundPairForSampler1(const juce::String& soundName);
    void loadProtectedSoundPairForSampler2(const juce::String& soundName);
    void loadSoundPairForSelector1(const juce::String& soundName);
    void loadSoundPairForSelector2(const juce::String& soundName);
    juce::StringArray getAvailableSounds() const;
    void updateADSR();
    
    juce::ADSR::Parameters& getADSRParams() { return mADSRParams; }
    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }
    //loop
    void setLoopEnabled(bool shouldLoop) { loopEnabled.store(shouldLoop); }
    bool isLooping() const { return loopEnabled.load(); }
    double getAudioLength() const { return audioLength.load(); }
    void setLoopPoints(int64_t startSamples, int64_t endSamples);
    //void setLoopPoints(double startMs, double endMs);
    double getLoopStart() const { return loopStartPosition.load(); }
    double getLoopEnd() const { return loopEndPosition.load(); }
    //filtro
    void setFilterFrequency(float frequency);
    void setFilterResonance(float resonance);
    float getFilterFrequency() const { return filterFrequency; }
    
    juce::AudioBuffer<float>& getWaveForm() { return waveForm; }
    const juce::String& getFileName() const { return fileName; }
    void setWaveForm(const juce::AudioBuffer<float>& newWaveForm) { waveForm = newWaveForm; }
    void setFileName(const juce::String& name) { fileName = name; }
    void updateEditorLoopSliders();


private:
    juce::Synthesiser mSampler1;
    juce::Synthesiser mSampler2;
    // En la sección private:
    juce::Synthesiser mSampler1Clean;
    juce::Synthesiser mSampler1Excited;
    juce::Synthesiser mSampler2Clean;
    juce::Synthesiser mSampler2Excited;
    const int mNumVoices { 3 };
    
    juce::dsp::Limiter<float> limiter;
    juce::ADSR::Parameters mADSRParams;
    juce::ADSR::Parameters mADSRParams2;
    juce::AudioBuffer<float> tempBuffer;
    
    juce::AudioFormatManager mFormatManager;
    juce::AudioFormatManager mFormatManager2;
    juce::AudioFormatReader* mFormatReader { nullptr };
    juce::AudioFormatReader* mFormatReader2 { nullptr };
    
    juce::AudioProcessorValueTreeState apvts;
    juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    void valueTreePropertyChanged(juce::ValueTree& treeWhosePropertyHasChanged,
                                const juce::Identifier& property) override;
    
    std::atomic<bool> sUpdate { false };
    std::atomic<bool> isNotePlaying { false };
    std::atomic<int> currentNoteNumber { -1 };
    std::atomic<bool> loopEnabled { false };
    std::atomic<double> audioLength { 0.0 };
    //std::atomic<double> loopStartPosition { 0.0 };
    //std::atomic<double> loopEndPosition { 0.0 };
    //double currentPosition = 0.0;
    std::atomic<int64_t> loopStartPosition{0};  // en samples
    std::atomic<int64_t> loopEndPosition{0};    // en samples
    std::atomic<int64_t> currentSamplePosition{0}; // en samples
    
    ProtectedSoundsManager soundsManager;
    
    juce::dsp::StateVariableTPTFilter<float> filter;
    float filterFrequency = 1000.0f;
    float filterResonance = 0.7f;
    
    float mixAmount = 0.5f;

    //waveform
    juce::AudioBuffer<float> waveForm;
    juce::String fileName;
    

    
    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProtectedSoundsAudioProcessor)
};
