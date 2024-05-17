/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ProtectedSoundsAudioProcessorEditor::ProtectedSoundsAudioProcessorEditor (ProtectedSoundsAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    mLoadButton.onClick = [&]() { audioProcessor.loadFile(); };
    addAndMakeVisible(mLoadButton);
    
    //==============================================================================//
    
    mAttackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mAttackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mAttackSlider.setRange(0.0f, 5.0f, 0.01f);
    mAttackSlider.addListener(this);
    addAndMakeVisible(mAttackSlider);
    
    mAttackLabel.setFont(10.0f);
    mAttackLabel.setText("Attack", juce::NotificationType::dontSendNotification);
    mAttackLabel.setJustificationType(juce::Justification::centredTop);
    mAttackLabel.attachToComponent(&mAttackSlider, false);
    
    //==============================================================================//
    
    mDecaySlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mDecaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mDecaySlider.setRange(0.0f, 5.0f, 0.01f);
    addAndMakeVisible(mDecaySlider);
    
    mDecayLabel.setFont(10.0f);
    mDecayLabel.setText("Decay", juce::NotificationType::dontSendNotification);
    mDecayLabel.setJustificationType(juce::Justification::centredTop);
    mDecayLabel.attachToComponent(&mDecaySlider, false);
    
    //==============================================================================//
    
    mSustainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mSustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mSustainSlider.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mSustainSlider);
    
    mSustainLabel.setFont(10.0f);
    mSustainLabel.setText("Sustain", juce::NotificationType::dontSendNotification);
    mSustainLabel.setJustificationType(juce::Justification::centredTop);
    mSustainLabel.attachToComponent(&mSustainSlider, false);
    
    //==============================================================================//
    
    mReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mReleaseSlider.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mReleaseSlider);
    
    mReleaseLabel.setFont(10.0f);
    mReleaseLabel.setText("Release", juce::NotificationType::dontSendNotification);
    mReleaseLabel.setJustificationType(juce::Justification::centredTop);
    mReleaseLabel.attachToComponent(&mReleaseSlider, false);
    

    
    
    
    
    
    setSize (600, 200);
}

ProtectedSoundsAudioProcessorEditor::~ProtectedSoundsAudioProcessorEditor()
{
}

//==============================================================================
void ProtectedSoundsAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ProtectedSoundsAudioProcessorEditor::resized()
{
    mLoadButton.setBounds(getWidth()/2 - 50, getHeight()/2 - 50, 100, 100);
    
    const auto startX = 0.6f;
    const auto startY = 0.6f;
    const auto dialWidth = 0.1f;
    const auto dialHeight = 0.4f;
    
    mAttackSlider.setBoundsRelative(startX, startY, dialWidth, dialHeight);
    mDecaySlider.setBoundsRelative(startX + dialWidth, startY, dialWidth, dialHeight);
    mSustainSlider.setBoundsRelative(startX + (dialWidth * 2), startY, dialWidth, dialHeight);
    mReleaseSlider.setBoundsRelative(startX + (dialWidth * 3), startY, dialWidth, dialHeight);
}

void ProtectedSoundsAudioProcessorEditor::sliderValueChanged(juce::Slider *slider){
    //escuchando a cambios que el usuario haga en el slider de la UI
    if(slider == &mAttackSlider){
        audioProcessor.getADSRParams().attack = mAttackSlider.getValue();
    }else if (slider == &mDecaySlider){
        audioProcessor.getADSRParams().decay = mDecaySlider.getValue();
    }else if (slider == &mSustainSlider){
        audioProcessor.getADSRParams().sustain = mSustainSlider.getValue();
    }else if (slider == &mReleaseSlider){
        audioProcessor.getADSRParams().release = mReleaseSlider.getValue();
    }
    
    audioProcessor.updateADSR();
        
}


