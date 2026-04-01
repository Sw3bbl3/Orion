#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace Orion {

enum class UxEventType {
    ProjectCreated,
    FirstTrackAdded,
    FirstClipCreated,
    PianoRollOpened,
    FirstNotesAdded,
    FirstPlayback,
    FirstExport,
    GuidedStartCompleted,
    GuidedStartCancelled
};

class UxTelemetry {
public:
    static void logEvent(UxEventType eventType, const nlohmann::json& payload = {});
    static std::string eventName(UxEventType eventType);
};

} // namespace Orion
