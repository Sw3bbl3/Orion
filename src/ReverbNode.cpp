#include "orion/ReverbNode.h"
#include <algorithm>

namespace Orion {

    void ReverbNode::Comb::resize(size_t size) {
        if (size == 0) size = 1;
        buffer.assign(size, 0.0f);
        head = 0;
        filterState = 0.0f;
    }

    float ReverbNode::Comb::process(float input) {
        float output = buffer[head];

        filterState = output * (1.0f - damp) + filterState * damp;

        buffer[head] = input + filterState * feedback;
        head++;
        if (head >= buffer.size()) head = 0;

        return output;
    }

    void ReverbNode::Allpass::resize(size_t size) {
        if (size == 0) size = 1;
        buffer.assign(size, 0.0f);
        head = 0;
    }

    float ReverbNode::Allpass::process(float input) {
        float bufOut = buffer[head];
        float output = -input + bufOut;
        buffer[head] = input + (bufOut * feedback);

        head++;
        if (head >= buffer.size()) head = 0;

        return output;
    }

    ReverbNode::ReverbNode() {

        roomSize = 0.7f;
        damping = 0.2f;
        wet = 0.4f;
    }

    void ReverbNode::setParameter(int index, float value) {
        if (index == 0) roomSize = std::clamp(value, 0.0f, 0.98f);
        if (index == 1) damping = std::clamp(value, 0.0f, 1.0f);
        if (index == 2) wet = std::clamp(value, 0.0f, 1.0f);
    }

    float ReverbNode::getParameter(int index) const {
        if (index == 0) return roomSize;
        if (index == 1) return damping;
        if (index == 2) return wet;
        return 0.0f;
    }

    std::string ReverbNode::getParameterName(int index) const {
        if (index == 0) return "Room Size";
        if (index == 1) return "Damping";
        if (index == 2) return "Mix (Wet)";
        return "";
    }

    void ReverbNode::initFilters(double newSampleRate) {
        channels.resize(2);





        int combLens[] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
        int apLens[] = {556, 441, 341, 225};

        double scale = newSampleRate / 44100.0;

        for (auto& ch : channels) {
            ch.combs.resize(8);
            for (int i=0; i<8; ++i) {
                ch.combs[i].resize((size_t)(combLens[i] * scale));
            }

            ch.allpasses.resize(4);
            for (int i=0; i<4; ++i) {
                ch.allpasses[i].resize((size_t)(apLens[i] * scale));
            }
        }
        initialized = true;
        lastSampleRate = newSampleRate;
    }

    void ReverbNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;
        double sr = getSampleRate();
        if (!initialized || sr != lastSampleRate) {
            initFilters(sr);
        }



        float fb = roomSize;
        float dp = damping;

        for (auto& ch : channels) {
            for (auto& c : ch.combs) {
                c.feedback = fb;
                c.damp = dp;
            }
        }

        size_t nSamples = block.getSampleCount();
        size_t nChannels = block.getChannelCount();
        if (nChannels > channels.size()) nChannels = channels.size();

        for (size_t c = 0; c < nChannels; ++c) {
            float* data = block.getChannelPointer(c);
            auto& reverbCh = channels[c];





            for (size_t i = 0; i < nSamples; ++i) {
                float in = data[i];
                float out = 0.0f;


                for (auto& comb : reverbCh.combs) {
                    out += comb.process(in);
                }


                for (auto& ap : reverbCh.allpasses) {
                    out = ap.process(out);
                }


                data[i] = (in * (1.0f - wet)) + (out * wet * 0.1f);
            }
        }
    }

}
