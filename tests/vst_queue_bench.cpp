#include "orion/VSTNode.h"
#include "orion/VST2.h"
#include "orion/AudioBlock.h"
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <chrono>
#include <cstring>

using namespace Orion;

// Wrapper to expose protected methods
class TestVSTNode : public VSTNode {
public:
    using VSTNode::VSTNode;

    void publicProcess(AudioBlock& block, uint64_t frameStart) {
        process(block, frameStart);
    }

    void publicProcessMidi(int status, int data1, int data2, int sampleOffset) {
        processMidi(status, data1, data2, sampleOffset);
    }
};

// Mock VST2 Functions
static VST2::VstIntPtr VSTPluginDispatcher(VST2::AEffect* effect, VST2::VstInt32 opcode, VST2::VstInt32 index, VST2::VstIntPtr value, void* ptr, float opt) {
    (void)effect; (void)opcode; (void)index; (void)value; (void)ptr; (void)opt;
    return 0;
}

static void VSTPluginProcessReplacing(VST2::AEffect* effect, float** inputs, float** outputs, VST2::VstInt32 sampleFrames) {
    (void)effect; (void)inputs; (void)outputs; (void)sampleFrames;
    // Simulate some work to hold the lock (Wait, process holds the lock? NO)
    // process() holds the lock ONLY during swap.
}

static float VSTPluginGetParameter(VST2::AEffect* effect, VST2::VstInt32 index) {
    (void)effect; (void)index;
    return 0.0f;
}

static void VSTPluginSetParameter(VST2::AEffect* effect, VST2::VstInt32 index, float parameter) {
    (void)effect; (void)index; (void)parameter;
}

int main() {
    // 1. Setup Mock Effect
    VST2::AEffect mockEffect;
    std::memset(&mockEffect, 0, sizeof(mockEffect));
    mockEffect.magic = VST2::kEffectMagic;
    mockEffect.dispatcher = VSTPluginDispatcher;
    mockEffect.processReplacing = VSTPluginProcessReplacing;
    mockEffect.getParameter = VSTPluginGetParameter;
    mockEffect.setParameter = VSTPluginSetParameter;
    mockEffect.numInputs = 2;
    mockEffect.numOutputs = 2;
    mockEffect.numParams = 10;

    // 2. Initialize VSTNode
    TestVSTNode vstNode(&mockEffect, "MockVST");
    vstNode.setFormat(2, 512);

    // 3. Setup Benchmark
    const int NUM_ITERATIONS = 500000; // Increased
    const int EVENTS_PER_BURST = 10;
    std::atomic<bool> running{true};

    // Audio Block for processing (Channels, Frames)
    AudioBlock block(2, 512);

    // Producer Thread (MIDI Input)
    std::thread producer([&]() {
        while(running) {
            // Constant stream of events
            for(int i=0; i<EVENTS_PER_BURST; ++i) {
                vstNode.publicProcessMidi(0x90, 60, 100, i);
            }
            // No sleep
            std::this_thread::yield();
        }
    });

    // Consumer (Audio Processing Thread)
    // Warmup
    for(int i=0; i<100; ++i) vstNode.publicProcess(block, 0);

    auto start = std::chrono::high_resolution_clock::now();

    for(int i=0; i<NUM_ITERATIONS; ++i) {
        vstNode.publicProcess(block, 0);
    }

    auto end = std::chrono::high_resolution_clock::now();
    running = false;
    producer.join();

    std::chrono::duration<double, std::milli> duration = end - start;
    std::cout << "Processed " << NUM_ITERATIONS << " blocks in " << duration.count() << " ms" << std::endl;
    std::cout << "Average time per block: " << duration.count() / NUM_ITERATIONS << " ms" << std::endl;

    return 0;
}
