#include "PluginEditor.h"

ProtectedSoundsAudioProcessorEditor::ProtectedSoundsAudioProcessorEditor(ProtectedSoundsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setupButtons();
    setupSliders();
    setupLabels();

    // Setup sound selectors
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

    setSize(800, 300);
}

ProtectedSoundsAudioProcessorEditor::~ProtectedSoundsAudioProcessorEditor() = default;

void ProtectedSoundsAudioProcessorEditor::setupButtons()
{
    addAndMakeVisible(loopButton);
    loopButton.setToggleState(false, juce::dontSendNotification);
    loopButton.onClick = [this]() {
        audioProcessor.setLoopEnabled(loopButton.getToggleState());
    };
}

void ProtectedSoundsAudioProcessorEditor::setupSliders()
{
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

    // Set up ADSR attachments
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
    
    addAndMakeVisible(loopStartSlider);
    addAndMakeVisible(loopEndSlider);
    addAndMakeVisible(loopStartLabel);
    addAndMakeVisible(loopEndLabel);

    // Configurar estilo
    loopStartSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    loopEndSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    
    loopStartSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    loopEndSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 60, 20);
    
    // Los rangos se actualizarán cuando se cargue un sonido
    loopStartSlider.setRange(0.0, 10000.0, 1.0);  // milisegundos
    loopEndSlider.setRange(0.0, 10000.0, 1.0);    // milisegundos
    
    loopStartSlider.onValueChange = [this]() { updateLoopPoints(); };
    loopEndSlider.onValueChange = [this]() { updateLoopPoints(); };
}

void ProtectedSoundsAudioProcessorEditor::updateLoopPoints()
{
    double start = loopStartSlider.getValue();
    double end = loopEndSlider.getValue();
    audioProcessor.setLoopPoints(start, end);
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
}

void ProtectedSoundsAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void ProtectedSoundsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    // Area para controles de loop
    auto loopArea = area.removeFromTop(90);
    auto loopControlsLeft = loopArea.removeFromLeft(400);
    
    // Botón de loop
    loopButton.setBounds(loopControlsLeft.removeFromTop(30).removeFromLeft(100).reduced(5));
    
    loopControlsLeft.removeFromTop(5);
    
    // Loop Start
    auto startRow = loopControlsLeft.removeFromTop(25);
    loopStartLabel.setBounds(startRow.removeFromLeft(100));
    loopStartSlider.setBounds(startRow);
    
    loopControlsLeft.removeFromTop(5);
    
    // Loop End
    auto endRow = loopControlsLeft.removeFromTop(25);
    loopEndLabel.setBounds(endRow.removeFromLeft(100));
    loopEndSlider.setBounds(endRow);

    // ADSR controls
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

    // Sound selectors
    soundSelector1.setBounds(getWidth()/2 + 100, getHeight()/2 - 50, 100, 50);
    soundSelector2.setBounds(getWidth()/2 - 300, getHeight()/2 - 50, 100, 50);
}
