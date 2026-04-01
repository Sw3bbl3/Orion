#pragma once

#include <juce_core/juce_core.h>
#include "orion/Logger.h"
#include <string>
#include <vector>
#include <functional>


struct lua_State;

namespace Orion {

    class AudioEngine;
    class ProjectManager;

    struct Extension {
        std::string name;
        std::string description;  // from manifest.lua
        std::string author;       // from manifest.lua
        std::string version;      // from manifest.lua
        std::string mainScript;
        std::string dir;          // absolute path to the extension directory
        bool loaded = false;
    };


    class ExtensionManager {
    public:
        static ExtensionManager& getInstance();

        void initialize();
        void shutdown();


        void setContext(AudioEngine* engine, ProjectManager* projectManager);

        AudioEngine* getAudioEngine() const { return engine; }
        ProjectManager* getProjectManager() const { return projectManager; }


        bool runScript(const std::string& scriptPath);
        bool runCode(const std::string& code);


        void scanExtensions();
        const std::vector<Extension>& getExtensions() const { return extensions; }

        bool importExtension(const juce::File& oexFile);
        bool exportExtension(const std::string& name, const juce::File& targetFile);
        bool deleteExtension(const std::string& name);


        void log(const std::string& msg, LogLevel level = LogLevel::Info);
        const std::vector<LogMessage>& getLogHistory() const { return logHistory; }
        std::function<void(const LogMessage&)> onLogMessage;


        struct SelectedClipInfo {
            int trackIndex;
            int clipIndex;
        };
        struct SelectedNoteInfo {
            int trackIndex;
            int clipIndex;
            int noteIndex;
        };

        std::function<std::vector<SelectedClipInfo>()> getSelectedClips;
        std::function<std::vector<SelectedNoteInfo>()> getSelectedNotes;

        // Theme query hook — set by the UI layer
        std::function<std::string()> getCurrentThemeName;

        void installFactoryExtensions(bool force = false);

    private:
        ExtensionManager() = default;
        ~ExtensionManager() = default;

        lua_State* L = nullptr;
        std::vector<Extension> extensions;
        std::vector<LogMessage> logHistory;

        AudioEngine* engine = nullptr;
        ProjectManager* projectManager = nullptr;

        void bindAPI();
    };

}
