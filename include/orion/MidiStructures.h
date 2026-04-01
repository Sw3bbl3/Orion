#pragma once

#include <cstdint>

namespace Orion {

    struct MidiNote {
        uint64_t startFrame;
        uint64_t lengthFrames;
        int noteNumber;
        float velocity;

        bool operator==(const MidiNote& other) const {
            return startFrame == other.startFrame &&
                   lengthFrames == other.lengthFrames &&
                   noteNumber == other.noteNumber;
        }
    };

}
