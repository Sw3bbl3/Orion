#pragma once

#include "EffectNode.h"
#include <vector>

namespace Orion {

    class DelayNode : public EffectNode {
    public:
        DelayNode();
        ~DelayNode() = default;

        std::string getName() const override { return "Delay"; }

        void setDelayTime(float seconds);
        void setFeedback(float feedback);
        void setMix(float mix);


        int getParameterCount() const override { return 3; }
        std::string getParameterName(int index) const override;
        void setParameter(int index, float value) override;
        float getParameter(int index) const override;

        ParameterRange getParameterRange(int index) const override {
             if (index == 0) return {0.0f, 2.0f, 0.01f};
             if (index == 1) return {0.0f, 0.95f, 0.01f};
             if (index == 2) return {0.0f, 1.0f, 0.01f};
             return {0.0f, 1.0f, 0.01f};
        }

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:
        float delayTime;
        float feedback;
        float mix;

        std::vector<float> buffer;
        size_t writeHead;
        double lastSampleRate = 0.0;
    };

}
