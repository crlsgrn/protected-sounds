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
class ProtectedSoundsAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Slider::Listener
{
public:
    ProtectedSoundsAudioProcessorEditor (ProtectedSoundsAudioProcessor&);
    ~ProtectedSoundsAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    //bool isInterestedInFileDrag (const juce::StringArray& files) override;
    //void filesDropped(const juce::StringArray& files, int x, int y) override;
    
    //sliderValueChanged (Slider *slider)=0 esta =0 por lo que hay que inicialziar la funcion virtual
    void sliderValueChanged (juce::Slider* slider) override;

private:
    juce::TextButton mLoadButton { "Load" };
    
    ProtectedSoundsAudioProcessor& audioProcessor;
    
    juce::Slider mAttackSlider, mDecaySlider, mSustainSlider, mReleaseSlider;
    juce::Label mAttackLabel, mDecayLabel, mSustainLabel, mReleaseLabel;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProtectedSoundsAudioProcessorEditor)
};

