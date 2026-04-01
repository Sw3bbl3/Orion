#include "orion/SettingsManager.h"
#include "orion/Logger.h"
#include "orion/UiThemeRegistry.h"
#include <fstream>
#include <filesystem>
#include <cstdlib>

namespace Orion {

    AppSettings SettingsManager::settings;
    std::string SettingsManager::filePath = "settings.orionconf";


    void to_json(nlohmann::json& j, const AppSettings& s) {
        j = nlohmann::json{
            {"audioDeviceIndex", s.audioDeviceIndex},
            {"audioDeviceName", s.audioDeviceName},
            {"sampleRate", s.sampleRate},
            {"bufferSize", s.bufferSize},
            {"audioBackend", s.audioBackend},
            {"exclusiveMode", s.exclusiveMode},
            {"vst2Paths", s.vst2Paths},
            {"vst3Paths", s.vst3Paths},
            {"theme", s.theme},
            {"themeId", s.themeId},
            {"uiScale", s.uiScale},
            {"reduceMotion", s.reduceMotion},
            {"uiMotionScale", s.uiMotionScale},
            {"uiSchemaVersion", s.uiSchemaVersion},
            {"developerMode", s.developerMode},
            {"extensionIdeShowApiExplorer", s.extensionIdeShowApiExplorer},
            {"extensionIdeShowDictionary", s.extensionIdeShowDictionary},
            {"extensionIdeShowProblems", s.extensionIdeShowProblems},
            {"extensionIdeLayoutState", s.extensionIdeLayoutState},
            {"luaLsExecutablePath", s.luaLsExecutablePath},
            {"experimentalFeatures", s.experimentalFeatures},
            {"presetMonitoringEnabled", s.presetMonitoringEnabled},
            {"glitchBrushEnabled", s.glitchBrushEnabled},
            {"fluxShaperEnabled", s.fluxShaperEnabled},
            {"prismStackEnabled", s.prismStackEnabled},
            {"autoSaveEnabled", s.autoSaveEnabled},
            {"autoSaveInterval", s.autoSaveInterval},
            {"scanPluginsAtStartup", s.scanPluginsAtStartup},
            {"pluginCacheEnabled", s.pluginCacheEnabled},
            {"pluginBackgroundVerificationOnStartup", s.pluginBackgroundVerificationOnStartup},
            {"pluginCacheSchemaVersion", s.pluginCacheSchemaVersion},
            {"setupComplete", s.setupComplete},
            {"rootDataPath", s.rootDataPath},
            {"hoverPreviewEnabled", s.hoverPreviewEnabled},
            {"returnToStartOnStop", s.returnToStartOnStop},
            {"previewLength", s.previewLength},
            {"favorites", s.favorites},
            {"pinnedLocations", s.pinnedLocations},
            {"projectLayoutState", s.projectLayoutState},
            {"panelLayoutSchemaVersion", s.panelLayoutSchemaVersion},
            {"fullTrackColoring", s.fullTrackColoring},
            {"customThemes", s.customThemes},
            {"contextualHintsEnabled", s.hintPolicy.contextualHintsEnabled},
            {"quickStartEnabled", s.hintPolicy.quickStartEnabled},
            {"showShortcutsInHints", s.hintPolicy.showShortcutsInHints},
            {"shortcutOverrides", s.shortcutOverrides}
        };
    }

    void from_json(const nlohmann::json& j, AppSettings& s) {
        if(j.contains("audioDeviceIndex")) j.at("audioDeviceIndex").get_to(s.audioDeviceIndex);
        if(j.contains("audioDeviceName")) j.at("audioDeviceName").get_to(s.audioDeviceName);
        if(j.contains("sampleRate")) j.at("sampleRate").get_to(s.sampleRate);
        if(j.contains("bufferSize")) j.at("bufferSize").get_to(s.bufferSize);

        if(j.contains("audioBackend")) j.at("audioBackend").get_to(s.audioBackend);
        if(j.contains("exclusiveMode")) j.at("exclusiveMode").get_to(s.exclusiveMode);

        if(j.contains("vst2Paths")) j.at("vst2Paths").get_to(s.vst2Paths);
        if(j.contains("vst3Paths")) j.at("vst3Paths").get_to(s.vst3Paths);
        if(j.contains("theme")) j.at("theme").get_to(s.theme);
        if(j.contains("themeId")) j.at("themeId").get_to(s.themeId);
        if(j.contains("uiScale")) j.at("uiScale").get_to(s.uiScale);
        if(j.contains("reduceMotion")) j.at("reduceMotion").get_to(s.reduceMotion);
        if(j.contains("uiMotionScale")) j.at("uiMotionScale").get_to(s.uiMotionScale);
        if(j.contains("uiSchemaVersion")) j.at("uiSchemaVersion").get_to(s.uiSchemaVersion);
        if(j.contains("developerMode")) j.at("developerMode").get_to(s.developerMode);
        if(j.contains("extensionIdeShowApiExplorer")) j.at("extensionIdeShowApiExplorer").get_to(s.extensionIdeShowApiExplorer);
        if(j.contains("extensionIdeShowDictionary")) j.at("extensionIdeShowDictionary").get_to(s.extensionIdeShowDictionary);
        if(j.contains("extensionIdeShowProblems")) j.at("extensionIdeShowProblems").get_to(s.extensionIdeShowProblems);
        if(j.contains("extensionIdeLayoutState")) j.at("extensionIdeLayoutState").get_to(s.extensionIdeLayoutState);
        if(j.contains("luaLsExecutablePath")) j.at("luaLsExecutablePath").get_to(s.luaLsExecutablePath);
        if(j.contains("experimentalFeatures")) j.at("experimentalFeatures").get_to(s.experimentalFeatures);
        if(j.contains("presetMonitoringEnabled")) j.at("presetMonitoringEnabled").get_to(s.presetMonitoringEnabled);
        if(j.contains("glitchBrushEnabled")) j.at("glitchBrushEnabled").get_to(s.glitchBrushEnabled);
        if(j.contains("fluxShaperEnabled")) j.at("fluxShaperEnabled").get_to(s.fluxShaperEnabled);
        if(j.contains("prismStackEnabled")) j.at("prismStackEnabled").get_to(s.prismStackEnabled);

        if(j.contains("autoSaveEnabled")) j.at("autoSaveEnabled").get_to(s.autoSaveEnabled);
        if(j.contains("autoSaveInterval")) j.at("autoSaveInterval").get_to(s.autoSaveInterval);
        if(j.contains("scanPluginsAtStartup")) j.at("scanPluginsAtStartup").get_to(s.scanPluginsAtStartup);
        if(j.contains("pluginCacheEnabled")) j.at("pluginCacheEnabled").get_to(s.pluginCacheEnabled);
        if(j.contains("pluginBackgroundVerificationOnStartup")) j.at("pluginBackgroundVerificationOnStartup").get_to(s.pluginBackgroundVerificationOnStartup);
        if(j.contains("pluginCacheSchemaVersion")) j.at("pluginCacheSchemaVersion").get_to(s.pluginCacheSchemaVersion);

        if(j.contains("rootDataPath")) j.at("rootDataPath").get_to(s.rootDataPath);
        if(j.contains("hoverPreviewEnabled")) j.at("hoverPreviewEnabled").get_to(s.hoverPreviewEnabled);
        if(j.contains("returnToStartOnStop")) j.at("returnToStartOnStop").get_to(s.returnToStartOnStop);
        if(j.contains("previewLength")) j.at("previewLength").get_to(s.previewLength);
        if(j.contains("favorites")) j.at("favorites").get_to(s.favorites);
        if(j.contains("pinnedLocations")) j.at("pinnedLocations").get_to(s.pinnedLocations);
        if(j.contains("projectLayoutState")) j.at("projectLayoutState").get_to(s.projectLayoutState);
        if(j.contains("panelLayoutSchemaVersion")) j.at("panelLayoutSchemaVersion").get_to(s.panelLayoutSchemaVersion);
        if(j.contains("fullTrackColoring")) j.at("fullTrackColoring").get_to(s.fullTrackColoring);
        if(j.contains("customThemes")) j.at("customThemes").get_to(s.customThemes);
        if(j.contains("contextualHintsEnabled")) j.at("contextualHintsEnabled").get_to(s.hintPolicy.contextualHintsEnabled);
        if(j.contains("quickStartEnabled")) j.at("quickStartEnabled").get_to(s.hintPolicy.quickStartEnabled);
        if(j.contains("showShortcutsInHints")) j.at("showShortcutsInHints").get_to(s.hintPolicy.showShortcutsInHints);
        if(j.contains("shortcutOverrides")) j.at("shortcutOverrides").get_to(s.shortcutOverrides);

        // Theme compatibility migration: preserve old "theme" values and resolve to a canonical v2 theme id.
        const bool hasCustomThemeByName = !s.theme.empty() && s.customThemes.contains(s.theme);
        const bool hasCustomThemeById = !s.themeId.empty() && s.customThemes.contains(s.themeId);
        if (hasCustomThemeByName)
        {
            s.themeId = s.theme;
        }
        else if (hasCustomThemeById)
        {
            s.theme = s.themeId;
        }
        else
        {
            const auto resolved = UiThemeRegistry::resolveThemeId(s.themeId.empty() ? s.theme : s.themeId);
            if (UiThemeRegistry::isKnownThemeId(resolved))
            {
                s.themeId = resolved;
                s.theme = UiThemeRegistry::getDisplayName(resolved);
            }
            else
            {
                s.themeId = "studio_one_dark";
                s.theme = UiThemeRegistry::getDisplayName(s.themeId);
            }
        }

        if (s.uiSchemaVersion < 2)
        {
            // Existing users keep legacy behavior by default, but can opt in later.
            s.reduceMotion = false;
            s.uiMotionScale = 1.0f;
            s.uiSchemaVersion = 2;
        }

        if(j.contains("setupComplete")) {
            j.at("setupComplete").get_to(s.setupComplete);
        } else {

            s.setupComplete = true;
        }
    }

    std::string getUserDataPath() {
#ifdef _WIN32
        const char* appData = std::getenv("APPDATA");
        if (appData) return std::string(appData) + "/Orion";
#else
        const char* home = std::getenv("HOME");
        if (home) {
#ifdef __APPLE__
            return std::string(home) + "/Library/Application Support/Orion";
#else
            return std::string(home) + "/.config/Orion";
#endif
        }
#endif
        return ".";
    }

    std::string SettingsManager::resolveSettingsPath() {
        if (std::filesystem::exists("orion_location.txt")) {
            std::ifstream f("orion_location.txt");
            std::string path;
            if (std::getline(f, path) && !path.empty()) {

                auto end = path.find_last_not_of(" \n\r\t");
                if (end != std::string::npos)
                    path = path.substr(0, end + 1);
                return path;
            }
        }

        std::string userDir = getUserDataPath();
        if (userDir != ".") {
            std::filesystem::create_directories(userDir);
            return (std::filesystem::path(userDir) / "settings.orionconf").string();
        }
        return "settings.orionconf";
    }

    void SettingsManager::load() {

        if (!std::filesystem::exists("orion_location.txt")) {
             if (std::filesystem::exists("settings.json") && !std::filesystem::exists("settings.orionconf")) {
                 try {
                     std::filesystem::rename("settings.json", "settings.orionconf");
                     Logger::info("Migrated legacy settings.json to settings.orionconf");
                 } catch(...) {}
             }

             if (std::filesystem::exists("settings.orionconf")) {

                 std::string newPath = resolveSettingsPath();
                 if (newPath != "settings.orionconf") {
                     try {
                         std::filesystem::copy_file("settings.orionconf", newPath, std::filesystem::copy_options::overwrite_existing);
                         Logger::info("Copied settings.orionconf to " + newPath);
                     } catch(...) {}
                 }
             }
        }

        filePath = resolveSettingsPath();


        std::filesystem::path p(filePath);
        if (p.extension() == ".json") {
             std::filesystem::path newP = p;
             newP.replace_extension(".orionconf");

             bool migrated = false;
             if (std::filesystem::exists(p) && !std::filesystem::exists(newP)) {
                 try {
                     std::filesystem::rename(p, newP);
                     filePath = newP.string();
                     migrated = true;
                     Logger::info("Migrated redirected settings to .orionconf");
                 } catch(...) {}
             } else if (std::filesystem::exists(newP)) {
                 filePath = newP.string();
                 migrated = true;
             }

             if (migrated && std::filesystem::exists("orion_location.txt")) {
                 try {
                     std::ofstream f("orion_location.txt");
                     f << filePath;
                 } catch(...) {}
             }
        }

        if (!std::filesystem::exists(filePath)) {

            settings.audioDeviceIndex = -1;
            settings.audioDeviceName = "";
            settings.sampleRate = 44100;
            settings.bufferSize = 128;
            settings.audioBackend = 0;
            settings.exclusiveMode = false;
            return;
        }

        try {
            std::ifstream f(filePath);
            nlohmann::json j;
            f >> j;
            settings = j.get<AppSettings>();
            Logger::info("Settings loaded from: " + filePath);
        } catch (const std::exception& e) {
            Logger::error(std::string("Failed to load settings: ") + e.what());
        }
    }

    void SettingsManager::save() {
        try {
            nlohmann::json j = settings;


            std::filesystem::path p(filePath);
            if (p.has_parent_path() && !std::filesystem::exists(p.parent_path())) {
                 std::filesystem::create_directories(p.parent_path());
            }

            std::ofstream f(filePath);
            f << j.dump(4);
            Logger::info("Settings saved to: " + filePath);
        } catch (const std::exception& e) {
            Logger::error(std::string("Failed to save settings: ") + e.what());
        }
    }

    void SettingsManager::set(const AppSettings& newSettings) {
        settings = newSettings;
        save();
    }

    void SettingsManager::setInstanceRoot(const std::string& newPath) {
        std::filesystem::path root(newPath);
        std::filesystem::path newSettingsPath = root / "settings.orionconf";


        try {
            std::ofstream f("orion_location.txt");
            f << newSettingsPath.string();
            f.close();
        } catch (const std::exception& e) {
             Logger::error("Failed to write redirection file: " + std::string(e.what()));
        }


        if (std::filesystem::exists(filePath)) {
             try {
                 bool same = false;
                 if (std::filesystem::exists(newSettingsPath)) {
                     try {
                        if (std::filesystem::equivalent(filePath, newSettingsPath)) same = true;
                     } catch(...) {}
                 }

                 if (!same) {
                     std::filesystem::copy_file(filePath, newSettingsPath, std::filesystem::copy_options::overwrite_existing);
                     std::filesystem::remove(filePath);
                 }
             } catch (const std::exception& e) {
                 Logger::error("Failed to move settings file: " + std::string(e.what()));
             }
        }

        filePath = newSettingsPath.string();
        settings.rootDataPath = newPath;
        save();
    }

    std::string SettingsManager::getSettingsFilePath() {
        return filePath;
    }

    std::string SettingsManager::getSettingsDirectory() {
        std::filesystem::path p(filePath.empty() ? resolveSettingsPath() : filePath);
        if (!p.has_parent_path()) return ".";
        return p.parent_path().string();
    }

}
