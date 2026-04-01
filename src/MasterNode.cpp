#include "orion/MasterNode.h"
#include <algorithm>
#include <cmath>

namespace Orion {

    MasterNode::MasterNode() : volume(1.0f), levelL(0.0f), levelR(0.0f) {
        auto empty = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>();
        std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(empty));
    }

    void MasterNode::addEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(mutex);
        if (effect) {
            effect->setSampleRate(getSampleRate());
            effect->setFormat(channels, blockSize);

            auto current = std::atomic_load(&effects);
            auto next = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*current);
            next->push_back(effect);
            std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(next));
        }
    }

    void MasterNode::removeEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(mutex);

        auto current = std::atomic_load(&effects);
        auto next = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*current);

        auto it = std::remove(next->begin(), next->end(), effect);
        if (it != next->end()) {
            next->erase(it, next->end());
            std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(next));
        }
    }

    void MasterNode::clearEffects() {
        std::lock_guard<std::mutex> lock(mutex);
        auto empty = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>();
        std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(empty));
    }

    std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> MasterNode::getEffects() const {
        return std::atomic_load(&effects);
    }

    void MasterNode::setVolume(float v) {
        volume = v;
    }

    void MasterNode::setFormat(size_t ch, size_t bs) {
        AudioNode::setFormat(ch, bs);





        auto current = std::atomic_load(&effects);
        for(const auto& eff : *current) {
            eff->setFormat(ch, bs);
        }
    }

    const AudioBlock& MasterNode::getOutput(uint64_t cacheKey, uint64_t projectTimeSamples) {
        if (cacheKey != lastCacheKey) {
            cachedOutput.zero();

            for (auto* input : inputs) {
                const AudioBlock& block = input->getOutput(cacheKey, projectTimeSamples);
                cachedOutput.add(block);
            }

            process(cachedOutput, cacheKey, projectTimeSamples);
            lastCacheKey = cacheKey;
        }
        return cachedOutput;
    }

    void MasterNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {

        auto localEffects = std::atomic_load(&effects);


        for (const auto& effect : *localEffects) {
            if (effect->getSampleRate() != getSampleRate()) {
                effect->setSampleRate(getSampleRate());
            }
            effect->processAudio(block, cacheKey, frameStart);
        }


        float vol = volume.load();
        if (std::abs(vol - 1.0f) > 0.0001f) {
            block.multiply(vol);
        }


        float maxL = 0.0f;
        float maxR = 0.0f;
        size_t samples = block.getSampleCount();
        size_t chans = block.getChannelCount();

        if (samples > 0) {
            float* lPtr = (chans > 0) ? block.getChannelPointer(0) : nullptr;
            float* rPtr = (chans > 1) ? block.getChannelPointer(1) : lPtr;

            if (lPtr) {
                for (size_t i=0; i<samples; ++i) {
                    float absL = std::abs(lPtr[i]);
                    if (absL > maxL) maxL = absL;
                }
            }
            if (rPtr && chans > 1) {
                for (size_t i=0; i<samples; ++i) {
                    float absR = std::abs(rPtr[i]);
                    if (absR > maxR) maxR = absR;
                }
            } else if (chans == 1) {
                maxR = maxL;
            }
        }


        float currentL = levelL.load();
        float currentR = levelR.load();

        if (maxL > currentL) levelL = maxL;
        else levelL = currentL * 0.90f;

        if (maxR > currentR) levelR = maxR;
        else levelR = currentR * 0.90f;
    }

}
