#include "orion/GainNode.h"
#include <cmath>
#include <algorithm>

namespace Orion {

    GainNode::GainNode() : gain(1.0f) {}

    void GainNode::setGain(float g) {
        gain = g;
    }

    void GainNode::process(AudioBlock& block) {
        float targetGain = gain.load();

        if (lastGain < 0.0f) {
            lastGain = targetGain;
        }

        if (std::abs(targetGain - lastGain) < 1e-6f) {
            block.multiply(targetGain);
        } else {
            size_t numSamples = block.getSampleCount();
            size_t numChannels = block.getChannelCount();
            float gainStep = (targetGain - lastGain) / (float)numSamples;

            for (size_t i = 0; i < numSamples; ++i) {
                float currentG = lastGain + gainStep * (float)(i + 1);
                for (size_t c = 0; c < numChannels; ++c) {
                    block.getChannelPointer(c)[i] *= currentG;
                }
            }
            lastGain = targetGain;
        }
    }

}
