#include "orion/AudioNode.h"
#include <algorithm>

namespace Orion {

    AudioNode::AudioNode()
        : lastCacheKey(std::numeric_limits<uint64_t>::max())
    {
        channels.store(AudioBlock::DefaultChannels);
        blockSize.store(AudioBlock::DefaultBlockSize);
        sampleRate.store(44100.0f);
        cachedOutput.resize(channels.load(), blockSize.load());
    }

    void AudioNode::connectInput(AudioNode* inputNode) {
        if (std::find(inputs.begin(), inputs.end(), inputNode) == inputs.end()) {
            inputs.push_back(inputNode);
        }
    }

    void AudioNode::disconnectInput(AudioNode* inputNode) {
        inputs.erase(std::remove(inputs.begin(), inputs.end(), inputNode), inputs.end());
    }

    void AudioNode::setFormat(size_t ch, size_t bs) {
        channels.store(ch);
        blockSize.store(bs);
        cachedOutput.resize(ch, bs);
    }

    void AudioNode::propagateBlockSize(size_t newBlockSize, std::vector<AudioNode*>& visited) {
        if (std::find(visited.begin(), visited.end(), this) != visited.end()) return;
        visited.push_back(this);


        setFormat(channels, newBlockSize);


        for (auto* input : inputs) {
            input->propagateBlockSize(newBlockSize, visited);
        }
    }

    void AudioNode::propagateSampleRate(double newSampleRate, std::vector<AudioNode*>& visited) {
        if (std::find(visited.begin(), visited.end(), this) != visited.end()) return;
        visited.push_back(this);

        setSampleRate(newSampleRate);
        setFormat(channels.load(), blockSize.load());

        for (auto* input : inputs) {
            input->propagateSampleRate(newSampleRate, visited);
        }
    }

    const AudioBlock& AudioNode::getOutput(uint64_t cacheKey, uint64_t projectTimeSamples) {
        if (cacheKey != lastCacheKey) {



            cachedOutput.zero();


            for (auto* input : inputs) {
                const AudioBlock& inputBlock = input->getOutput(cacheKey, projectTimeSamples);
                cachedOutput.add(inputBlock);
            }


            process(cachedOutput, cacheKey, projectTimeSamples);

            lastCacheKey = cacheKey;
        }
        return cachedOutput;
    }

}
