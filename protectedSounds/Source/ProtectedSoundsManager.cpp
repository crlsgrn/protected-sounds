/*
  ==============================================================================

    ProtectedSoundsManager.cpp
    Created: 3 Aug 2024 12:42:27pm
    Author:  Carlos Garin

  ==============================================================================
*/

#include "ProtectedSoundsManager.h"
#include "BinaryData.h"

ProtectedSoundsManager::ProtectedSoundsManager()
{
    // Añade los nombres de tus sonidos aquí
    // Asegúrate de que estos nombres coincidan con los nombres de los recursos que has añadido
    availableSounds = {"comb_57_68_v89_110"};
    formatManager.registerBasicFormats();

}

juce::StringArray ProtectedSoundsManager::getAvailableSounds() const
{
    return availableSounds;
}

std::unique_ptr<juce::MemoryInputStream> ProtectedSoundsManager::loadSound(const juce::String& soundName)
{
    int size;
    const char* data = BinaryData::getNamedResource((soundName + "_wav").toRawUTF8(), size);
    if (data != nullptr && size > 0)
    {
        return std::make_unique<juce::MemoryInputStream>(data, size, false);
    }
    return nullptr;
}
