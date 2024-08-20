/*
  ==============================================================================

    AudioEncryptor.cpp
    Created: 6 Aug 2024 7:04:36pm
    Author:  Carlos Garin

  ==============================================================================
*/

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

    /*bool success = juce::BlowFish::encrypt(originalData,encryptedData,encryptionKey.toUTF8(),encryptionKey.length());

    if (success)
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
     */
}
