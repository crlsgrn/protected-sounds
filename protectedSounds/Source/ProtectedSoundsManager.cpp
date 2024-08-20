/*
  ==============================================================================

    ProtectedSoundsManager.cpp
    Created: 3 Aug 2024 12:42:27pm
    Author:  Carlos Garin

  ==============================================================================
*/

#include "ProtectedSoundsManager.h"
#include "BinaryData.h"
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>


ProtectedSoundsManager::ProtectedSoundsManager()
{
    // Añade los nombres de tus sonidos aquí
    // Asegúrate de que estos nombres coincidan con los nombres de los recursos que has añadido
    availableSounds = {"comb_57_68_v89_110"};
    formatManager.registerBasicFormats();
    encryptionKey = juce::String("clave").toUTF8();

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

std::unique_ptr<juce::MemoryInputStream> ProtectedSoundsManager::loadSoundEncrypted(const juce::String& soundName)
{
    int size;
    const char* encryptedData = BinaryData::getNamedResource((soundName + "_encrypted").toRawUTF8(), size);
    
    if (encryptedData != nullptr && size > 0)
    {
        // Crear una instancia de BlowFish
        juce::BlowFish blowfish(encryptionKey.toUTF8(), encryptionKey.length());

        // Copiar los datos encriptados a un nuevo buffer
        std::vector<juce::uint32> dataBuffer(size / sizeof(juce::uint32) + (size % sizeof(juce::uint32) != 0));
        std::memcpy(dataBuffer.data(), encryptedData, size);

        // Desencriptar los datos en bloques de 64 bits (dos uint32)
        for (size_t i = 0; i < dataBuffer.size() - 1; i += 2)
        {
            blowfish.decrypt(dataBuffer[i], dataBuffer[i + 1]);
        }

        // Crear un MemoryBlock con los datos desencriptados
        juce::MemoryBlock decryptedBlock(dataBuffer.data(), size);

        return std::make_unique<juce::MemoryInputStream>(decryptedBlock, true);
    }
    return nullptr;
}
