#include "orion/DelayNode.h"
#include <algorithm>
#include <cmath>

namespace Orion {

    DelayNode::DelayNode()
        : delayTime(0.5f), feedback(0.5f), mix(0.5f), writeHead(0), lastSampleRate(44100.0)
    {

        buffer.resize((size_t)(44100 * 2.0) * 2, 0.0f);
    }

    void DelayNode::setDelayTime(float seconds) {
        delayTime = std::max(0.0f, std::min(seconds, 2.0f));
    }

    void DelayNode::setFeedback(float fb) {
        feedback = std::max(0.0f, std::min(fb, 0.95f));
    }

    void DelayNode::setMix(float m) {
        mix = std::max(0.0f, std::min(m, 1.0f));
    }

    std::string DelayNode::getParameterName(int index) const {
        switch(index) {
            case 0: return "Time";
            case 1: return "Feedback";
            case 2: return "Mix";
            default: return "";
        }
    }

    void DelayNode::setParameter(int index, float value) {
        switch(index) {
            case 0: setDelayTime(value); break;
            case 1: setFeedback(value); break;
            case 2: setMix(value); break;
        }
    }

    float DelayNode::getParameter(int index) const {
        switch(index) {
            case 0: return delayTime;
            case 1: return feedback;
            case 2: return mix;
            default: return 0.0f;
        }
    }

    void DelayNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;

        double currentSR = (double)sampleRate.load();

        if (std::abs(currentSR - lastSampleRate) > 0.1) {
            lastSampleRate = currentSR;
            buffer.resize((size_t)(currentSR * 2.0) * 2, 0.0f);
            std::fill(buffer.begin(), buffer.end(), 0.0f);
            writeHead = 0;
        }

        size_t delaySamples = (size_t)(delayTime * currentSR);
        if (delaySamples == 0) return;

        size_t bufferSize = buffer.size() / 2;

        float* lChannel = (block.getChannelCount() > 0) ? block.getChannelPointer(0) : nullptr;
        float* rChannel = (block.getChannelCount() > 1) ? block.getChannelPointer(1) : lChannel;

        if (!lChannel) return;

        for (size_t i = 0; i < block.getSampleCount(); ++i) {

            {
                float dry = lChannel[i];
                long readHead = (long)writeHead - (long)delaySamples;
                if (readHead < 0) readHead += (long)bufferSize;

                float wet = buffer[readHead * 2 + 0];
                buffer[writeHead * 2 + 0] = dry + wet * feedback;
                lChannel[i] = dry * (1.0f - mix) + wet * mix;
            }


            if (rChannel) {
                float dry = rChannel[i];
                long readHead = (long)writeHead - (long)delaySamples;
                if (readHead < 0) readHead += (long)bufferSize;

                float wet = buffer[readHead * 2 + 1];
                buffer[writeHead * 2 + 1] = dry + wet * feedback;
                rChannel[i] = dry * (1.0f - mix) + wet * mix;
            }

            writeHead++;
            if (writeHead >= bufferSize) writeHead = 0;
        }
    }

}
