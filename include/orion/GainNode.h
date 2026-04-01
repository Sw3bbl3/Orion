#pragma once
#include "EffectNode.h"
#include <atomic>

namespace Orion {

    class GainNode : public EffectNode {
    public:
        GainNode();
        void setGain(float gain);
        float getGain() const { return gain.load(); }

        std::string getName() const override { return "Gain"; }

    protected:
        void process(AudioBlock& block) override;

    private:
        std::atomic<float> gain;
        float lastGain = -1.0f;
    };

}
