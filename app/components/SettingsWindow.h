#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_devices/juce_audio_devices.h>
#include "orion/SettingsManager.h"
#include "../OrionLookAndFeel.h"

class SettingsWindow : public juce::Component
{
public:
    enum class CloseMode
    {
        ModalWindow,
        Embedded
    };

    SettingsWindow(juce::AudioDeviceManager& deviceManager,
                   std::function<void(const juce::String&)> onThemeChange,
                   CloseMode closeMode = CloseMode::ModalWindow);
    ~SettingsWindow() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Component navPanel;
    juce::Component contentPanel;
    juce::TextButton generalNavButton, audioNavButton, pluginsNavButton, appearanceNavButton, commandsNavButton, advancedNavButton;
    std::function<void(const juce::String&)> onThemeChangeCallback;

    Orion::AppSettings bufferedSettings;
    std::string initialTheme;
    float initialScale;
    CloseMode closeMode = CloseMode::ModalWindow;

    juce::TextButton okButton, applyButton, cancelButton;

    class GeneralPage : public juce::Component
    {
    public:
        GeneralPage(Orion::AppSettings& settings, std::function<void(const juce::String&)> callback);
        void paint(juce::Graphics& g) override;
        void resized() override;
    private:
        Orion::AppSettings& settings;
        std::function<void(const juce::String&)> onThemeChangeCallback;
        juce::Label themeLabel;
        juce::ComboBox themeSelector;
        juce::Label scaleLabel;
        juce::Slider scaleSlider;
        juce::ToggleButton autoSaveToggle;
        juce::ToggleButton hoverPreviewToggle;
        juce::ToggleButton returnToStartToggle;
        juce::ToggleButton contextualHintsToggle;
        juce::ToggleButton quickStartToggle;
        juce::ToggleButton shortcutsInHintsToggle;
        juce::Label autoSaveIntervalLabel;
        juce::Slider autoSaveIntervalSlider;
        juce::Label previewLengthLabel;
        juce::Slider previewLengthSlider;
        juce::Label pathHeaderLabel;
        juce::Label currentPathDisplay;
        juce::TextButton browsePathBtn;
    };


    std::unique_ptr<juce::AudioDeviceSelectorComponent> audioPage;


    class PluginPage : public juce::Component, public juce::ListBoxModel
    {
    public:
        PluginPage(Orion::AppSettings& settings);
        void paint(juce::Graphics& g) override;
        void resized() override;
        int getNumRows() override;
        void paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void updateList();
    private:
        void addPath();
        void removePath();
        void rescan();
        Orion::AppSettings& settings;
        juce::Label headerLabel;
        juce::ListBox pathList;
        juce::TextButton addButton, removeButton, rescanButton;
        juce::ToggleButton scanOnStartupToggle;
        juce::Label cachePathLabel;
        juce::Label cacheStatsLabel;
        juce::TextButton clearCacheButton;
    };


    class AdvancedPage : public juce::Component
    {
    public:
        AdvancedPage(Orion::AppSettings& settings);
        void paint(juce::Graphics& g) override;
        void resized() override;
    private:
        Orion::AppSettings& settings;
        juce::ToggleButton devModeToggle;
        juce::Label devDescriptionLabel;
        juce::Label warningLabel;
        juce::ToggleButton expFeaturesToggle;
        juce::ToggleButton presetMonitoringToggle;
        juce::ToggleButton fluxShaperToggle;
        juce::ToggleButton prismStackToggle;
        juce::ToggleButton pluginCacheToggle;
        juce::ToggleButton startupVerifyToggle;
        juce::ToggleButton extensionApiExplorerToggle;
        juce::ToggleButton extensionDictionaryToggle;
        juce::ToggleButton extensionProblemsToggle;
        juce::Label luaLsPathLabel;
        juce::TextEditor luaLsPathEditor;
        juce::Label maintenanceHeaderLabel;
        juce::TextButton reinstallExtensionsBtn;
        juce::TextButton rescanPluginsBtn;
        juce::Label statusLabel;
    };

    class AppearancePage : public juce::Component, public juce::ChangeListener
    {
    public:
        AppearancePage(Orion::AppSettings& settings, std::function<void(const juce::String&)> themeCallback);
        void paint(juce::Graphics& g) override;
        void resized() override;
        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        void mouseDown(const juce::MouseEvent& e) override;
    private:
        Orion::AppSettings& settings;
        std::function<void(const juce::String&)> onThemeChangeCallback;
        juce::Label themeLabel;
        juce::ComboBox themeSelector;
        juce::ToggleButton fullColorToggle;
        juce::TextButton saveThemeBtn;
        juce::Label colorLabel;
        juce::Label accentColorPreview;
        void openColorPicker();
    };

    class CommandsPage : public juce::Component
    {
    public:
        CommandsPage(Orion::AppSettings& settings);
        void paint(juce::Graphics& g) override;
        void resized() override;
    private:
        struct ActionRow {
            std::string id;
            juce::String title;
            juce::String defaultShortcut;
            std::unique_ptr<juce::Label> label;
            std::unique_ptr<juce::TextEditor> editor;
        };

        Orion::AppSettings& settings;
        juce::Label headerLabel;
        juce::Label conflictLabel;
        juce::TextEditor filterBox;
        juce::Viewport listViewport;
        juce::Component listContent;
        std::vector<ActionRow> rows;

        void buildRows();
        void applyFilter();
        void updateConflicts();
    };

    std::unique_ptr<GeneralPage> generalPage;
    std::unique_ptr<AppearancePage> appearancePage;
    std::unique_ptr<PluginPage> pluginPage;
    std::unique_ptr<AdvancedPage> advancedPage;
    std::unique_ptr<CommandsPage> commandsPage;

    void applySettings();
    void closeFromButtons(int modalResult);
    void selectPage(int index);
    void styleNavButton(juce::TextButton& button, int pageIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsWindow)
};
