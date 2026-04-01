#include "orion/OscillatorNode.h"
#include "orion/AudioBlock.h"
#include <iostream>
#include <chrono>
#include <vector>

using namespace Orion;

int main() {
    const int numChannels = 2;
    const int blockSize = 1024;
    const int numBlocks = 50000;
    const float sampleRate = 44100.0f;

    OscillatorNode osc;
    osc.setFormat(numChannels, blockSize);
    osc.setSampleRate(sampleRate);
    osc.setFrequency(440.0f);

    auto run_bench = [&](Waveform wf, const char* name) {
        osc.setWaveform(wf);
        // Reset phases to have consistent runs
        osc.setFormat(numChannels, blockSize);

        auto start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < numBlocks; ++i) {
            osc.getOutput(i, i * blockSize);
        }
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        std::cout << name << ": " << duration << " us" << std::endl;
    };

    std::cout << "Starting Benchmark (BlockSize: " << blockSize << ", Blocks: " << numBlocks << ")" << std::endl;
    run_bench(Waveform::Sine, "Sine");
    run_bench(Waveform::Square, "Square");
    run_bench(Waveform::Sawtooth, "Sawtooth");
    run_bench(Waveform::Triangle, "Triangle");

    return 0;
}
