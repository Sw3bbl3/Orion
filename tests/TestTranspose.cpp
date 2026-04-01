#include "orion/AudioClip.h"
#include "orion/WavWriter.h"
#include "orion/AudioBlock.h"
#include <iostream>
#include <cmath>
#include <cassert>
#include <vector>
#include <string>

using namespace Orion;

int main() {
    std::string testFile = "Test_Transpose.wav";
    int sampleRate = 44100;

    // Create File
    {
        WavWriter writer(testFile, sampleRate, 1); // Mono
        size_t numSamples = sampleRate * 2;
        std::vector<float> data(numSamples);
        for (size_t i = 0; i < numSamples; ++i) {
            data[i] = std::sin(2.0f * 3.14159f * 440.0f * i / sampleRate);
        }
        writer.write(data.data(), numSamples);
    }

    AudioClip clip;
    if (!clip.loadFromFile(testFile, sampleRate)) {
        std::cerr << "Failed to load test file" << std::endl;
        return 1;
    }

    clip.prepareToPlay(sampleRate, 512);

    // Test Transpose (Off mode, but with pitch set)
    clip.setSyncMode(SyncMode::Off);
    clip.setPitch(12.0f); // 1 Octave up

    AudioBlock output(2, 512);
    output.zero();

    // Process at 120 BPM (Default)
    clip.process(output, 0, 120.0);

    float maxVal = 0.0f;
    for (size_t i = 0; i < 512; ++i) {
        if (std::abs(output.getChannelPointer(0)[i]) > maxVal) maxVal = std::abs(output.getChannelPointer(0)[i]);
    }

    if (maxVal < 0.001f) {
        std::cerr << "Transpose (Off Mode) Output Silent!" << std::endl;
        return 1;
    }
    std::cout << "Transpose (Off Mode) Output OK. Max: " << maxVal << std::endl;

    // Test Transpose with Resample mode
    clip.setSyncMode(SyncMode::Resample);
    clip.setOriginalBpm(120.0f);
    output.zero();
    // Process at 60 BPM (0.5x speed) but +12 st pitch
    clip.process(output, 512, 60.0);

    maxVal = 0.0f;
    for (size_t i = 0; i < 512; ++i) {
        if (std::abs(output.getChannelPointer(0)[i]) > maxVal) maxVal = std::abs(output.getChannelPointer(0)[i]);
    }

    if (maxVal < 0.001f) {
        std::cerr << "Transpose (Resample Mode) Output Silent!" << std::endl;
        return 1;
    }
    std::cout << "Transpose (Resample Mode) Output OK. Max: " << maxVal << std::endl;

    std::cout << "Transpose Tests Passed." << std::endl;
    return 0;
}
