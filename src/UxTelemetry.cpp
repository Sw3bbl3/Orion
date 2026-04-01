#include "orion/UxTelemetry.h"
#include "orion/SettingsManager.h"
#include "orion/Logger.h"

#include <filesystem>
#include <fstream>
#include <chrono>

namespace Orion {

std::string UxTelemetry::eventName(UxEventType eventType)
{
    switch (eventType)
    {
        case UxEventType::ProjectCreated:   return "project_created";
        case UxEventType::FirstTrackAdded:  return "first_track_added";
        case UxEventType::FirstClipCreated: return "first_clip_created";
        case UxEventType::PianoRollOpened:  return "piano_roll_opened";
        case UxEventType::FirstNotesAdded:  return "first_notes_added";
        case UxEventType::FirstPlayback:    return "first_playback";
        case UxEventType::FirstExport:      return "first_export";
        case UxEventType::GuidedStartCompleted: return "guided_start_completed";
        case UxEventType::GuidedStartCancelled: return "guided_start_cancelled";
        default:                            return "unknown";
    }
}

void UxTelemetry::logEvent(UxEventType eventType, const nlohmann::json& payload)
{
    try
    {
        std::filesystem::path basePath = SettingsManager::get().rootDataPath.empty()
            ? std::filesystem::current_path()
            : std::filesystem::path(SettingsManager::get().rootDataPath);

        std::filesystem::path logPath = basePath / "ux_events.jsonl";
        if (logPath.has_parent_path())
            std::filesystem::create_directories(logPath.parent_path());

        const auto nowMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

        nlohmann::json j;
        j["ts_ms"] = nowMs;
        j["event"] = eventName(eventType);
        if (!payload.is_null() && !payload.empty())
            j["payload"] = payload;

        std::ofstream out(logPath.string(), std::ios::app);
        out << j.dump() << "\n";
    }
    catch (const std::exception& e)
    {
        Logger::error(std::string("UxTelemetry log failed: ") + e.what());
    }
}

} // namespace Orion
