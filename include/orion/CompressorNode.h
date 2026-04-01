#pragma once

#include "EffectNode.h"
#include <string>
#include <atomic>

namespace Orion {

    class CompressorNode : public EffectNode {
    public:
        CompressorNode();
        virtual ~CompressorNode() = default;

        std::string getName() const override { return "Compressor"; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 4; }

        ParameterRange getParameterRange(int index) const override {
             if (index == 0) return {0.0f, 1.0f, 0.01f};
             if (index == 1) return {1.0f, 20.0f, 0.1f};
             if (index == 2) return {0.001f, 0.5f, 0.001f};
             if (index == 3) return {0.01f, 2.0f, 0.01f};
             return {0.0f, 1.0f, 0.01f};
        }

        void setSidechainSource(AudioNode* source) { sidechainSource = source; }
        AudioNode* getSidechainSource() const { return sidechainSource; }

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:
        std::atomic<AudioNode*> sidechainSource { nullptr };

        float threshold = 0.5f;
        float ratio = 4.0f;
        float attack = 0.01f;
        float release = 0.1f;

        struct ChannelState {
            float env = 0.0f;
        };
        std::vector<ChannelState> states;
    };

}
