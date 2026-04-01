#include "orion/AudioTrack.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>

int main() {
    using namespace Orion;

    // Parameters
    const size_t SAMPLE_RATE = 44100;
    const size_t NUM_SECONDS = 300;
    const size_t BLOCK_SIZE = 512;
    const size_t TOTAL_FRAMES = SAMPLE_RATE * NUM_SECONDS;
    const size_t NUM_CHANNELS = 2;

    std::cout << "Initializing AudioTrack..." << std::endl;
    AudioTrack track;
    track.setArmed(true);

    std::vector<float> inputBlock(BLOCK_SIZE * NUM_CHANNELS);
    // Fill with pattern
    for(size_t i=0; i<inputBlock.size(); ++i) inputBlock[i] = (float)(i % 100) / 100.0f;

    std::cout << "Benchmarking AudioTrack::appendRecordingData ("
              << NUM_SECONDS << " seconds)..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    size_t framesProcessed = 0;
    while(framesProcessed < TOTAL_FRAMES) {
        size_t frames = std::min(BLOCK_SIZE, TOTAL_FRAMES - framesProcessed);
        track.appendRecordingData(inputBlock.data(), frames, NUM_CHANNELS);
        framesProcessed += frames;
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Time: " << diff.count() << " s" << std::endl;

    // Verify correctness
    std::cout << "Verifying data..." << std::endl;
    std::vector<float> recorded = track.takeRecordingBuffer();
    std::cout << "Recorded Samples: " << recorded.size() << std::endl;

    if (recorded.size() != TOTAL_FRAMES * NUM_CHANNELS) {
        std::cerr << "ERROR: Size mismatch! Expected " << TOTAL_FRAMES * NUM_CHANNELS << ", got " << recorded.size() << std::endl;
        return 1;
    }

    // Verify data pattern
    bool error = false;
    for(size_t i=0; i<recorded.size(); ++i) {
        size_t patternIdx = i % (BLOCK_SIZE * NUM_CHANNELS);
        float expected = inputBlock[patternIdx];

        if (std::abs(recorded[i] - expected) > 0.0001f) {
             std::cerr << "ERROR: Mismatch at index " << i << ". Exp: " << expected << ", Got: " << recorded[i] << std::endl;
             error = true;
             if (i > 100) break;
        }
    }

    if (!error) {
        std::cout << "Verification SUCCESS" << std::endl;
        return 0;
    } else {
        std::cout << "Verification FAILED" << std::endl;
        return 1;
    }
}
