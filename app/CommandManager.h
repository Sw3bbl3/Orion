#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"

namespace Orion {

    class CommandManager : public juce::ApplicationCommandTarget
    {
    public:
        CommandManager(AudioEngine& engine);

        juce::ApplicationCommandManager& getApplicationCommandManager() { return commandManager; }

        enum CommandIDs
        {
            PlayStop = 0x2000,
            Stop,
            Record,
            ToggleLoop,
            ToggleClick,
            ReturnToZero,
            Rewind,
            FastForward,
            Panic,

            Undo,
            Redo,
            Save,
            Load,
            Export,

            DeleteSelection,

            ToolSelect,
            ToolDraw,
            ToolErase,
            ToolSplit,
            ToolGlue,
            ToolZoom,
            ToolMute,
            ToolListen,

            AddAudioTrack,
            AddInstrumentTrack,
            AddBusTrack,

            ShowSettings,
            ShowCommandPalette,
            OpenShortcutCheatsheet,
            ShowGuidedStartWizard
        };


        ApplicationCommandTarget* getNextCommandTarget() override;
        void getAllCommands(juce::Array<juce::CommandID>& commands) override;
        void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
        bool perform(const InvocationInfo& info) override;

        void registerAllCommands();

    private:
        AudioEngine& engine;
        juce::ApplicationCommandManager commandManager;
    };

}
