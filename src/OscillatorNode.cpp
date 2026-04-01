#include "orion/OscillatorNode.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    template<typename F>
    inline void runOscillatorLoop(Orion::AudioBlock& block,
                                  float baseFreq,
                                  double sampleRate,
                                  size_t safeChannelCount,
                                  std::vector<double>& phases,
                                  F&& computeSample)
    {
        size_t sampleCount = block.getSampleCount();
        for (size_t ch = 0; ch < safeChannelCount; ++ch) {
            float* channelData = block.getChannelPointer(ch);
            double& phase = phases[ch];

            for (size_t i = 0; i < sampleCount; ++i) {


                float modulation = channelData[i];
                float instantaneousFreq = baseFreq + modulation;

                double phaseIncrement = 2.0 * M_PI * instantaneousFreq / sampleRate;


                channelData[i] = computeSample(phase);

                phase += phaseIncrement;


                while (phase >= 2.0 * M_PI) {
                    phase -= 2.0 * M_PI;
                }
                while (phase < 0.0) {
                    phase += 2.0 * M_PI;
                }
            }
        }
    }
}

namespace Orion {

    OscillatorNode::OscillatorNode()
        : frequency(440.0f), waveform(Waveform::Sine)
    {
        phases.resize(AudioBlock::DefaultChannels, 0.0);
    }

    void OscillatorNode::setFrequency(float freq) {
        frequency = freq;
    }

    void OscillatorNode::setWaveform(Waveform type) {
        waveform = type;
    }

    void OscillatorNode::setFormat(size_t numChannels, size_t numBlockSize) {
        AudioNode::setFormat(numChannels, numBlockSize);


        if (phases.size() != numChannels) {
            phases.resize(numChannels, 0.0);
        }
    }

    void OscillatorNode::process(AudioBlock& block) {
        float baseFreq = frequency.load();
        Waveform currentWave = waveform.load();

        size_t channelCount = block.getChannelCount();

        size_t safeChannelCount = std::min(channelCount, phases.size());
        double sr = (double)sampleRate.load();

        switch (currentWave) {
            case Waveform::Sine:
                runOscillatorLoop(block, baseFreq, sr, safeChannelCount, phases, [](double p) {
                    return static_cast<float>(std::sin(p));
                });
                break;
            case Waveform::Square:
                runOscillatorLoop(block, baseFreq, sr, safeChannelCount, phases, [](double p) {
                    return (p < M_PI) ? 1.0f : -1.0f;
                });
                break;
            case Waveform::Sawtooth:
                runOscillatorLoop(block, baseFreq, sr, safeChannelCount, phases, [](double p) {
                    return static_cast<float>((p / M_PI) - 1.0);
                });
                break;
            case Waveform::Triangle:
                runOscillatorLoop(block, baseFreq, sr, safeChannelCount, phases, [](double p) {
                    double normalizedP = p / (2.0 * M_PI);
                    if (normalizedP < 0.25) return static_cast<float>(4.0 * normalizedP);
                    else if (normalizedP < 0.75) return static_cast<float>(2.0 - 4.0 * normalizedP);
                    else return static_cast<float>(4.0 * normalizedP - 4.0);
                });
                break;
        }
    }

}
