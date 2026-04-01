#pragma once
#include <vector>
#include <string>
#include <functional>
#include <memory>
#include "orion/AudioClip.h"

namespace Orion {

    enum class GlitchType {
        Stutter,
        Reverse,
        Mute,
        TapeStop,
        Retrigger
    };

    struct GlitchSettings {
        GlitchType type = GlitchType::Stutter;
        int gridSize = 16; // 1/16th
        float intensity = 1.0f;
    };

    class GlitchProcessor {
    public:
        static void applyGlitch(std::shared_ptr<AudioClip> clip, double selectionStart, double selectionEnd, double bpm, const GlitchSettings& settings);
    };
}
