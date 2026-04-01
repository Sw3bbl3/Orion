#include "orion/TimeStretcher.h"
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iostream>

namespace Orion {

    TimeStretcher::TimeStretcher()
        : windowSize(2048), overlapSize(512), searchSize(256), outputBufferValidLen(0)
    {
    }

    TimeStretcher::~TimeStretcher() {
    }

    void TimeStretcher::setSource(std::shared_ptr<AudioFile> file) {
        source = file;
        reset();
    }

    void TimeStretcher::prepare(uint32_t rate, size_t maxBlockSize) {
        sampleRate = rate;



        windowSize = (size_t)(0.05 * sampleRate);
        if (windowSize % 2 != 0) windowSize++;


        overlapSize = windowSize / 2;
        searchSize = windowSize / 4;


        window.resize(windowSize);
        for (size_t i = 0; i < windowSize; ++i) {
            window[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * i / (windowSize - 1)));
        }


        size_t bufferSize = maxBlockSize + windowSize * 4;
        outputBuffer.resize(2);
        for (auto& ch : outputBuffer) {
            ch.resize(bufferSize, 0.0f);
        }

        reset();
    }

    void TimeStretcher::reset() {
        outputBufferValidLen = overlapSize;
        virtualReadPos = 0.0;
        grainCounter = 0;
        samplesSinceLastGrain = 0.0;

        for (auto& ch : outputBuffer) {
            std::fill(ch.begin(), ch.end(), 0.0f);
        }
    }

    void TimeStretcher::setPosition(double pos) {
        virtualReadPos = pos;

        outputBufferValidLen = overlapSize;
        for (auto& ch : outputBuffer) {
            std::fill(ch.begin(), ch.end(), 0.0f);
        }
    }

    int TimeStretcher::findBestOffset(const float* const* inputData, size_t numChannels, size_t inputOffset, size_t checkLen) {
        (void)inputData; (void)numChannels; (void)inputOffset; (void)checkLen;
        return 0;
    }

    void TimeStretcher::process(float* const* output, size_t numChannels, size_t numFrames, double speedRatio, double pitchRatio) {
        if (!source || numChannels == 0 || source->getLengthFrames() == 0) {
            for (size_t c = 0; c < numChannels; ++c) {
                std::fill(output[c], output[c] + numFrames, 0.0f);
            }
            return;
        }

        size_t Hs = windowSize / 2;
        size_t maxChannels = std::min((size_t)source->getChannels(), outputBuffer.size());
        size_t outChans = std::min(numChannels, outputBuffer.size());

        while (outputBufferValidLen < numFrames + overlapSize) {
             double Ha = (double)Hs * speedRatio;

             double grainReadPos = virtualReadPos;

             if (outputBufferValidLen < overlapSize) {
                 outputBufferValidLen = overlapSize;
             }
             size_t writePos = outputBufferValidLen - overlapSize;

             for (size_t c = 0; c < maxChannels; ++c) {
                 for (size_t i = 0; i < windowSize; ++i) {
                     // Resample grain by pitchRatio
                     double sourceIdx = grainReadPos + (double)i * pitchRatio;

                     float val = 0.0f;
                     if (looping && loopLength > 0) {
                         sourceIdx = std::fmod(sourceIdx, (double)loopLength);
                         if (sourceIdx < 0) sourceIdx += (double)loopLength;
                     }

                     uint64_t i0 = (uint64_t)sourceIdx;
                     uint64_t i1 = i0 + 1;
                     float frac = (float)(sourceIdx - i0);

                     uint64_t sourceLen = source->getLengthFrames();
                     if (i0 >= sourceLen) i0 = sourceLen - 1;
                     if (i1 >= sourceLen) {
                         if (looping && loopLength > 0) i1 = 0; else i1 = sourceLen - 1;
                     }

                     float s0 = source->getSample((uint32_t)c, i0);
                     float s1 = source->getSample((uint32_t)c, i1);
                     val = s0 + frac * (s1 - s0);

                     float w = window[i];

                     if (writePos + i < outputBuffer[c].size()) {
                         outputBuffer[c][writePos + i] += val * w;
                     }
                 }
             }

             virtualReadPos += Ha;
             if (looping && loopLength > 0) {
                 virtualReadPos = std::fmod(virtualReadPos, (double)loopLength);
             }

             outputBufferValidLen += Hs;
             grainCounter++;
        }

        for (size_t c = 0; c < outChans; ++c) {
             std::copy(outputBuffer[c].begin(), outputBuffer[c].begin() + numFrames, output[c]);

             size_t remaining = outputBufferValidLen - numFrames;
             if (outputBuffer[c].size() > numFrames) {
                 std::memmove(outputBuffer[c].data(), outputBuffer[c].data() + numFrames, remaining * sizeof(float));

                 size_t clearStart = remaining;
                 size_t clearLen = outputBuffer[c].size() - remaining;
                 std::memset(outputBuffer[c].data() + clearStart, 0, clearLen * sizeof(float));
             }
        }
        outputBufferValidLen -= numFrames;

        for (size_t c = outChans; c < numChannels; ++c) {
            std::fill(output[c], output[c] + numFrames, 0.0f);
        }
    }

}
