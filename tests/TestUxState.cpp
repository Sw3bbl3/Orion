#include <iostream>
#include "orion/UxState.h"

int main()
{
    Orion::QuickStartState s;

    if (s.completedCount() != 0) {
        std::cerr << "Expected 0 completed tasks\n";
        return 1;
    }

    s.firstTrackAdded = true;
    s.firstClipCreated = true;
    if (s.completedCount() != 2) {
        std::cerr << "Expected 2 completed tasks\n";
        return 1;
    }

    s.firstNotesAdded = true;
    s.loopEnabled = true;
    s.firstExport = true;
    if (!s.isComplete()) {
        std::cerr << "Expected quick start to be complete\n";
        return 1;
    }

    Orion::HintPolicy policy;
    if (!policy.contextualHintsEnabled || !policy.quickStartEnabled || !policy.showShortcutsInHints) {
        std::cerr << "Expected default hint policy flags enabled\n";
        return 1;
    }

    Orion::GuidedStartState guided;
    if (guided.dismissed) {
        std::cerr << "Expected guided start dismissed to default false\n";
        return 1;
    }
    if (guided.lastTrackType != Orion::GuidedStartTrackType::Instrument) {
        std::cerr << "Expected default guided start track type to Instrument\n";
        return 1;
    }
    if (guided.lastBpm != 120.0f || guided.lastLoopBars != 8) {
        std::cerr << "Expected default guided start bpm/loop values\n";
        return 1;
    }

    std::cout << "TestUxState passed\n";
    return 0;
}
