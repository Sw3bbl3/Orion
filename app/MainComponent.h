#pragma once

#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/CommandHistory.h"
#include "CommandManager.h"
#include "components/TimelineComponent.h"
#include "components/HeaderComponent.h"
#include "components/BrowserComponent.h"
#include "components/InspectorComponent.h"
#include "components/EditorComponent.h"
#include "components/MixerComponent.h"
#include "components/ExtensionViewComponent.h"
#include "components/DocumentationViewComponent.h"
#include "components/DockablePanel.h"
#include "components/DetachedWindow.h"
#include "components/SettingsWindow.h"
#include "components/DashboardComponent.h"
#include "components/CustomTitleBar.h"
#include "components/StatusBarComponent.h"
#include "components/LoadingWindow.h"
#include "components/GuidedStartWizard.h"
#include "components/CommandPaletteComponent.h"
#include "OrionLookAndFeel.h"
#include "ProjectManager.h"
#include "QwertyMidi.h"
#include "orion/UxState.h"

class MainComponent : public juce::AudioAppComponent,
                      public juce::ApplicationCommandTarget,
                      public juce::DragAndDropContainer,
                      public juce::MenuBarModel,
                      public juce::Timer,
                      public juce::MidiInputCallback,
                      public juce::KeyListener
{
public:
    MainComponent();
    ~MainComponent() override;

    void parentHierarchyChanged() override;


    void handleIncomingMidiMessage(juce::MidiInput* source, const juce::MidiMessage& message) override;


    bool keyPressed(const juce::KeyPress& key, Component* originatingComponent) override;
    bool keyStateChanged(bool isKeyDown, Component* originatingComponent) override;


    void timerCallback() override;

    void startAutoSave();
    void performAutoSave();

    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    void paint (juce::Graphics& g) override;
    void resized() override;


    ApplicationCommandTarget* getNextCommandTarget() override;
    void getAllCommands(juce::Array<juce::CommandID>& commands) override;
    void getCommandInfo(juce::CommandID commandID, juce::ApplicationCommandInfo& result) override;
    bool perform(const InvocationInfo& info) override;


    juce::StringArray getMenuBarNames() override;
    juce::PopupMenu getMenuForIndex(int menuIndex, const juce::String& menuName) override;
    void menuItemSelected(int menuItemID, int topLevelMenuIndex) override;

    void setTheme(const juce::String& themeName);


    std::function<void(float, juce::String)> onInitProgress;
    std::function<void()> onInitComplete;

private:
    enum MainState {
        Dashboard,
        Editor
    };

    MainState state = Dashboard;
    bool isFirstRun = true;
    bool lastDirtyState = false;
    int initStep = 0;

    Orion::AudioEngine engine;
    Orion::CommandHistory commandHistory;
    Orion::CommandManager commandManager;
    OrionLookAndFeel lnf;
    Orion::ProjectManager projectManager;
    Orion::QwertyMidi qwertyMidi;

    std::string currentProjectFile;
    std::thread previewThread;


    Orion::DashboardComponent dashboard;

    CustomTitleBar customTitleBar;
    StatusBarComponent statusBar;
    std::unique_ptr<LoadingWindow> loadingWindow;
    std::unique_ptr<juce::ResizableBorderComponent> windowResizer;

    HeaderComponent header;
    TimelineComponent timeline;
    BrowserComponent browser;
    InspectorComponent inspector;
    EditorComponent editor;
    MixerComponent mixer;
    Orion::ExtensionViewComponent extensionView;

    std::unique_ptr<DockablePanel> browserPanel;
    std::unique_ptr<DockablePanel> inspectorPanel;
    std::unique_ptr<DockablePanel> editorPanel;
    std::unique_ptr<DockablePanel> mixerPanel;
    std::unique_ptr<DockablePanel> extensionsPanel;

    std::unique_ptr<DetachedWindow> browserWindow;
    std::unique_ptr<DetachedWindow> inspectorWindow;
    std::unique_ptr<DetachedWindow> editorWindow;
    std::unique_ptr<DetachedWindow> mixerWindow;
    std::unique_ptr<DetachedWindow> extensionsWindow;
    std::unique_ptr<DetachedWindow> documentationWindow;
    std::unique_ptr<Orion::DocumentationViewComponent> documentationView;

    std::unique_ptr<juce::DocumentWindow> settingsWindow;
    std::unique_ptr<juce::DocumentWindow> aboutWindow;
    juce::Component::SafePointer<juce::CallOutBox> commandPaletteCallout;
    GuidedStartWizard guidedStartWizard;
    Orion::GuidedStartState guidedStartState;
    bool forceShowGuidedStart = false;
    Orion::WorkspacePreset currentWorkspacePreset = Orion::WorkspacePreset::Compose;
    bool firstPlaybackLogged = false;

    void detachPanel(DockablePanel* panel, juce::Component* content, std::unique_ptr<DetachedWindow>* window, const juce::String& name);
    void attachPanel(DockablePanel* panel, juce::Component* content, std::unique_ptr<DetachedWindow>* window);


    void closeProject();
    void saveProject(std::function<void(bool)> onComplete = nullptr);
    void promptToSaveAndClose();
    void showSettings();
    void showAbout();
    void showDocumentation();
    void showCommandPalette();
    void showShortcutCheatsheet();
    void showGuidedStartWizard(bool force = true);
    void applyGuidedStartSetup(const Orion::GuidedStartState& guidedState, bool createStarterClip = true, bool onlyIfNoTracks = true);
    void applyWorkspacePreset(Orion::WorkspacePreset preset);
    void updateGuidedStartVisibility();
    void trackUxEventProgress();


    std::string getLayoutState() const;
    void applyLayoutState(const std::string& state);
    void updateTitleBarForState();
    void updateTitleBarViewButtons();


    juce::AudioBuffer<float> browserMixBuffer;
    juce::AudioBuffer<float> dashboardMixBuffer;


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> dashboardReaderSource;
    juce::AudioTransportSource dashboardTransport;


    juce::StretchableLayoutManager verticalLayout;
    juce::StretchableLayoutManager horizontalLayout;

    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerV;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerLeft;
    std::unique_ptr<juce::StretchableLayoutResizerBar> resizerRight;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};
