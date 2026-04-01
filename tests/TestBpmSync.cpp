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
    std::string testFile = "Test_120bpm.wav";
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

    std::cout << "Loaded Clip. BPM: " << clip.getOriginalBpm() << std::endl;
    // Filename BPM Detection
    // "Test_120bpm.wav" -> 120
    if (std::abs(clip.getOriginalBpm() - 120.0f) > 0.1f) {
        std::cerr << "BPM Detection Failed! Expected 120, Got " << clip.getOriginalBpm() << std::endl;
        return 1;
    }

    // Set Properties
    clip.setOriginalBpm(120.0f);
    clip.prepareToPlay(sampleRate, 512);

    // Test Resample
    clip.setSyncMode(SyncMode::Resample);
    AudioBlock output(2, 512); // Stereo output
    output.zero();

    // Process at 60 BPM (0.5x)
    clip.process(output, 0, 60.0);

    float maxVal = 0.0f;
    for (size_t i = 0; i < 512; ++i) {
        if (std::abs(output.getChannelPointer(0)[i]) > maxVal) maxVal = std::abs(output.getChannelPointer(0)[i]);
    }

    if (maxVal < 0.001f) {
        std::cerr << "Resample Output Silent!" << std::endl;
        return 1;
    }
    std::cout << "Resample Output OK. Max: " << maxVal << std::endl;

    // Test Stretch
    clip.setSyncMode(SyncMode::Stretch);
    output.zero();
    clip.process(output, 512, 60.0);

    maxVal = 0.0f;
    for (size_t i = 0; i < 512; ++i) {
        if (std::abs(output.getChannelPointer(0)[i]) > maxVal) maxVal = std::abs(output.getChannelPointer(0)[i]);
    }

    if (maxVal < 0.001f) {
        std::cerr << "Stretch Output Silent!" << std::endl;
        return 1;
    }
    std::cout << "Stretch Output OK. Max: " << maxVal << std::endl;

    std::cout << "All Tests Passed." << std::endl;
    return 0;
}
