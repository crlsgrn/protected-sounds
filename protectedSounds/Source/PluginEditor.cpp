#include "PluginEditor.h"

ProtectedSoundsAudioProcessorEditor::ProtectedSoundsAudioProcessorEditor(ProtectedSoundsAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setupButtons();
    setupSliders();
    setupLabels();
    

    // Setup sound selectors
    //addAndMakeVisible(soundSelector1);
    addAndMakeVisible(soundSelector2);
    
    // Asegurar que los ComboBox estén vacíos antes de agregar items
    soundSelector1.clear();
    soundSelector2.clear();
    
    // Configuración del slider de frecuencia del filtro
    filterFreqSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    filterFreqSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    filterFreqSlider.setRange(20.0f, 20000.0f, 1.0f);
    filterFreqSlider.setSkewFactorFromMidPoint(1000.0f); // Hace que el control sea más sensible alrededor de 1kHz
    addAndMakeVisible(filterFreqSlider);
    
    filterFreqLabel.setFont(10.0f);
    filterFreqLabel.setText("Frequency", juce::NotificationType::dontSendNotification);
    filterFreqLabel.setJustificationType(juce::Justification::centredTop);
    filterFreqLabel.attachToComponent(&filterFreqSlider, false);
    addAndMakeVisible(filterFreqLabel);
    
    filterFreqAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "FilterFreq", filterFreqSlider);

    // Configuración del slider de resonancia del filtro
    filterResSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    filterResSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 80, 20);
    filterResSlider.setRange(0.1f, 1.0f, 0.01f);
    addAndMakeVisible(filterResSlider);
    
    filterResLabel.setFont(10.0f);
    filterResLabel.setText("Resonance", juce::NotificationType::dontSendNotification);
    filterResLabel.setJustificationType(juce::Justification::centredTop);
    filterResLabel.attachToComponent(&filterResSlider, false);
    addAndMakeVisible(filterResLabel);
    
    filterResAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "FilterRes", filterResSlider);
    
    mixSlider.setSliderStyle(juce::Slider::SliderStyle::RotaryVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, true, 40, 20);
    mixSlider.setRange(0.0f, 100.0f, 0.1f);
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix %", juce::dontSendNotification);
    mixLabel.attachToComponent(&mixSlider, false);

    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        audioProcessor.getAPVTS(), "MixAmount", mixSlider);
    
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
            audioProcessor.loadProtectedSoundPair(soundSelector1.getText());
    };
    
    soundSelector2.onChange = [this]() {
        if (soundSelector2.getSelectedItemIndex() >= 0)
            audioProcessor.loadProtectedSoundPair(soundSelector2.getText());
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

    auto waveform = audioProcessor.getWaveForm();
    
    if (waveform.getNumSamples() > 0)
    {
        juce::Path p;
        audioPoints.clear();
        
        // Área para dibujar la forma de onda
        auto waveformBounds = getLocalBounds().reduced(10).removeFromTop(100);
        
        auto ratio = waveform.getNumSamples() / waveformBounds.getWidth();
        auto buffer = waveform.getReadPointer(0);
        
        // Escalar audio al ancho de la ventana
        for (int sample = 0; sample < waveform.getNumSamples(); sample += ratio)
        {
            audioPoints.push_back(buffer[sample]);
        }
        
        g.setColour(juce::Colours::yellow);
        p.startNewSubPath(waveformBounds.getX(), waveformBounds.getCentreY());
        
        // Escalar en el eje Y
        for (int sample = 0; sample < audioPoints.size(); ++sample)
        {
            auto point = juce::jmap<float>(audioPoints[sample], -1.0f, 1.0f,
                                         waveformBounds.getBottom(), waveformBounds.getY());
            p.lineTo(waveformBounds.getX() + sample, point);
        }
        
        g.strokePath(p, juce::PathStrokeType(2));
        
        // Dibujar marcadores de loop
        if (audioProcessor.isLooping())
        {
            auto startX = juce::jmap<float>(audioProcessor.getLoopStart(),
                                          0.0f, audioProcessor.getAudioLength(),
                                          waveformBounds.getX(), waveformBounds.getRight());
            
            auto endX = juce::jmap<float>(audioProcessor.getLoopEnd(),
                                        0.0f, audioProcessor.getAudioLength(),
                                        waveformBounds.getX(), waveformBounds.getRight());
            
            g.setColour(juce::Colours::red);
            g.drawLine(startX, waveformBounds.getY(), startX, waveformBounds.getBottom(), 2.0f);
            g.drawLine(endX, waveformBounds.getY(), endX, waveformBounds.getBottom(), 2.0f);
        }
    }
    
    auto waveformBounds = getLocalBounds().reduced(10).removeFromTop(100);

    // Dibujar marcadores de loop como áreas interactivas
    if (audioProcessor.isLooping())
    {
        auto startX = juce::jmap<float>(audioProcessor.getLoopStart(),
                                      0.0f, audioProcessor.getAudioLength(),
                                      waveformBounds.getX(), waveformBounds.getRight());
        
        auto endX = juce::jmap<float>(audioProcessor.getLoopEnd(),
                                    0.0f, audioProcessor.getAudioLength(),
                                    waveformBounds.getX(), waveformBounds.getRight());
        
        g.setColour(juce::Colours::red);
        
        // Dibujar marcadores con áreas para arrastrar
        g.drawLine(startX, waveformBounds.getY(), startX, waveformBounds.getBottom(), 2.0f);
        g.drawLine(endX, waveformBounds.getY(), endX, waveformBounds.getBottom(), 2.0f);
        
        // Áreas de agarre
        g.setColour(juce::Colours::red.withAlpha(0.3f));
        g.fillRect(juce::Rectangle<float>(startX - markerDragTolerance,
                                         waveformBounds.getY(),
                                         markerDragTolerance * 2,
                                         waveformBounds.getHeight()));
        g.fillRect(juce::Rectangle<float>(endX - markerDragTolerance,
                                         waveformBounds.getY(),
                                         markerDragTolerance * 2,
                                         waveformBounds.getHeight()));
    }
}


void ProtectedSoundsAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    auto waveformBounds = getLocalBounds().reduced(10).removeFromTop(100);
    
    auto startX = juce::jmap<float>(audioProcessor.getLoopStart(),
                                  0.0f, audioProcessor.getAudioLength(),
                                  waveformBounds.getX(), waveformBounds.getRight());
    
    auto endX = juce::jmap<float>(audioProcessor.getLoopEnd(),
                                0.0f, audioProcessor.getAudioLength(),
                                waveformBounds.getX(), waveformBounds.getRight());
    
    if (std::abs(e.x - startX) < markerDragTolerance && waveformBounds.contains(e.getPosition()))
        isDraggingStartMarker = true;
    else if (std::abs(e.x - endX) < markerDragTolerance && waveformBounds.contains(e.getPosition()))
        isDraggingEndMarker = true;
}

void ProtectedSoundsAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e)
{
    if (isDraggingStartMarker || isDraggingEndMarker)
    {
        auto waveformBounds = getLocalBounds().reduced(10).removeFromTop(100);
        auto timeInMs = juce::jmap<float>(e.x,
                                        waveformBounds.getX(), waveformBounds.getRight(),
                                        0.0f, audioProcessor.getAudioLength() * 1000.0f);
        
        timeInMs = juce::jlimit<float>(0.0f, audioProcessor.getAudioLength() * 1000.0f, timeInMs);
        
        if (isDraggingStartMarker)
        {
            loopStartSlider.setValue(timeInMs, juce::sendNotification);
        }
        else
        {
            loopEndSlider.setValue(timeInMs, juce::sendNotification);
        }
        
        repaint();
    }
}

void ProtectedSoundsAudioProcessorEditor::mouseUp(const juce::MouseEvent&)
{
    isDraggingStartMarker = false;
    isDraggingEndMarker = false;
}


void ProtectedSoundsAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(10);
    
    auto waveformArea = area.removeFromTop(100);
    
    // Area para controles de loop
    auto loopArea = area.removeFromTop(90);
    auto loopControlsLeft = loopArea.removeFromLeft(400);
    
    // Mix control
    mixSlider.setBounds(loopControlsLeft.removeFromRight(100).reduced(5));
    
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
