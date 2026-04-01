#pragma once

#include "AudioBlock.h"
#include <vector>
#include <cstdint>
#include <limits>
#include <string>
#include <atomic>

namespace Orion {

    class AudioNode {
    public:
        AudioNode();
        virtual ~AudioNode() = default;


        void connectInput(AudioNode* inputNode);
        void disconnectInput(AudioNode* inputNode);

        virtual std::string getName() const { return "AudioNode"; }


        virtual const AudioBlock& getOutput(uint64_t cacheKey, uint64_t projectTimeSamples);


        virtual void setFormat(size_t channels, size_t blockSize);


        virtual int getLatency() const { return 0; }


        void propagateBlockSize(size_t newBlockSize, std::vector<AudioNode*>& visited);

        void propagateSampleRate(double newSampleRate, std::vector<AudioNode*>& visited);



        virtual void processMidi(int status, int data1, int data2, int sampleOffset = 0) {
            (void)status; (void)data1; (void)data2; (void)sampleOffset;
        }

    protected:




        virtual void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
            (void)cacheKey;
            (void)frameStart;
            process(block);
        }


        virtual void process(AudioBlock& ) {}

        std::atomic<size_t> channels { 2 };
        std::atomic<size_t> blockSize { 512 };
        std::atomic<float> sampleRate { 44100.0f };

    public:
        virtual void setSampleRate(double rate) { sampleRate.store((float)rate); }
        double getSampleRate() const { return (double)sampleRate.load(); }

    protected:
        std::vector<AudioNode*> inputs;
        AudioBlock cachedOutput;
        uint64_t lastCacheKey;
    };

}
