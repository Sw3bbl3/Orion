#pragma once

#include "EffectNode.h"
#include <string>
#include <vector>

namespace Orion {

    class ReverbNode : public EffectNode {
    public:
        ReverbNode();
        virtual ~ReverbNode() = default;

        std::string getName() const override { return "Reverb"; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 3; }

        ParameterRange getParameterRange(int index) const override {
             if (index == 0) return {0.0f, 0.98f, 0.01f};
             if (index == 1) return {0.0f, 1.0f, 0.01f};
             if (index == 2) return {0.0f, 1.0f, 0.01f};
             return {0.0f, 1.0f, 0.01f};
        }

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:
        float roomSize = 0.5f;
        float damping = 0.5f;
        float wet = 0.3f;




        class Comb {
        public:
            std::vector<float> buffer;
            size_t head = 0;
            float feedback = 0.0f;
            float damp = 0.0f;
            float filterState = 0.0f;

            void resize(size_t size);
            float process(float input);
        };

        class Allpass {
        public:
            std::vector<float> buffer;
            size_t head = 0;
            float feedback = 0.5f;

            void resize(size_t size);
            float process(float input);
        };

        struct ChannelReverb {
            std::vector<Comb> combs;
            std::vector<Allpass> allpasses;
        };

        std::vector<ChannelReverb> channels;
        void initFilters(double newSampleRate);
        bool initialized = false;
        double lastSampleRate = 0;
    };

}
