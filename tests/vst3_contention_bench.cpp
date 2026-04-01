#include "orion/RingBuffer.h"
#include <vector>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <iostream>
#include <cstdint>

// Simulate Steinberg::Vst::Event
struct MockEvent {
    int32_t busIndex;
    int32_t sampleOffset;
    double ppqPosition;
    uint16_t flags;
    uint16_t type;
    struct {
        int16_t channel;
        int16_t pitch;
        float velocity;
        int32_t noteId;
    } noteOn;
};

// Simulate processing work (e.g. 500 microseconds per block is heavy but realistic for heavy plugins)
void simulatePluginProcess() {
    auto start = std::chrono::high_resolution_clock::now();
    while (std::chrono::high_resolution_clock::now() - start < std::chrono::microseconds(50));
}

class BaselineVST3 {
    std::vector<MockEvent> events;
    std::mutex eventMutex;
public:
    void processMidi(const MockEvent& e) {
        std::lock_guard<std::mutex> lock(eventMutex);
        events.push_back(e);
    }

    void process() {
        std::lock_guard<std::mutex> lock(eventMutex);
        // Simulate plugin reading events
        volatile size_t count = events.size();
        (void)count;

        simulatePluginProcess();
        events.clear();
    }
};

class OptimizedVST3 {
    std::vector<MockEvent> localEvents;
    Orion::RingBuffer<MockEvent> eventQueue{4096};
public:
    void processMidi(const MockEvent& e) {
        eventQueue.push(e);
    }

    void process() {
        MockEvent e;
        while(eventQueue.pop(e)) {
            localEvents.push_back(e);
        }

        volatile size_t count = localEvents.size();
        (void)count;

        simulatePluginProcess();
        localEvents.clear();
    }
};

template<typename Node>
void runBenchmark(const char* name) {
    Node node;
    std::atomic<bool> running{true};
    std::atomic<long> midiProcessed{0};

    // Producer Thread (MIDI)
    std::thread midiThread([&]() {
        MockEvent e = {};
        while(running) {
            // Simulate high rate midi
            for(int i=0; i<10; ++i) {
                node.processMidi(e);
                midiProcessed++;
            }
            std::this_thread::sleep_for(std::chrono::microseconds(10));
        }
    });

    // Consumer Thread (Audio)
    auto start = std::chrono::high_resolution_clock::now();
    const int numBlocks = 10000;

    for(int i=0; i<numBlocks; ++i) {
        node.process();
        // Simulate audio rate spacing (e.g. 48kHz, buffer 512 ~ 10ms)
        // But here we want to saturate to see lock contention overhead
        // So we just run tight loop, relying on simulatePluginProcess to throttle
    }

    auto end = std::chrono::high_resolution_clock::now();
    running = false;
    midiThread.join();

    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << name << ": " << duration.count() << " ms total time ("
              << (duration.count() / numBlocks) << " ms/block). MIDI Events pushed: " << midiProcessed << std::endl;
}

int main() {
    std::cout << "Starting Contention Benchmark..." << std::endl;
    runBenchmark<BaselineVST3>("Baseline (Mutex)");
    runBenchmark<OptimizedVST3>("Optimized (RingBuffer)");
    return 0;
}
