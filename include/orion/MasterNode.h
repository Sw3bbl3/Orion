#pragma once

#include "AudioNode.h"
#include "EffectNode.h"
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>

namespace Orion {

    class MasterNode : public AudioNode {
    public:
        MasterNode();
        ~MasterNode() override = default;


        void addEffect(std::shared_ptr<EffectNode> effect);
        void removeEffect(std::shared_ptr<EffectNode> effect);
        void clearEffects();


        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> getEffects() const;


        void setVolume(float v);
        float getVolume() const { return volume.load(); }


        float getLevelL() const { return levelL.load(); }
        float getLevelR() const { return levelR.load(); }


        void setFormat(size_t channels, size_t blockSize) override;
        const AudioBlock& getOutput(uint64_t cacheKey, uint64_t projectTimeSamples) override;

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:
        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> effects;
        mutable std::mutex mutex;

        std::atomic<float> volume;
        std::atomic<float> levelL;
        std::atomic<float> levelR;
    };
}
