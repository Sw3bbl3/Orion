#pragma once

#include "EffectNode.h"
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>

namespace Orion {

    class EffectRackNode : public EffectNode {
    public:
        EffectRackNode();
        virtual ~EffectRackNode() = default;

        std::string getName() const override { return "Effect Rack"; }


        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 8; }
        ParameterRange getParameterRange(int ) const override {
            return {0.0f, 1.0f, 0.01f};
        }


        void addEffect(std::shared_ptr<EffectNode> effect);
        void removeEffect(std::shared_ptr<EffectNode> effect);
        void moveEffect(int fromIndex, int toIndex);

        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> getEffects() const;


        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void setFormat(size_t channels, size_t blockSize) override;
        void setSampleRate(double rate) override;
        int getLatency() const override;

    private:
        struct MacroMapping {
            std::shared_ptr<EffectNode> targetEffect;
            int parameterIndex;
            float minVal;
            float maxVal;
        };





        float macros[8] = { 0.0f };

        mutable std::mutex chainMutex;
        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> effects;
    };

}
