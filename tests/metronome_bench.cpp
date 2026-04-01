#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>
#include <algorithm>
#include <cstring>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Parameters
const size_t SAMPLE_RATE = 44100;
const double BPM = 120.0;
const size_t NUM_CHANNELS = 2;
const size_t TOTAL_FRAMES = SAMPLE_RATE * 60; // 60 seconds to get measurable time
const size_t BLOCK_SIZE = 128;

// LUT
constexpr size_t LUT_SIZE = 4096;
constexpr size_t LUT_MASK = LUT_SIZE - 1;
std::vector<float> sineLUT;

void initLUT() {
    sineLUT.resize(LUT_SIZE);
    for (size_t i = 0; i < LUT_SIZE; ++i) {
        sineLUT[i] = std::sin(2.0 * M_PI * (double)i / (double)LUT_SIZE);
    }
}

void runBaseline(std::vector<float>& outputBuffer, size_t numFrames) {
    double framesPerBeat = (60.0 / BPM) * SAMPLE_RATE;
    int durFrames = (int)(0.1 * SAMPLE_RATE);

    // To simulate block processing somewhat, although here we run flat loop for max pressure
    for (size_t i = 0; i < numFrames; ++i) {
        uint64_t frame = i;

        uint64_t prevBeat = (uint64_t)(std::floor((double)frame / framesPerBeat) * framesPerBeat);
        uint64_t framesSinceBeat = frame - prevBeat;

        if (framesSinceBeat < (uint64_t)durFrames) {
            uint64_t beatIndex = (uint64_t)std::round((double)prevBeat / framesPerBeat);
            bool isBar = (beatIndex % 4 == 0);
            float freq = isBar ? 1200.0f : 800.0f;
            float t = (float)framesSinceBeat / (float)SAMPLE_RATE;
            float env = 1.0f - ((float)framesSinceBeat / durFrames);
            float sample = static_cast<float>(std::sin(2.0 * M_PI * freq * t)) * env * 0.5f;

            for(size_t ch=0; ch<NUM_CHANNELS; ++ch) {
                outputBuffer[i*NUM_CHANNELS + ch] += sample;
            }
        }
    }
}

void runOptimized(std::vector<float>& outputBuffer, size_t numFrames) {
    double framesPerBeat = (60.0 / BPM) * SAMPLE_RATE;
    int durFrames = (int)(0.1 * SAMPLE_RATE);

    // Precalculate increments
    float inc1200 = 1200.0f * (float)LUT_SIZE / (float)SAMPLE_RATE;
    float inc800 = 800.0f * (float)LUT_SIZE / (float)SAMPLE_RATE;

    for (size_t i = 0; i < numFrames; ++i) {
        uint64_t frame = i;

        uint64_t prevBeat = (uint64_t)(std::floor((double)frame / framesPerBeat) * framesPerBeat);
        uint64_t framesSinceBeat = frame - prevBeat;

        if (framesSinceBeat < (uint64_t)durFrames) {
            uint64_t beatIndex = (uint64_t)std::round((double)prevBeat / framesPerBeat);
            bool isBar = (beatIndex % 4 == 0);

            float inc = isBar ? inc1200 : inc800;

            // Optimization: Use LUT
            // Phase index = (framesSinceBeat * inc) % LUT_SIZE
            // Note: casting float to size_t truncates towards zero.
            // Using & LUT_MASK handles wrapping if the index goes above LUT_SIZE (which it will).

            size_t index = (size_t)(framesSinceBeat * inc) & LUT_MASK;

            float env = 1.0f - ((float)framesSinceBeat / durFrames);
            float sample = sineLUT[index] * env * 0.5f;

            for(size_t ch=0; ch<NUM_CHANNELS; ++ch) {
                outputBuffer[i*NUM_CHANNELS + ch] += sample;
            }
        }
    }
}

int main() {
    initLUT();
    std::vector<float> buffer1(TOTAL_FRAMES * NUM_CHANNELS, 0.0f);
    std::vector<float> buffer2(TOTAL_FRAMES * NUM_CHANNELS, 0.0f);

    std::cout << "Benchmarking Metronome Logic (" << TOTAL_FRAMES << " frames / " << TOTAL_FRAMES/SAMPLE_RATE << "s)..." << std::endl;

    auto start1 = std::chrono::high_resolution_clock::now();
    runBaseline(buffer1, TOTAL_FRAMES);
    auto end1 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff1 = end1 - start1;
    std::cout << "Baseline (std::sin): " << diff1.count() << " s" << std::endl;

    auto start2 = std::chrono::high_resolution_clock::now();
    runOptimized(buffer2, TOTAL_FRAMES);
    auto end2 = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff2 = end2 - start2;
    std::cout << "Optimized (LUT):     " << diff2.count() << " s" << std::endl;

    if (diff2.count() > 0) {
        double speedup = diff1.count() / diff2.count();
        std::cout << "Speedup: " << speedup << "x" << std::endl;
    }

    // Verify
    double maxErr = 0.0;
    double totalErr = 0.0;
    size_t processedSamples = buffer1.size();
    for (size_t i = 0; i < processedSamples; ++i) {
        double err = std::abs(buffer1[i] - buffer2[i]);
        if (err > maxErr) maxErr = err;
        totalErr += err * err;
    }
    double rms = std::sqrt(totalErr / processedSamples);
    std::cout << "Max Error: " << maxErr << std::endl;
    std::cout << "RMS Error: " << rms << std::endl;

    if (rms < 0.05) { // Allow some error due to LUT quantization
        std::cout << "Verification PASSED" << std::endl;
        return 0;
    } else {
        std::cout << "Verification FAILED (Error too high)" << std::endl;
        return 1;
    }
}
