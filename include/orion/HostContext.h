#pragma once
#include <cstdint>

namespace Orion {
    struct HostContext {
        double bpm = 120.0;
        double sampleRate = 44100.0;
        uint64_t projectTimeSamples = 0;
        bool playing = false;
        double timeSigNumerator = 4.0;
        double timeSigDenominator = 4.0;


        bool loopActive = false;
        uint64_t loopStart = 0;
        uint64_t loopEnd = 0;
    };

    extern thread_local HostContext gHostContext;
}
