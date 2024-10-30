#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class ProtectedSoundsAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit ProtectedSoundsAudioProcessorEditor(ProtectedSoundsAudioProcessor&);
    ~ProtectedSoundsAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    
    juce::Slider loopStartSlider;
    juce::Slider loopEndSlider;
    juce::Label loopStartLabel{"", "Loop Start (ms)"};
    juce::Label loopEndLabel{"", "Loop End (ms)"};

private:
    ProtectedSoundsAudioProcessor& audioProcessor;

    // Sound selectors
    juce::ComboBox soundSelector1;
    juce::ComboBox soundSelector2;

    // Loop control
    juce::ToggleButton loopButton{"Loop"};

    // ADSR controls
    juce::Slider mAttackSlider, mDecaySlider, mSustainSlider, mReleaseSlider;
    juce::Slider mAttackSlider2, mDecaySlider2, mSustainSlider2, mReleaseSlider2;
    juce::Label mAttackLabel, mDecayLabel, mSustainLabel, mReleaseLabel;
    juce::Label mAttackLabel2, mDecayLabel2, mSustainLabel2, mReleaseLabel2;

    // APVTS attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttackAttachment2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDecayAttachment2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mSustainAttachment2;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mReleaseAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mReleaseAttachment2;

    void setupSliders();
    void setupLabels();
    void setupButtons();
    void updateLoopPoints();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProtectedSoundsAudioProcessorEditor)
};
