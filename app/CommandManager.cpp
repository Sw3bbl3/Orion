#include "CommandManager.h"

namespace Orion {

    CommandManager::CommandManager(AudioEngine& e) : engine(e)
    {
    }

    void CommandManager::registerAllCommands()
    {
        commandManager.registerAllCommandsForTarget(this);
    }

    juce::ApplicationCommandTarget* CommandManager::getNextCommandTarget()
    {
        return nullptr;
    }

    void CommandManager::getAllCommands(juce::Array<juce::CommandID>& commands)
    {


        commands.add(PlayStop);
        commands.add(Stop);
        commands.add(Record);
        commands.add(ToggleLoop);
        commands.add(ToggleClick);
        commands.add(ReturnToZero);
        commands.add(Rewind);
        commands.add(FastForward);
        commands.add(Panic);

        commands.add(AddAudioTrack);
        commands.add(AddInstrumentTrack);
        commands.add(AddBusTrack);
        commands.add(ShowCommandPalette);
        commands.add(OpenShortcutCheatsheet);
        commands.add(ShowGuidedStartWizard);
    }

    void CommandManager::getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result)
    {

        switch (commandID)
        {
        case PlayStop:
            result.setInfo("Play/Stop", "Toggles playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::spaceKey, 0);
            break;
        case Stop:
            result.setInfo("Stop", "Stops playback", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::escapeKey, 0);
            break;
        case Record:
            result.setInfo("Record", "Toggles recording", "Transport", 0);
            result.addDefaultKeypress('r', 0);
            result.addDefaultKeypress('*', 0);
            break;
        case ToggleLoop:
            result.setInfo("Loop", "Toggles looping", "Transport", 0);
            result.addDefaultKeypress('/', 0);
            break;
        case ToggleClick:
            result.setInfo("Click", "Toggles metronome", "Transport", 0);
            result.addDefaultKeypress('c', 0);
            break;
        case ReturnToZero:
            result.setInfo("Return to Zero", "Moves playhead to start or stops", "Transport", 0);
            result.addDefaultKeypress(juce::KeyPress::returnKey, 0);
            break;
        case Rewind:
            result.setInfo("Rewind", "Moves playhead backward", "Transport", 0);
            result.addDefaultKeypress(',', 0);
            break;
        case FastForward:
            result.setInfo("Fast Forward", "Moves playhead forward", "Transport", 0);
            result.addDefaultKeypress('.', 0);
            break;
        case Panic:
            result.setInfo("Panic", "Stops all sound immediately", "Transport", 0);
            break;
        case Save:
            result.setInfo("Save", "Saves the project", "File", 0);
            result.addDefaultKeypress('s', juce::ModifierKeys::commandModifier);
            break;
        case Load:
            result.setInfo("Open...", "Open Project", "File", 0);
            result.addDefaultKeypress('o', juce::ModifierKeys::commandModifier);
            break;
        case Export:
            result.setInfo("Export...", "Export Project to Audio File", "File", 0);
            result.addDefaultKeypress('e', juce::ModifierKeys::commandModifier);
            break;
        case Undo:
            result.setInfo("Undo", "Undo last action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier);
            break;
        case Redo:
            result.setInfo("Redo", "Redo last action", "Edit", 0);
            result.addDefaultKeypress('z', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;
        case DeleteSelection:
            result.setInfo("Delete", "Deletes selected items", "Edit", 0);
            result.addDefaultKeypress(juce::KeyPress::deleteKey, 0);
            result.addDefaultKeypress(juce::KeyPress::backspaceKey, 0);
            break;
        case ToolSelect:
            result.setInfo("Select Tool", "Switch to selection tool", "Tools", 0);
            result.addDefaultKeypress('1', 0);
            break;
        case ToolDraw:
            result.setInfo("Draw Tool", "Switch to draw tool", "Tools", 0);
            result.addDefaultKeypress('2', 0);
            break;
        case ToolErase:
            result.setInfo("Erase Tool", "Switch to erase tool", "Tools", 0);
            result.addDefaultKeypress('3', 0);
            break;
        case ToolSplit:
            result.setInfo("Split Tool", "Switch to split tool", "Tools", 0);
            result.addDefaultKeypress('4', 0);
            break;
        case ToolGlue:
            result.setInfo("Glue Tool", "Switch to glue tool", "Tools", 0);
            result.addDefaultKeypress('5', 0);
            break;
        case ToolZoom:
            result.setInfo("Zoom Tool", "Switch to zoom tool", "Tools", 0);
            result.addDefaultKeypress('6', 0);
            break;
        case ToolMute:
            result.setInfo("Mute Tool", "Switch to mute tool", "Tools", 0);
            result.addDefaultKeypress('7', 0);
            break;
        case ToolListen:
            result.setInfo("Listen Tool", "Switch to listen tool", "Tools", 0);
            result.addDefaultKeypress('8', 0);
            break;

        case AddAudioTrack:
            result.setInfo("Add Audio Track", "Create a new audio track", "Track", 0);
            result.addDefaultKeypress('T', juce::ModifierKeys::commandModifier);
            break;
        case AddInstrumentTrack:
            result.setInfo("Add Instrument Track", "Create a new instrument track", "Track", 0);
            result.addDefaultKeypress('I', juce::ModifierKeys::commandModifier);
            break;
        case AddBusTrack:
            result.setInfo("Add Bus Track", "Create a new bus track", "Track", 0);
            result.addDefaultKeypress('B', juce::ModifierKeys::commandModifier);
            break;
        case ShowCommandPalette:
            result.setInfo("Command Palette", "Search and run commands", "View", 0);
            result.addDefaultKeypress('k', juce::ModifierKeys::commandModifier);
            break;
        case OpenShortcutCheatsheet:
            result.setInfo("Shortcut Cheatsheet", "Show keyboard shortcuts", "Help", 0);
            result.addDefaultKeypress('/', juce::ModifierKeys::commandModifier);
            break;
        case ShowGuidedStartWizard:
            result.setInfo("Quick Start", "Apply quick starter setup (track + loop)", "View", 0);
            result.addDefaultKeypress('G', juce::ModifierKeys::commandModifier | juce::ModifierKeys::shiftModifier);
            break;

        default:
            break;
        }
    }

    bool CommandManager::perform(const InvocationInfo& info)
    {
        switch (info.commandID)
        {
        case PlayStop:
            if (engine.getTransportState() == TransportState::Playing || engine.getTransportState() == TransportState::Recording)
                engine.pause();
            else
                engine.play();
            return true;
        case Stop:
            engine.stop();
            return true;
        case Record:
            engine.record();
            return true;
        case ToggleLoop:
            engine.setLooping(!engine.isLooping());
            return true;
        case ToggleClick:
            engine.setMetronomeEnabled(!engine.isMetronomeEnabled());
            return true;
        case ReturnToZero:
            engine.stop();
            engine.setCursor(0);
            return true;
        case Rewind:
            {
                uint64_t cur = engine.getCursor();
                uint64_t step = (uint64_t)engine.getSampleRate();
                if (cur > step) engine.setCursor(cur - step);
                else engine.setCursor(0);
            }
            return true;
        case FastForward:
            {
                uint64_t cur = engine.getCursor();
                uint64_t step = (uint64_t)engine.getSampleRate();
                engine.setCursor(cur + step);
            }
            return true;
        case Panic:
            engine.panic();
            return true;
        default:
            return false;
        }
    }

}
