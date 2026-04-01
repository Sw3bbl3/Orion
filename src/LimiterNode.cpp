#include "orion/LimiterNode.h"
#include <algorithm>
#include <cmath>

namespace Orion {

    LimiterNode::LimiterNode()
    {
        thresholdDb = -3.0f;
        ceilingDb = -0.2f;
        releaseSec = 0.08f;
    }

    void LimiterNode::setParameter(int index, float value)
    {
        if (index == 0) thresholdDb = std::clamp(value, -24.0f, 0.0f);
        else if (index == 1) ceilingDb = std::clamp(value, -3.0f, 0.0f);
        else if (index == 2) releaseSec = std::clamp(value, 0.01f, 0.5f);
    }

    float LimiterNode::getParameter(int index) const
    {
        if (index == 0) return thresholdDb;
        if (index == 1) return ceilingDb;
        if (index == 2) return releaseSec;
        return 0.0f;
    }

    std::string LimiterNode::getParameterName(int index) const
    {
        if (index == 0) return "Threshold (dB)";
        if (index == 1) return "Ceiling (dB)";
        if (index == 2) return "Release";
        return "";
    }

    void LimiterNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart)
    {
        juce::ignoreUnused(cacheKey, frameStart);

        const size_t numChannels = block.getChannelCount();
        const size_t samples = block.getSampleCount();
        if (numChannels == 0 || samples == 0) return;

        const double sr = std::max(1.0, (double)sampleRate.load());
        const size_t lookaheadSamples = (size_t)std::clamp((int)(0.0015 * sr), 8, 256); // ~1.5ms
        if (peakHistory.size() != lookaheadSamples) {
            peakHistory.assign(lookaheadSamples, 0.0f);
            peakIndex = 0;
        }

        const float thresholdGain = juce::Decibels::decibelsToGain(thresholdDb);
        const float ceilingGain = juce::Decibels::decibelsToGain(ceilingDb);
        const float releaseCoeff = std::exp(-1.0f / (releaseSec * (float)sr));

        for (size_t i = 0; i < samples; ++i) {
            float currentPeak = 0.0f;
            for (size_t c = 0; c < numChannels; ++c) {
                currentPeak = std::max(currentPeak, std::abs(block.getChannelPointer(c)[i]));
            }

            peakHistory[peakIndex] = currentPeak;
            peakIndex = (peakIndex + 1) % peakHistory.size();

            float lookaheadPeak = 0.0f;
            for (const float p : peakHistory) lookaheadPeak = std::max(lookaheadPeak, p);

            float targetGain = 1.0f;
            if (lookaheadPeak > thresholdGain && lookaheadPeak > 0.0f) {
                const float limited = thresholdGain / lookaheadPeak;
                targetGain = std::min(1.0f, limited) * (ceilingGain / thresholdGain);
            }

            if (targetGain < gainSmoothed) gainSmoothed = targetGain; // fast attack
            else gainSmoothed = (gainSmoothed * releaseCoeff) + (targetGain * (1.0f - releaseCoeff));

            for (size_t c = 0; c < numChannels; ++c) {
                auto* data = block.getChannelPointer(c);
                float out = data[i] * gainSmoothed;

                // Final safety ceiling.
                if (out > ceilingGain) out = ceilingGain;
                else if (out < -ceilingGain) out = -ceilingGain;
                data[i] = out;
            }
        }
    }

}
