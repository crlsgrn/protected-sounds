/*
  ==============================================================================

    ProtectedSoundsManager.h
    Created: 3 Aug 2024 12:42:27pm
    Author:  Carlos Garin

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ProtectedSoundsManager
{
public:
    
    struct AudioPair {
        juce::String cleanName;
        juce::String excitedName;
    };
    
    ProtectedSoundsManager();
    ~ProtectedSoundsManager() = default;

    // Devuelve una lista de los nombres de los sonidos disponibles
    juce::StringArray getAvailableSounds() const;

    // Carga un sonido por su nombre y devuelve un MemoryInputStream
    std::unique_ptr<juce::MemoryInputStream> loadSound(const juce::String& soundName);
    std::unique_ptr<juce::MemoryInputStream> loadSoundEncrypted(const juce::String& soundName);
    
    std::pair<std::unique_ptr<juce::MemoryInputStream>, std::unique_ptr<juce::MemoryInputStream>>
    loadSoundPair(const juce::String& baseName);

    

private:
    std::vector<AudioPair> audioPairs;
    juce::StringArray availableSounds;
    juce::String encryptionKey;
    juce::AudioFormatManager formatManager;


};
