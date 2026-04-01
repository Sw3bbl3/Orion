#pragma once

#include "EffectNode.h"
#include <vector>

namespace Orion {

    class LimiterNode : public EffectNode {
    public:
        LimiterNode();
        ~LimiterNode() override = default;

        std::string getName() const override { return "Limiter"; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 3; }

        ParameterRange getParameterRange(int index) const override {
            if (index == 0) return { -24.0f, 0.0f, 0.1f };  // Threshold dB
            if (index == 1) return { -3.0f, 0.0f, 0.1f };   // Ceiling dB
            if (index == 2) return { 0.01f, 0.5f, 0.01f };  // Release seconds
            return { 0.0f, 1.0f, 0.01f };
        }

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:
        float thresholdDb = -3.0f;
        float ceilingDb = -0.2f;
        float releaseSec = 0.08f;

        float gainSmoothed = 1.0f;
        std::vector<float> peakHistory;
        size_t peakIndex = 0;
    };

}
