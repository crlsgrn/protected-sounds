
#include "PluginEditor.h"

ProtectedSoundsAudioProcessorEditor::ProtectedSoundsAudioProcessorEditor(ProtectedSoundsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{

    setupButtons();
    setupSliders();
    setupLabels();
    setupLoopControls();
    setupPositionDisplay();

    // Configurar selectores de sonido
    addAndMakeVisible(soundSelector1);
    addAndMakeVisible(soundSelector2);
    
    // Asegurar que los ComboBox estén vacíos antes de agregar items
    soundSelector1.clear();
    soundSelector2.clear();
    
    // Agregar items
    auto sounds = audioProcessor.getAvailableSounds();
    soundSelector1.addItemList(sounds, 1);
    soundSelector2.addItemList(sounds, 1);
    
    // Seleccionar primer item por defecto si existe
    if (sounds.size() > 0)
    {
        soundSelector1.setSelectedItemIndex(0, juce::dontSendNotification);
        soundSelector2.setSelectedItemIndex(0, juce::dontSendNotification);
    }
    
    // Configurar callbacks
    soundSelector1.onChange = [this]() {
        if (soundSelector1.getSelectedItemIndex() >= 0)
            audioProcessor.loadProtectedSound1(soundSelector1.getText());
    };
    
    soundSelector2.onChange = [this]() {
        if (soundSelector2.getSelectedItemIndex() >= 0)
            audioProcessor.loadProtectedSound2(soundSelector2.getText());
    };

    startTimer(50);
    setSize(800, 300);

}

ProtectedSoundsAudioProcessorEditor::~ProtectedSoundsAudioProcessorEditor()
{
    stopTimer();
}

void ProtectedSoundsAudioProcessorEditor::setupButtons()
{
    addAndMakeVisible(playButton);
    addAndMakeVisible(stopButton);
    addAndMakeVisible(loopButton);

    playButton.onClick = [this]() {
        if (audioProcessor.transportSource.isPlaying())
        {
            audioProcessor.transportSource.stop();
            playButton.setButtonText("Play");
        }
        else
        {
            audioProcessor.transportSource.start();
            playButton.setButtonText("Stop");
        }
    };

    stopButton.onClick = [this]() {
        audioProcessor.transportSource.stop();
        audioProcessor.transportSource.setPosition(0.0);
        playButton.setButtonText("Play");
    };

    loopButton.onClick = [this]() {
        bool shouldLoop = loopButton.getToggleState();
        audioProcessor.setLoopEnabled(shouldLoop);
    };
}

void ProtectedSoundsAudioProcessorEditor::setupSliders()
{
    // ADSR Sliders setup
    auto setupADSRSlider = [this](juce::Slider& slider, const juce::String& name) {
        addAndMakeVisible(slider);
        slider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
        slider.setRange(0.0f, name.contains("Sustain") ? 1.0f : 5.0f, 0.01f);
    };

    setupADSRSlider(mAttackSlider, "Attack");
    setupADSRSlider(mDecaySlider, "Decay");
    setupADSRSlider(mSustainSlider, "Sustain");
    setupADSRSlider(mReleaseSlider, "Release");
    setupADSRSlider(mAttackSlider2, "Attack2");
    setupADSRSlider(mDecaySlider2, "Decay2");
    setupADSRSlider(mSustainSlider2, "Sustain2");
    setupADSRSlider(mReleaseSlider2, "Release2");

    // ADSR attachments
    mAttackAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Attack", mAttackSlider);
    mDecayAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Decay", mDecaySlider);
    mSustainAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Sustain", mSustainSlider);
    mReleaseAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Release", mReleaseSlider);
        
    mAttackAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Attack2", mAttackSlider2);
    mDecayAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Decay2", mDecaySlider2);
    mSustainAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Sustain2", mSustainSlider2);
    mReleaseAttachment2 = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "Release2", mReleaseSlider2);

    // Loop control sliders
    addAndMakeVisible(loopStartSlider);
    addAndMakeVisible(loopEndSlider);
    
    loopStartSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    loopEndSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    
    loopStartSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    loopEndSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    
    loopStartSlider.setRange(0, 10000, 1);
    loopEndSlider.setRange(0, 10000, 1);
    
    loopStartSlider.setValue(0);
    loopEndSlider.setValue(10000);

    loopStartSlider.onValueChange = [this]() { updateLoopPoints(); };
    loopEndSlider.onValueChange = [this]() { updateLoopPoints(); };
}

void ProtectedSoundsAudioProcessorEditor::setupPositionDisplay()
{
    addAndMakeVisible(positionSlider);
    addAndMakeVisible(positionLabel);
    
    positionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    positionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    positionSlider.setRange(0.0, 1.0);
    
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(15.0f);
}

void ProtectedSoundsAudioProcessorEditor::setupLabels()
{
    auto setupADSRLabel = [this](juce::Label& label, juce::Slider& slider, const juce::String& text) {
        addAndMakeVisible(label);
        label.setText(text, juce::NotificationType::dontSendNotification);
        label.setFont(10.0f);
        label.setJustificationType(juce::Justification::centredTop);
        label.attachToComponent(&slider, false);
    };

    setupADSRLabel(mAttackLabel, mAttackSlider, "Attack");
    setupADSRLabel(mDecayLabel, mDecaySlider, "Decay");
    setupADSRLabel(mSustainLabel, mSustainSlider, "Sustain");
    setupADSRLabel(mReleaseLabel, mReleaseSlider, "Release");
    setupADSRLabel(mAttackLabel2, mAttackSlider2, "Attack2");
    setupADSRLabel(mDecayLabel2, mDecaySlider2, "Decay2");
    setupADSRLabel(mSustainLabel2, mSustainSlider2, "Sustain2");
    setupADSRLabel(mReleaseLabel2, mReleaseSlider2, "Release2");

    addAndMakeVisible(loopStartLabel);
    addAndMakeVisible(loopEndLabel);
}

void ProtectedSoundsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ProtectedSoundsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Position display area
    auto topArea = area.removeFromTop(40);
    positionSlider.setBounds(topArea.reduced(10, 5));
    positionLabel.setBounds(topArea.removeFromRight(100));

    // Transport controls
    area.removeFromTop(5);
    auto controlsArea = area.removeFromTop(30);
    playButton.setBounds(controlsArea.removeFromLeft(100).reduced(5));
    stopButton.setBounds(controlsArea.removeFromLeft(100).reduced(5));
    loopButton.setBounds(controlsArea.removeFromLeft(100).reduced(5));

    // Sound selectors
    area.removeFromTop(5);
    auto selectorsArea = area.removeFromTop(30);
    soundSelector1.setBounds(selectorsArea.removeFromLeft(200).reduced(5));
    soundSelector2.setBounds(selectorsArea.removeFromLeft(200).reduced(5));

    // ADSR sliders
    const auto startX = 0.6f;
    const auto startY = 0.6f;
    const auto dialWidth = 0.1f;
    const auto dialHeight = 0.4f;

    mAttackSlider.setBoundsRelative(startX, startY, dialWidth, dialHeight);
    mDecaySlider.setBoundsRelative(startX + dialWidth, startY, dialWidth, dialHeight);
    mSustainSlider.setBoundsRelative(startX + (dialWidth * 2), startY, dialWidth, dialHeight);
    mReleaseSlider.setBoundsRelative(startX + (dialWidth * 3), startY, dialWidth, dialHeight);

    mAttackSlider2.setBoundsRelative(startX - 0.5f, startY, dialWidth, dialHeight);
    mDecaySlider2.setBoundsRelative(startX - 0.5f + dialWidth, startY, dialWidth, dialHeight);
    mSustainSlider2.setBoundsRelative(startX - 0.5f + (dialWidth * 2), startY, dialWidth, dialHeight);
    mReleaseSlider2.setBoundsRelative(startX - 0.5f + (dialWidth * 3), startY, dialWidth, dialHeight);

    // Loop controls
    loopStartSlider.setBounds(10, 200, 200, 20);
    loopEndSlider.setBounds(10, 230, 200, 20);
    loopStartLabel.setBounds(220, 200, 100, 20);
    loopEndLabel.setBounds(220, 230, 100, 20);
}

void ProtectedSoundsAudioProcessorEditor::updateLoopPoints()
{
    double start = loopStartSlider.getValue();
    double end = loopEndSlider.getValue();
    audioProcessor.setLoopPoints(start, end);
}

void ProtectedSoundsAudioProcessorEditor::updatePositionDisplay()
{
    double currentPos = audioProcessor.getCurrentPosition();
    double totalLength = audioProcessor.getLengthInSeconds();
    
    if (totalLength > 0.0)
    {
        positionSlider.setValue(currentPos / totalLength, juce::dontSendNotification);
        positionLabel.setText(formatTime(currentPos) + " / " + formatTime(totalLength),
                            juce::dontSendNotification);
    }
    else
    {
        positionSlider.setValue(0.0, juce::dontSendNotification);
        positionLabel.setText("0:00 / 0:00", juce::dontSendNotification);
    }
}

void ProtectedSoundsAudioProcessorEditor::timerCallback()
{
    updatePositionDisplay();
    updateTransportState();
}

void ProtectedSoundsAudioProcessorEditor::updateTransportState()
{
    if (audioProcessor.transportSource.isPlaying())
    {
        playButton.setButtonText("Stop");
    }
    else
    {
        playButton.setButtonText("Play");
    }
}

juce::String ProtectedSoundsAudioProcessorEditor::formatTime(double timeInSeconds)
{
    int minutes = static_cast<int>(timeInSeconds) / 60;
    int seconds = static_cast<int>(timeInSeconds) % 60;
    return juce::String(minutes) + ":" + juce::String(seconds).paddedLeft('0', 2);
}

void ProtectedSoundsAudioProcessorEditor::setupLoopControls()
{
    addAndMakeVisible(positionSlider);
    addAndMakeVisible(positionLabel);
    
    positionSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    positionSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    positionSlider.setRange(0.0, 1.0);
    
    positionLabel.setJustificationType(juce::Justification::centred);
    positionLabel.setFont(15.0f);

    // Loop controls
    addAndMakeVisible(loopStartSlider);
    addAndMakeVisible(loopEndSlider);
    addAndMakeVisible(loopStartLabel);
    addAndMakeVisible(loopEndLabel);
    
    loopStartSlider.setRange(0.0, 10000.0, 1.0);
    loopEndSlider.setRange(0.0, 10000.0, 1.0);
    
    loopStartSlider.onValueChange = [this]() { updateLoopPoints(); };
    loopEndSlider.onValueChange = [this]() { updateLoopPoints(); };
    
    loopStartLabel.setText("Loop Start (ms)", juce::dontSendNotification);
    loopEndLabel.setText("Loop End (ms)", juce::dontSendNotification);
}
