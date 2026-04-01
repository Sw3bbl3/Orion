#pragma once

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <nlohmann/json.hpp>
#include "UxState.h"

namespace Orion {

    struct AppSettings {
        int audioDeviceIndex = -1;
        std::string audioDeviceName = "";
        int sampleRate = 44100;
        int bufferSize = 128;

        int audioBackend = 0;
        bool exclusiveMode = false;

        std::vector<std::string> vst2Paths;
        std::vector<std::string> vst3Paths;
        std::string theme = "Dark";
        std::string themeId = "studio_one_dark";
        float uiScale = 1.0f;
        bool reduceMotion = false;
        float uiMotionScale = 1.0f;
        int uiSchemaVersion = 2;


        bool experimentalFeatures = false;
        bool presetMonitoringEnabled = false;
        bool glitchBrushEnabled = false;
        bool fluxShaperEnabled = false;
        bool prismStackEnabled = false;


        bool developerMode = false;

        bool extensionIdeShowApiExplorer = true;
        bool extensionIdeShowDictionary = true;
        bool extensionIdeShowProblems = true;
        std::string extensionIdeLayoutState = "";
        std::string luaLsExecutablePath = "";


        bool autoSaveEnabled = true;
        int autoSaveInterval = 5;

        bool scanPluginsAtStartup = true;
        bool pluginCacheEnabled = true;
        bool pluginBackgroundVerificationOnStartup = true;
        int pluginCacheSchemaVersion = 2;

        bool setupComplete = false;
        std::string rootDataPath = "";
        bool hoverPreviewEnabled = false;
        bool returnToStartOnStop = false;
        float previewLength = 3.0f;


        std::vector<std::string> favorites;
        std::vector<std::string> pinnedLocations;

        std::string projectLayoutState;
        int panelLayoutSchemaVersion = 2;

        bool fullTrackColoring = false;
        nlohmann::json customThemes;

        HintPolicy hintPolicy;
        std::map<std::string, std::string> shortcutOverrides;
    };

    class SettingsManager {
    public:
        static void load();
        static void save();

        static AppSettings& get() { return settings; }
        static void set(const AppSettings& newSettings);

        static void setInstanceRoot(const std::string& newPath);
        static std::string getSettingsFilePath();
        static std::string getSettingsDirectory();

    private:
        static AppSettings settings;
        static std::string filePath;

        static std::string resolveSettingsPath();
    };

}
