#pragma once

#include <string>

namespace Orion {

enum class WorkspacePreset {
    Compose,
    Arrange,
    Mix
};

enum class GuidedStartTrackType {
    Instrument,
    Audio
};

struct HintPolicy {
    bool contextualHintsEnabled = true;
    bool quickStartEnabled = true;
    bool showShortcutsInHints = true;
};

struct GuidedStartState {
    bool dismissed = false;
    GuidedStartTrackType lastTrackType = GuidedStartTrackType::Instrument;
    float lastBpm = 120.0f;
    int lastLoopBars = 8;
};

struct QuickStartState {
    bool firstTrackAdded = false;
    bool firstClipCreated = false;
    bool firstNotesAdded = false;
    bool loopEnabled = false;
    bool firstExport = false;

    bool dismissed = false;
    std::string lastHintId;

    int completedCount() const {
        int count = 0;
        if (firstTrackAdded) ++count;
        if (firstClipCreated) ++count;
        if (firstNotesAdded) ++count;
        if (loopEnabled) ++count;
        if (firstExport) ++count;
        return count;
    }

    bool isComplete() const {
        return completedCount() == 5;
    }
};

} // namespace Orion
