#include "orion/EQ3Node.h"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Orion {

    EQ3Node::EQ3Node() {
        spectrumMagnitudes.assign(fftSize / 2, 0.0f);
    }

    void EQ3Node::setParameter(int index, float value) {


        if (index == 0) lowGain = value;
        else if (index == 1) midGain = value;
        else if (index == 2) highGain = value;
    }

    float EQ3Node::getParameter(int index) const {
        if (index == 0) return lowGain;
        if (index == 1) return midGain;
        if (index == 2) return highGain;
        return 0.0f;
    }

    std::string EQ3Node::getParameterName(int index) const {
        if (index == 0) return "Low";
        if (index == 1) return "Mid";
        if (index == 2) return "High";
        return "";
    }


    struct BiquadCoeffs {
        double b0, b1, b2, a0, a1, a2;
    };

    static BiquadCoeffs calcLowShelf(double fs, double f0, double gain) {
        double A = std::sqrt(gain);
        double w0 = 2 * M_PI * f0 / fs;
        double cosw0 = std::cos(w0);
        double sinw0 = std::sin(w0);
        double alpha = sinw0 / 2.0 * std::sqrt(2.0);

        double b0 =    A*( (A+1) - (A-1)*cosw0 + 2*std::sqrt(A)*alpha );
        double b1 =  2*A*( (A-1) - (A+1)*cosw0                   );
        double b2 =    A*( (A+1) - (A-1)*cosw0 - 2*std::sqrt(A)*alpha );
        double a0 =        (A+1) + (A-1)*cosw0 + 2*std::sqrt(A)*alpha;
        double a1 =   -2*( (A-1) + (A+1)*cosw0                   );
        double a2 =        (A+1) + (A-1)*cosw0 - 2*std::sqrt(A)*alpha;

        return {b0, b1, b2, a0, a1, a2};
    }

    static BiquadCoeffs calcHighShelf(double fs, double f0, double gain) {
        double A = std::sqrt(gain);
        double w0 = 2 * M_PI * f0 / fs;
        double cosw0 = std::cos(w0);
        double sinw0 = std::sin(w0);
        double alpha = sinw0 / 2.0 * std::sqrt(2.0);

        double b0 =    A*( (A+1) + (A-1)*cosw0 + 2*std::sqrt(A)*alpha );
        double b1 = -2*A*( (A-1) + (A+1)*cosw0                   );
        double b2 =    A*( (A+1) + (A-1)*cosw0 - 2*std::sqrt(A)*alpha );
        double a0 =        (A+1) - (A-1)*cosw0 + 2*std::sqrt(A)*alpha;
        double a1 =    2*( (A-1) - (A+1)*cosw0                   );
        double a2 =        (A+1) - (A-1)*cosw0 - 2*std::sqrt(A)*alpha;

        return {b0, b1, b2, a0, a1, a2};
    }

    static BiquadCoeffs calcPeaking(double fs, double f0, double gain, double Q) {
        double A = std::sqrt(gain);
        double w0 = 2 * M_PI * f0 / fs;
        double cosw0 = std::cos(w0);
        double sinw0 = std::sin(w0);
        double alpha = sinw0 / (2.0 * Q);

        double b0 =   1 + alpha*A;
        double b1 =  -2*cosw0;
        double b2 =   1 - alpha*A;
        double a0 =   1 + alpha/A;
        double a1 =  -2*cosw0;
        double a2 =   1 - alpha/A;

        return {b0, b1, b2, a0, a1, a2};
    }

    static float processBiquad(float in, EQ3Node::FilterState& s, const BiquadCoeffs& c) {



        double x = (double)in;
        double y = (c.b0 * x + c.b1 * s.x1 + c.b2 * s.x2 - c.a1 * s.y1 - c.a2 * s.y2) / c.a0;

        s.x2 = s.x1;
        s.x1 = x;
        s.y2 = s.y1;
        s.y1 = y;

        return (float)y;
    }

    void EQ3Node::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;
        if (channelStates.size() < block.getChannelCount()) {
            channelStates.resize(block.getChannelCount());
        }

        double fs = getSampleRate();
        if (fs <= 0) fs = 44100.0;


        double lowFreq = 250.0;
        double highFreq = 4000.0;
        double midFreq = 1000.0;

        auto cLow = calcLowShelf(fs, lowFreq, std::max(0.0f, lowGain));
        auto cHigh = calcHighShelf(fs, highFreq, std::max(0.0f, highGain));
        auto cMid = calcPeaking(fs, midFreq, std::max(0.0f, midGain), 1.0);

        size_t numSamples = block.getSampleCount();
        for (size_t ch = 0; ch < block.getChannelCount(); ++ch) {
            float* data = block.getChannelPointer(ch);
            auto& state = channelStates[ch];

            for (size_t i = 0; i < numSamples; ++i) {
                float s = data[i];
                s = processBiquad(s, state.low, cLow);
                s = processBiquad(s, state.mid, cMid);
                s = processBiquad(s, state.high, cHigh);
                data[i] = s;

                if (ch == 0) {
                    pushNextSampleIntoFifo(s);
                }
            }
        }
    }

    std::vector<float> EQ3Node::getSpectrumData() const
    {
        std::unique_lock<std::mutex> lock(spectrumMutex, std::try_to_lock);
        if (lock.owns_lock()) {
            return spectrumMagnitudes;
        }
        return {};
    }

    void EQ3Node::pushNextSampleIntoFifo(float sample)
    {
        if (fftFifoIndex == fftSize) {
            performFFT();
            fftFifoIndex = 0;
        }

        fftFifo[(size_t)fftFifoIndex++] = sample;
    }

    void EQ3Node::performFFT()
    {
        std::fill(fftData.begin(), fftData.end(), 0.0f);
        for (int i = 0; i < fftSize; ++i) {
            fftData[(size_t)i] = fftFifo[(size_t)i];
        }

        window.multiplyWithWindowingTable(fftData.data(), fftSize);
        fft.performFrequencyOnlyForwardTransform(fftData.data());

        std::vector<float> latest((size_t)fftSize / 2);
        for (int i = 0; i < fftSize / 2; ++i) {
            const float mag = juce::jmax(fftData[(size_t)i], 1.0e-6f);
            const float db = juce::Decibels::gainToDecibels(mag) - 36.0f;
            latest[(size_t)i] = juce::jlimit(-96.0f, 12.0f, db);
        }

        std::unique_lock<std::mutex> lock(spectrumMutex, std::try_to_lock);
        if (!lock.owns_lock()) return;

        if (spectrumMagnitudes.size() != latest.size()) {
            spectrumMagnitudes = std::move(latest);
            return;
        }

        for (size_t i = 0; i < latest.size(); ++i) {
            const float prev = spectrumMagnitudes[i];
            const float next = latest[i];
            const float attack = 0.52f;
            const float release = 0.12f;
            spectrumMagnitudes[i] = (next > prev) ? (prev + (next - prev) * attack)
                                                  : (prev + (next - prev) * release);
        }
    }

}
