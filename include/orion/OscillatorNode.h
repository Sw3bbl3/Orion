#pragma once
#include "AudioNode.h"
#include <atomic>
#include <vector>

namespace Orion {

    enum class Waveform {
        Sine,
        Square,
        Sawtooth,
        Triangle
    };

    class OscillatorNode : public AudioNode {
    public:
        OscillatorNode();

        void setFrequency(float freq);
        void setWaveform(Waveform type);

        void setFormat(size_t numChannels, size_t numBlockSize) override;

    protected:
        void process(AudioBlock& block) override;

    private:
        std::atomic<float> frequency;
        std::vector<double> phases;
        std::atomic<Waveform> waveform;
    };

}
