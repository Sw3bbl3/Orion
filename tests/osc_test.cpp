#include "orion/OscillatorNode.h"
#include "orion/AudioBlock.h"
#include <iostream>
#include <cassert>
#include <cmath>

using namespace Orion;

int main() {
    auto test_wf = [&](Waveform wf, const char* name) {
        OscillatorNode osc;
        const int channels = 1;
        const int samples = 128;
        osc.setFormat(channels, samples);
        osc.setSampleRate(44100.0f);
        osc.setFrequency(440.0f);
        osc.setWaveform(wf);

        const AudioBlock& output = osc.getOutput(0, 0);

        float firstSample = output.getChannelPointer(0)[0];
        std::cout << name << " first sample: " << firstSample << std::endl;

        // Basic checks
        if (wf == Waveform::Sine) {
            // sin(0) = 0
            if (std::abs(firstSample) > 1e-5) {
                std::cerr << "Sine failed: " << firstSample << std::endl;
                return false;
            }
        } else if (wf == Waveform::Square) {
            // (0 < PI) ? 1 : -1 => 1
            if (std::abs(firstSample - 1.0f) > 1e-5) {
                std::cerr << "Square failed: " << firstSample << std::endl;
                return false;
            }
        } else if (wf == Waveform::Sawtooth) {
            // (0 / PI) - 1 => -1
            if (std::abs(firstSample + 1.0f) > 1e-5) {
                std::cerr << "Sawtooth failed: " << firstSample << std::endl;
                return false;
            }
        } else if (wf == Waveform::Triangle) {
            // (0 / 2PI) => 0 < 0.25 => 4*0 => 0
            if (std::abs(firstSample) > 1e-5) {
                std::cerr << "Triangle failed: " << firstSample << std::endl;
                return false;
            }
        }
        return true;
    };

    bool success = true;
    success &= test_wf(Waveform::Sine, "Sine");
    success &= test_wf(Waveform::Square, "Square");
    success &= test_wf(Waveform::Sawtooth, "Sawtooth");
    success &= test_wf(Waveform::Triangle, "Triangle");

    if (success) {
        std::cout << "Oscillator functional test passed!" << std::endl;
        return 0;
    } else {
        std::cout << "Oscillator functional test FAILED!" << std::endl;
        return 1;
    }
}
