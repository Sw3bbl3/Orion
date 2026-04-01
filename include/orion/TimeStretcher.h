#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "AudioFile.h"

namespace Orion {

    class TimeStretcher {
    public:
        TimeStretcher();
        ~TimeStretcher();

        void setSource(std::shared_ptr<AudioFile> file);


        void prepare(uint32_t sampleRate, size_t maxBlockSize);


        void reset();

        void setLooping(bool shouldLoop, uint64_t length) {
            looping = shouldLoop;
            loopLength = length;
        }


        void setPosition(double position);








        void process(float* const* output, size_t numChannels, size_t numFrames, double speedRatio, double pitchRatio = 1.0);

    private:
        std::shared_ptr<AudioFile> source;
        uint32_t sampleRate = 44100;


        size_t windowSize;
        size_t overlapSize;
        size_t searchSize;

        std::vector<float> window;




        std::vector<std::vector<float>> outputBuffer;
        size_t outputBufferValidLen = 0;

        double virtualReadPos = 0.0;


        int findBestOffset(const float* const* inputData, size_t numChannels, size_t inputOffset, size_t checkLen);

        double samplesSinceLastGrain = 0.0;
        size_t grainCounter = 0;

        bool looping = false;
        uint64_t loopLength = 0;
    };

}
