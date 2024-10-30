/*
  ==============================================================================

    AudioEncryptor.cpp
    Created: 6 Aug 2024 7:04:36pm
    Author:  Carlos Garin

  ==============================================================================
*/

#pragma once

#define JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED 1
#define JUCE_STANDALONE_APPLICATION 1
#define JUCE_USE_CURL 0

#include "JuceHeader.h"
#include </Users/carlosgarin/Desktop/protected-sounds/protectedSounds/JuceLibraryCode/JuceHeader.h>
#include <juce_core/juce_core.h>
#include <juce_cryptography/juce_cryptography.h>
#include <iostream>

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cout << "Usage: encrypt_audio <input_file> <output_file>" << std::endl;
        return 1;
    }

    juce::File inputFile(argv[1]);
    juce::File outputFile(argv[2]);

    if (!inputFile.existsAsFile())
    {
        std::cout << "Input file does not exist." << std::endl;
        return 1;
    }

    juce::MemoryBlock originalData;
    inputFile.loadFileAsData(originalData);

    juce::String encryptionKey = "clave";
    juce::MemoryBlock encryptedData;

    juce::BlowFish blowfish(encryptionKey.toUTF8(), encryptionKey.length());

    // Asegurarse de que el tamaño de los datos es un múltiplo de 8 bytes (64 bits)
    size_t paddedSize = ((originalData.getSize() + 7) / 8) * 8;
    originalData.setSize(paddedSize, true);

    encryptedData.setSize(paddedSize);

    // Encriptar los datos en bloques de 64 bits
    for (size_t i = 0; i < paddedSize; i += 8)
    {
        juce::uint32& left = *reinterpret_cast<juce::uint32*>(originalData.begin() + i);
        juce::uint32& right = *reinterpret_cast<juce::uint32*>(originalData.begin() + i + 4);
        
        blowfish.encrypt(left, right);
        
        *reinterpret_cast<juce::uint32*>(encryptedData.begin() + i) = left;
        *reinterpret_cast<juce::uint32*>(encryptedData.begin() + i + 4) = right;
    }

    if (encryptedData.getSize() > 0)
    {
        outputFile.create();
        outputFile.appendData(encryptedData.getData(), encryptedData.getSize());
        std::cout << "Encryption successful." << std::endl;
        return 0;
    }
    else
    {
        std::cout << "Encryption failed." << std::endl;
        return 1;
    }
    
}
