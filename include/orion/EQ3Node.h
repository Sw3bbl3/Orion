#pragma once

#include "EffectNode.h"
#include <string>
#include <vector>
#include <cmath>
#include <array>
#include <mutex>
#include <juce_dsp/juce_dsp.h>

namespace Orion {

    class EQ3Node : public EffectNode {
    public:
        EQ3Node();
        virtual ~EQ3Node() = default;

        std::string getName() const override { return "EQ3"; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 3; }

        ParameterRange getParameterRange(int index) const override {
             (void)index;
             return {0.0f, 4.0f, 0.01f};
        }

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        std::vector<float> getSpectrumData() const;


        struct FilterState {
            double x1 = 0, x2 = 0, y1 = 0, y2 = 0;
        };

    private:
        float lowGain = 1.0f;
        float midGain = 1.0f;
        float highGain = 1.0f;

        struct ChannelState {
            FilterState low;
            FilterState mid;
            FilterState high;
        };

        std::vector<ChannelState> channelStates;

        void updateFilters(double sampleRate);

        static constexpr int fftOrder = 10;
        static constexpr int fftSize = 1 << fftOrder;
        juce::dsp::FFT fft { fftOrder };
        juce::dsp::WindowingFunction<float> window { fftSize, juce::dsp::WindowingFunction<float>::hann };
        std::array<float, fftSize> fftFifo {};
        std::array<float, fftSize * 2> fftData {};
        int fftFifoIndex = 0;

        mutable std::mutex spectrumMutex;
        std::vector<float> spectrumMagnitudes;
        void pushNextSampleIntoFifo(float sample);
        void performFFT();
    };

}
