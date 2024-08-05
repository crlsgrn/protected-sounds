/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class ProtectedSoundsAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    ProtectedSoundsAudioProcessorEditor (ProtectedSoundsAudioProcessor&);
    ~ProtectedSoundsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    
    
    //sliderValueChanged (Slider *slider)=0 esta =0 por lo que hay que inicialziar la funcion virtual
    //void sliderValueChanged (juce::Slider* slider) override;

private:
    juce::TextButton mLoadButton1 { "Load" };
    juce::TextButton mLoadButton2 { "Load 2nd sound" };

    
    ProtectedSoundsAudioProcessor& audioProcessor;
    
    juce::ComboBox soundSelector1;
    juce::ComboBox soundSelector2;
    
    juce::Slider mAttackSlider, mDecaySlider, mSustainSlider, mReleaseSlider;
    juce::Label mAttackLabel, mDecayLabel, mSustainLabel, mReleaseLabel;
    
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mAttackAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mDecayAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mSustainAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mReleaseAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProtectedSoundsAudioProcessorEditor)
};

