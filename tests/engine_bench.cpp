#include "orion/AudioEngine.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>

using namespace Orion;

int main() {
    AudioEngine engine;
    engine.play();

    // Setup buffers
    size_t numFrames = 128;
    size_t numChannels = 2;
    std::vector<float> outputBuffer(numFrames * numChannels);

    std::atomic<bool> running{true};
    std::atomic<long long> maxProcessTime{0};
    std::atomic<long long> totalProcessTime{0};
    std::atomic<long> processCount{0};

    // Audio Thread
    std::thread audioThread([&]() {
        while(running) {
            auto start = std::chrono::high_resolution_clock::now();
            engine.process(outputBuffer.data(), nullptr, numFrames, numChannels);
            auto end = std::chrono::high_resolution_clock::now();

            long long duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            if (duration > maxProcessTime) maxProcessTime = duration;
            totalProcessTime += duration;
            processCount++;

            // Simulate real-time constraint (wait remaining time)
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
    });

    // UI Thread (Heavy Contention)
    std::thread uiThread([&]() {
        while(running) {
            // Rapidly add/remove tracks
            auto track = engine.createTrack();
            // Busy wait a bit
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            engine.removeTrack(track);
            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
    running = false;
    audioThread.join();
    uiThread.join();

    double avgTime = (double)totalProcessTime / processCount;
    std::cout << "Average process time: " << avgTime << " ns" << std::endl;
    std::cout << "Max process time: " << maxProcessTime << " ns" << std::endl;

    return 0;
}
