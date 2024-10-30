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
    availableSounds = {"comb_57_68_v89_110", "encrypted_audio", "CiberEncriptado-two_notes"};
    formatManager.registerBasicFormats();
    encryptionKey = juce::String("mysecretkey").toUTF8();

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

        // Crear un buffer para los datos desencriptados
        juce::MemoryBlock decryptedBlock(size);
        char* decryptedData = static_cast<char*>(decryptedBlock.getData());

        // Desencriptar los datos en bloques de 8 bytes (64 bits)
        for (int i = 0; i < size; i += 8)
        {
            juce::uint32 left = *reinterpret_cast<const juce::uint32*>(encryptedData + i);
            juce::uint32 right = *reinterpret_cast<const juce::uint32*>(encryptedData + i + 4);
            
            blowfish.decrypt(left, right);
            
            *reinterpret_cast<juce::uint32*>(decryptedData + i) = left;
            *reinterpret_cast<juce::uint32*>(decryptedData + i + 4) = right;
        }

        // Eliminar el padding PKCS7 si se aplicó durante la encriptación
        int paddingSize = decryptedData[size - 1];
        if (paddingSize > 0 && paddingSize <= 8)
        {
            decryptedBlock.setSize(size - paddingSize, true);
        }

        return std::make_unique<juce::MemoryInputStream>(decryptedBlock, true);
    }
    return nullptr;
}
