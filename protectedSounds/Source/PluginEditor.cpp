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
    //mLoadButton1.onClick = [&]() { audioProcessor.loadFile1(); };
    //addAndMakeVisible(mLoadButton1);
    
    //mLoadButton2.onClick = [&]() { audioProcessor.loadFile2(); };
    //addAndMakeVisible(mLoadButton2);
    
    addAndMakeVisible(soundSelector1);
    addAndMakeVisible(soundSelector2);

    soundSelector1.addItemList(audioProcessor.getAvailableSounds(), 1);
    soundSelector2.addItemList(audioProcessor.getAvailableSounds(), 1);

    soundSelector1.onChange = [this] { audioProcessor.loadProtectedSound1(soundSelector1.getText()); };
    soundSelector2.onChange = [this] { audioProcessor.loadProtectedSound2(soundSelector2.getText()); };

    
    //==============================================================================//
    
    mAttackSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mAttackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mAttackSlider.setRange(0.0f, 5.0f, 0.01f);
    addAndMakeVisible(mAttackSlider);
    
    mAttackLabel.setFont(10.0f);
    mAttackLabel.setText("Attack", juce::NotificationType::dontSendNotification);
    mAttackLabel.setJustificationType(juce::Justification::centredTop);
    mAttackLabel.attachToComponent(&mAttackSlider, false);
    
    mAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Attack", mAttackSlider);
    
    mAttackSlider2.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mAttackSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mAttackSlider2.setRange(0.0f, 5.0f, 0.01f);
    addAndMakeVisible(mAttackSlider2);
    
    mAttackLabel2.setFont(10.0f);
    mAttackLabel2.setText("Attack2", juce::NotificationType::dontSendNotification);
    mAttackLabel2.setJustificationType(juce::Justification::centredTop);
    mAttackLabel2.attachToComponent(&mAttackSlider2, false);
    
    mAttackAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Attack2", mAttackSlider2);
    
    //==============================================================================//
    
    mDecaySlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mDecaySlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mDecaySlider.setRange(0.0f, 5.0f, 0.01f);
    addAndMakeVisible(mDecaySlider);
    
    mDecayLabel.setFont(10.0f);
    mDecayLabel.setText("Decay", juce::NotificationType::dontSendNotification);
    mDecayLabel.setJustificationType(juce::Justification::centredTop);
    mDecayLabel.attachToComponent(&mDecaySlider, false);
    
    mDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Decay", mDecaySlider);
    
    mDecaySlider2.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mDecaySlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mDecaySlider2.setRange(0.0f, 5.0f, 0.01f);
    addAndMakeVisible(mDecaySlider2);
    
    mDecayLabel2.setFont(10.0f);
    mDecayLabel2.setText("Decay2", juce::NotificationType::dontSendNotification);
    mDecayLabel2.setJustificationType(juce::Justification::centredTop);
    mDecayLabel2.attachToComponent(&mDecaySlider2, false);
    
    mDecayAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Decay2", mDecaySlider2);

    //==============================================================================//
    
    mSustainSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mSustainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mSustainSlider.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mSustainSlider);
    
    mSustainLabel.setFont(10.0f);
    mSustainLabel.setText("Sustain", juce::NotificationType::dontSendNotification);
    mSustainLabel.setJustificationType(juce::Justification::centredTop);
    mSustainLabel.attachToComponent(&mSustainSlider, false);
    
    mSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Sustain", mSustainSlider);
    
    mSustainSlider2.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mSustainSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mSustainSlider2.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mSustainSlider2);
    
    mSustainLabel2.setFont(10.0f);
    mSustainLabel2.setText("Sustain2", juce::NotificationType::dontSendNotification);
    mSustainLabel2.setJustificationType(juce::Justification::centredTop);
    mSustainLabel2.attachToComponent(&mSustainSlider2, false);
    
    mSustainAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Sustain2", mSustainSlider);
    //==============================================================================//
    
    mReleaseSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mReleaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mReleaseSlider.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mReleaseSlider);
    
    mReleaseLabel.setFont(10.0f);
    mReleaseLabel.setText("Release", juce::NotificationType::dontSendNotification);
    mReleaseLabel.setJustificationType(juce::Justification::centredTop);
    mReleaseLabel.attachToComponent(&mReleaseSlider, false);
    
    mReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Release", mReleaseSlider);
    
    mReleaseSlider2.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mReleaseSlider2.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mReleaseSlider2.setRange(0.0f, 1.0f, 0.01f);
    addAndMakeVisible(mReleaseSlider2);
    
    mReleaseLabel2.setFont(10.0f);
    mReleaseLabel2.setText("Release2", juce::NotificationType::dontSendNotification);
    mReleaseLabel2.setJustificationType(juce::Justification::centredTop);
    mReleaseLabel2.attachToComponent(&mReleaseSlider2, false);
    
    mReleaseAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.getAPVTS(), "Release2", mReleaseSlider);
    
    setSize (800, 300);
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
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void ProtectedSoundsAudioProcessorEditor::resized()
{
    soundSelector1.setBounds(getWidth()/2 + 100, getHeight()/2 - 100, 100, 100);
    soundSelector2.setBounds(getWidth()/2 - 300, getHeight()/2 - 100, 100, 100);
    
    const auto startX = 0.6f;
    const auto startY = 0.6f;
    const auto dialWidth = 0.1f;
    const auto dialHeight = 0.4f;
    
    mAttackSlider.setBoundsRelative(startX, startY, dialWidth, dialHeight);
    mAttackSlider2.setBoundsRelative(startX - 0.5f , startY, dialWidth, dialHeight);
    mDecaySlider.setBoundsRelative(startX + dialWidth, startY, dialWidth, dialHeight);
    mDecaySlider2.setBoundsRelative(startX - 0.5f + dialWidth, startY, dialWidth, dialHeight);
    mSustainSlider.setBoundsRelative(startX + (dialWidth * 2), startY, dialWidth, dialHeight);
    mSustainSlider2.setBoundsRelative(startX - 0.5f + (dialWidth * 2), startY, dialWidth, dialHeight);
    mReleaseSlider.setBoundsRelative(startX + (dialWidth * 3), startY, dialWidth, dialHeight);
    mReleaseSlider2.setBoundsRelative(startX - 0.5f + (dialWidth * 3), startY, dialWidth, dialHeight);

}

