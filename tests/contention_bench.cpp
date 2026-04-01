#include "orion/AudioTrack.h"
#include "orion/AudioBlock.h"
#include "orion/AudioClip.h"
#include "orion/EffectNode.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>
#include <atomic>
#include <random>
#include <cmath>

using namespace Orion;

// Subclass to expose protected process method
class TestableAudioTrack : public AudioTrack {
public:
    void processPublic(AudioBlock& block, uint64_t frameStart) {
        AudioTrack::process(block, frameStart);
    }
};

// Mock EffectNode for testing
class MockEffect : public EffectNode {
protected:
    void process(AudioBlock& block, uint64_t frameStart) override {
        // heavy work simulation
        volatile float x = 0.0f;
        for(int i=0; i<500; ++i) {
             x += std::sin((float)i * 0.01f);
        }
    }
public:
    std::string getName() const override { return "MockEffect"; }
};

int main() {
    TestableAudioTrack track;
    AudioBlock block;
    block.resize(2, 512);
    block.zero();

    // Add some clips and effects
    for(int i=0; i<10; ++i) {
        auto clip = std::make_shared<AudioClip>();
        track.addClip(clip);
    }

    for(int i=0; i<5; ++i) {
        track.addEffect(std::make_shared<MockEffect>());
    }

    std::atomic<bool> running{true};
    std::atomic<long long> totalProcessTime{0};
    std::atomic<long> processCount{0};

    // Thread 1: Audio Process
    std::thread audioThread([&]() {
        uint64_t frame = 0;
        while(running) {
            auto start = std::chrono::high_resolution_clock::now();
            track.processPublic(block, frame);
            auto end = std::chrono::high_resolution_clock::now();

            totalProcessTime += std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            processCount++;

            frame += 512;
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
        }
    });

    // Thread 2: UI Interaction (Contention)
    std::thread uiThread([&]() {
        std::default_random_engine rng(42);
        std::uniform_int_distribution<int> dist(0, 100);

        while(running) {
            track.setVolume(0.8f + (dist(rng) / 1000.0f));
            track.setPan(0.1f);

            if (dist(rng) < 5) {
                auto clip = std::make_shared<AudioClip>();
                track.addClip(clip);
                std::this_thread::sleep_for(std::chrono::microseconds(100));
                track.removeClip(clip);
            }

            std::this_thread::sleep_for(std::chrono::microseconds(500));
        }
    });

    std::this_thread::sleep_for(std::chrono::seconds(5));
    running = false;
    audioThread.join();
    uiThread.join();

    double avgTime = (double)totalProcessTime / processCount;
    std::cout << "Average process time: " << avgTime << " ns" << std::endl;
    std::cout << "Total process calls: " << processCount << std::endl;

    return 0;
}
