#include "orion/CompressorNode.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace Orion {

    namespace {
        thread_local std::unordered_set<const CompressorNode*> gActiveCompressors;
    }

    CompressorNode::CompressorNode() {
        threshold = 0.5f;
        ratio = 2.0f;
        attack = 0.02f;
        release = 0.2f;
    }

    void CompressorNode::setParameter(int index, float value) {
        if (index == 0) threshold = std::clamp(value, 0.0f, 1.0f);
        if (index == 1) ratio = std::clamp(value, 1.0f, 20.0f);
        if (index == 2) attack = std::clamp(value, 0.001f, 0.5f);
        if (index == 3) release = std::clamp(value, 0.01f, 2.0f);
    }

    float CompressorNode::getParameter(int index) const {
        if (index == 0) return threshold;
        if (index == 1) return ratio;
        if (index == 2) return attack;
        if (index == 3) return release;
        return 0.0f;
    }

    std::string CompressorNode::getParameterName(int index) const {
        if (index == 0) return "Threshold";
        if (index == 1) return "Ratio";
        if (index == 2) return "Attack";
        if (index == 3) return "Release";
        return "";
    }

    void CompressorNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        size_t nChannels = block.getChannelCount();
        if (states.size() < nChannels) states.resize(nChannels);

        double fs = (double)sampleRate.load();
        if (fs <= 0) fs = 44100.0;

        const bool firstEntry = gActiveCompressors.insert(this).second;
        struct ScopeExit {
            bool enabled = false;
            const CompressorNode* node = nullptr;
            ~ScopeExit() {
                if (enabled && node) gActiveCompressors.erase(node);
            }
        } scopeExit { firstEntry, this };


        float alphaA = (float)std::exp(-1.0 / (attack * fs));
        float alphaR = (float)std::exp(-1.0 / (release * fs));

        size_t nSamples = block.getSampleCount();


        const AudioBlock* scBlock = nullptr;
        AudioNode* scSource = sidechainSource.load();
        if (firstEntry && scSource && scSource != this) {

            scBlock = &scSource->getOutput(cacheKey, frameStart);
        }

        for (size_t c = 0; c < nChannels; ++c) {
            float* data = block.getChannelPointer(c);
            auto& state = states[c];

            const float* scData = nullptr;
            if (scBlock) {

                if (c < scBlock->getChannelCount()) {
                    scData = scBlock->getChannelPointer(c);
                } else if (scBlock->getChannelCount() > 0) {
                    scData = scBlock->getChannelPointer(0);
                }
            }

            for (size_t i = 0; i < nSamples; ++i) {
                float in = data[i];

                float detectorIn = in;
                if (scData) {
                    detectorIn = scData[i];
                }

                float absIn = std::abs(detectorIn);


                if (absIn > state.env) {
                    state.env = alphaA * state.env + (1.0f - alphaA) * absIn;
                } else {
                    state.env = alphaR * state.env + (1.0f - alphaR) * absIn;
                }


                float gain = 1.0f;
                if (state.env > threshold) {










                    if (threshold > 0.0001f) {
                        float envOverThresh = state.env / threshold;




                        gain = std::pow(envOverThresh, (1.0f / ratio) - 1.0f);
                    } else {
                        gain = 0.0f;
                    }
                }

                data[i] = in * gain;



            }
        }
    }
}
