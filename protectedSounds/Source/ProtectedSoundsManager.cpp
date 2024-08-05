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
}

juce::StringArray ProtectedSoundsManager::getAvailableSounds() const
{
    return availableSounds;
}

std::unique_ptr<juce::MemoryInputStream> ProtectedSoundsManager::loadSound(const juce::String& soundName)
{
    // Construye el nombre del recurso
    juce::String resourceName = soundName + ".wav";
    
    // Obtiene el tamaño y los datos del recurso
    int size = 0;
    const char* data = BinaryData::getNamedResource(resourceName.toRawUTF8(), size);
    
    if (data != nullptr && size > 0)
    {
        // Crea y devuelve un MemoryInputStream con los datos del recurso
        return std::make_unique<juce::MemoryInputStream>(data, size, false);
    }
    
    // Si no se encuentra el recurso, devuelve nullptr
    return nullptr;
}
