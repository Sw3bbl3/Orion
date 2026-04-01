#include "SettingsWindow.h"
#include "../OrionLookAndFeel.h"
#include "orion/PluginManager.h"
#include "orion/UiThemeRegistry.h"
#include "../OrionIcons.h"
#include "../ExtensionManager.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"



SettingsWindow::SettingsWindow(juce::AudioDeviceManager& deviceManager,
                               std::function<void(const juce::String&)> onThemeChange,
                               CloseMode mode)
    : onThemeChangeCallback(onThemeChange),
      bufferedSettings(Orion::SettingsManager::get()),
      initialTheme(bufferedSettings.theme),
      initialScale(bufferedSettings.uiScale),
      closeMode(mode)
{
    addAndMakeVisible(navPanel);
    addAndMakeVisible(contentPanel);

    generalPage = std::make_unique<GeneralPage>(bufferedSettings, onThemeChangeCallback);
    contentPanel.addAndMakeVisible(generalPage.get());

    appearancePage = std::make_unique<AppearancePage>(bufferedSettings, onThemeChangeCallback);
    contentPanel.addAndMakeVisible(appearancePage.get());

    audioPage = std::make_unique<juce::AudioDeviceSelectorComponent>(
        deviceManager,
        0, 256,
        0, 256,
        true,
        true,
        true,
        false
    );
    contentPanel.addAndMakeVisible(audioPage.get());


    pluginPage = std::make_unique<PluginPage>(bufferedSettings);
    contentPanel.addAndMakeVisible(pluginPage.get());

    advancedPage = std::make_unique<AdvancedPage>(bufferedSettings);
    contentPanel.addAndMakeVisible(advancedPage.get());

    commandsPage = std::make_unique<CommandsPage>(bufferedSettings);
    contentPanel.addAndMakeVisible(commandsPage.get());

    addAndMakeVisible(generalNavButton);
    generalNavButton.setButtonText("General");
    generalNavButton.onClick = [this] { selectPage(0); };
    addAndMakeVisible(audioNavButton);
    audioNavButton.setButtonText("Audio");
    audioNavButton.onClick = [this] { selectPage(1); };
    addAndMakeVisible(pluginsNavButton);
    pluginsNavButton.setButtonText("Plugins");
    pluginsNavButton.onClick = [this] { selectPage(2); };
    addAndMakeVisible(appearanceNavButton);
    appearanceNavButton.setButtonText("Appearance");
    appearanceNavButton.onClick = [this] { selectPage(3); };
    addAndMakeVisible(commandsNavButton);
    commandsNavButton.setButtonText("Commands");
    commandsNavButton.onClick = [this] { selectPage(4); };
    addAndMakeVisible(advancedNavButton);
    advancedNavButton.setButtonText("Advanced");
    advancedNavButton.onClick = [this] { selectPage(5); };

    addAndMakeVisible(okButton);
    okButton.setButtonText("OK");
    Orion::UI::Primitives::stylePrimaryButton(okButton);
    okButton.onClick = [this] {
        applySettings();
        closeFromButtons(1);
    };

    addAndMakeVisible(applyButton);
    applyButton.setButtonText("Apply");
    Orion::UI::Primitives::styleSecondaryButton(applyButton);
    applyButton.onClick = [this] { applySettings(); };

    addAndMakeVisible(cancelButton);
    cancelButton.setButtonText("Cancel");
    Orion::UI::Primitives::styleGhostButton(cancelButton);
    cancelButton.onClick = [this] {
        if (onThemeChangeCallback && initialTheme != bufferedSettings.theme)
            onThemeChangeCallback(initialTheme);

        if (initialScale != bufferedSettings.uiScale)
            juce::Desktop::getInstance().setGlobalScaleFactor(initialScale);

        closeFromButtons(0);
    };

    selectPage(0);
    setSize(980, 680);
}

SettingsWindow::~SettingsWindow()
{
}

void SettingsWindow::paint(juce::Graphics& g)
{
    Orion::UI::Primitives::paintWindowBackdrop(g, getLocalBounds().toFloat());

    auto shell = getLocalBounds().toFloat().reduced(8.0f);
    Orion::UI::Primitives::paintPanelSurface(g, shell, 10.0f, true);

    auto sidebar = navPanel.getBounds().toFloat().expanded(6.0f, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.18f));
    g.fillRoundedRectangle(sidebar, 8.0f);

    auto content = contentPanel.getBounds().toFloat().expanded(6.0f, 4.0f);
    g.setColour(juce::Colours::black.withAlpha(0.16f));
    g.fillRoundedRectangle(content, 8.0f);
}

void SettingsWindow::resized()
{
    auto area = getLocalBounds().reduced(12);
    auto bottomBar = area.removeFromBottom(48).reduced(0, 6);

    cancelButton.setBounds(bottomBar.removeFromRight(100));
    bottomBar.removeFromRight(10);
    applyButton.setBounds(bottomBar.removeFromRight(100));
    bottomBar.removeFromRight(10);
    okButton.setBounds(bottomBar.removeFromRight(100));

    auto body = area.reduced(0, 4);
    auto nav = body.removeFromLeft(180).reduced(4);
    navPanel.setBounds(nav);
    contentPanel.setBounds(body.reduced(4));

    auto navItems = nav.reduced(8);
    generalNavButton.setBounds(navItems.removeFromTop(34));
    navItems.removeFromTop(6);
    audioNavButton.setBounds(navItems.removeFromTop(34));
    navItems.removeFromTop(6);
    pluginsNavButton.setBounds(navItems.removeFromTop(34));
    navItems.removeFromTop(6);
    appearanceNavButton.setBounds(navItems.removeFromTop(34));
    navItems.removeFromTop(6);
    commandsNavButton.setBounds(navItems.removeFromTop(34));
    navItems.removeFromTop(6);
    advancedNavButton.setBounds(navItems.removeFromTop(34));

    auto contentBounds = contentPanel.getLocalBounds().reduced(8);
    if (generalPage) generalPage->setBounds(contentBounds);
    if (audioPage) audioPage->setBounds(contentBounds);
    if (pluginPage) pluginPage->setBounds(contentBounds);
    if (appearancePage) appearancePage->setBounds(contentBounds);
    if (commandsPage) commandsPage->setBounds(contentBounds);
    if (advancedPage) advancedPage->setBounds(contentBounds);
}

void SettingsWindow::applySettings()
{
    bool pathChanged = (bufferedSettings.rootDataPath != Orion::SettingsManager::get().rootDataPath);
    bool pluginToggleChanged = (bufferedSettings.fluxShaperEnabled != Orion::SettingsManager::get().fluxShaperEnabled ||
                                bufferedSettings.prismStackEnabled != Orion::SettingsManager::get().prismStackEnabled);
    bool pluginCacheBehaviorChanged = (bufferedSettings.pluginCacheEnabled != Orion::SettingsManager::get().pluginCacheEnabled);

    if (pathChanged)
    {
        Orion::SettingsManager::setInstanceRoot(bufferedSettings.rootDataPath);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Restart Required",
            "You have changed the data folder. Please restart Orion for changes to take full effect.");
    }

    Orion::SettingsManager::set(bufferedSettings);

    if (pluginToggleChanged || pluginCacheBehaviorChanged)
    {
        Orion::PluginManager::scanPlugins(true);
    }

    initialTheme = bufferedSettings.theme;
    initialScale = bufferedSettings.uiScale;
}

void SettingsWindow::closeFromButtons(int modalResult)
{
    if (closeMode == CloseMode::Embedded)
        return;

    if (auto* dw = findParentComponentOfClass<juce::DialogWindow>())
    {
        dw->exitModalState(modalResult);
        return;
    }

    if (auto* w = findParentComponentOfClass<juce::DocumentWindow>())
        w->setVisible(false);
}

void SettingsWindow::styleNavButton(juce::TextButton& button, int pageIndex)
{
    const auto& ds = Orion::UI::DesignSystem::instance();
    const auto& colors = ds.currentTheme().colors;
    const int currentIndex = static_cast<int>(button.getProperties().getWithDefault("pageIndex", 0));
    const bool selected = currentIndex == pageIndex;
    button.setColour(juce::TextButton::buttonColourId,
                     selected ? colors.accent.withAlpha(0.45f)
                              : colors.panelBgAlt.withAlpha(0.6f));
    button.setColour(juce::TextButton::textColourOffId,
                     selected ? colors.textPrimary : colors.textSecondary);
}

void SettingsWindow::selectPage(int index)
{
    if (generalPage) generalPage->setVisible(index == 0);
    if (audioPage) audioPage->setVisible(index == 1);
    if (pluginPage) pluginPage->setVisible(index == 2);
    if (appearancePage) appearancePage->setVisible(index == 3);
    if (commandsPage) commandsPage->setVisible(index == 4);
    if (advancedPage) advancedPage->setVisible(index == 5);

    generalNavButton.getProperties().set("pageIndex", 0);
    audioNavButton.getProperties().set("pageIndex", 1);
    pluginsNavButton.getProperties().set("pageIndex", 2);
    appearanceNavButton.getProperties().set("pageIndex", 3);
    commandsNavButton.getProperties().set("pageIndex", 4);
    advancedNavButton.getProperties().set("pageIndex", 5);

    styleNavButton(generalNavButton, index);
    styleNavButton(audioNavButton, index);
    styleNavButton(pluginsNavButton, index);
    styleNavButton(appearanceNavButton, index);
    styleNavButton(commandsNavButton, index);
    styleNavButton(advancedNavButton, index);
}



SettingsWindow::GeneralPage::GeneralPage(Orion::AppSettings& s, std::function<void(const juce::String&)> callback)
    : settings(s), onThemeChangeCallback(callback)
{
    addAndMakeVisible(themeLabel);
    themeLabel.setText("Theme", juce::dontSendNotification);
    themeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(themeSelector);

    OrionLookAndFeel tempLnf;
    juce::StringArray themes = tempLnf.getAvailableThemes();

    std::string currentTheme = settings.theme;

    for (int i = 0; i < themes.size(); ++i)
    {
        themeSelector.addItem(themes[i], i + 1);
        if (themes[i] == juce::String(currentTheme))
            themeSelector.setSelectedId(i + 1, juce::dontSendNotification);
    }

    // Fallback if current theme not found (e.g. first run or invalid)
    if (themeSelector.getSelectedId() == 0 && themes.size() > 0)
        themeSelector.setSelectedId(1, juce::dontSendNotification);

    themeSelector.onChange = [this] {
        int id = themeSelector.getSelectedId();
        if (id > 0)
        {
            juce::String themeName = themeSelector.getItemText(id - 1);
            settings.theme = themeName.toStdString();
            settings.themeId = Orion::UiThemeRegistry::resolveThemeId(settings.theme);
            if (onThemeChangeCallback) onThemeChangeCallback(themeName);
        }
    };

    addAndMakeVisible(scaleLabel);
    scaleLabel.setText("UI Scale", juce::dontSendNotification);
    scaleLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(scaleSlider);
    scaleSlider.setRange(0.5, 2.0, 0.1);
    scaleSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    scaleSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    scaleSlider.setValue(settings.uiScale, juce::dontSendNotification);
    scaleSlider.onValueChange = [this] {
        settings.uiScale = (float)scaleSlider.getValue();
        juce::Desktop::getInstance().setGlobalScaleFactor(settings.uiScale);
    };

    addAndMakeVisible(autoSaveToggle);
    autoSaveToggle.setButtonText("Enable Auto-Save");
    autoSaveToggle.setToggleState(settings.autoSaveEnabled, juce::dontSendNotification);
    autoSaveToggle.onClick = [this] {
        settings.autoSaveEnabled = autoSaveToggle.getToggleState();
        autoSaveIntervalSlider.setEnabled(settings.autoSaveEnabled);
    };

    addAndMakeVisible(hoverPreviewToggle);
    hoverPreviewToggle.setButtonText("Enable Hover to Play Preview (Dashboard)");
    hoverPreviewToggle.setToggleState(settings.hoverPreviewEnabled, juce::dontSendNotification);
    hoverPreviewToggle.onClick = [this] {
        settings.hoverPreviewEnabled = hoverPreviewToggle.getToggleState();
    };

    addAndMakeVisible(returnToStartToggle);
    returnToStartToggle.setButtonText("Return to start position on stop");
    returnToStartToggle.setToggleState(settings.returnToStartOnStop, juce::dontSendNotification);
    returnToStartToggle.onClick = [this] {
        settings.returnToStartOnStop = returnToStartToggle.getToggleState();
    };

    addAndMakeVisible(autoSaveIntervalLabel);
    autoSaveIntervalLabel.setText("Interval (min)", juce::dontSendNotification);

    addAndMakeVisible(autoSaveIntervalSlider);
    autoSaveIntervalSlider.setRange(1.0, 60.0, 1.0);
    autoSaveIntervalSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    autoSaveIntervalSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    autoSaveIntervalSlider.setValue(settings.autoSaveInterval, juce::dontSendNotification);
    autoSaveIntervalSlider.setEnabled(settings.autoSaveEnabled);
    autoSaveIntervalSlider.onValueChange = [this] {
        settings.autoSaveInterval = (int)autoSaveIntervalSlider.getValue();
    };

    addAndMakeVisible(previewLengthLabel);
    previewLengthLabel.setText("Preview Length (s)", juce::dontSendNotification);

    addAndMakeVisible(previewLengthSlider);
    previewLengthSlider.setRange(1.0, 30.0, 0.5);
    previewLengthSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    previewLengthSlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 50, 20);
    previewLengthSlider.setValue(settings.previewLength, juce::dontSendNotification);
    previewLengthSlider.onValueChange = [this] {
        settings.previewLength = (float)previewLengthSlider.getValue();
    };

    addAndMakeVisible(contextualHintsToggle);
    contextualHintsToggle.setButtonText("Enable Contextual Hints");
    contextualHintsToggle.setToggleState(settings.hintPolicy.contextualHintsEnabled, juce::dontSendNotification);
    contextualHintsToggle.onClick = [this] {
        settings.hintPolicy.contextualHintsEnabled = contextualHintsToggle.getToggleState();
    };

    addAndMakeVisible(quickStartToggle);
    quickStartToggle.setButtonText("Enable Guided Start");
    quickStartToggle.setToggleState(settings.hintPolicy.quickStartEnabled, juce::dontSendNotification);
    quickStartToggle.onClick = [this] {
        settings.hintPolicy.quickStartEnabled = quickStartToggle.getToggleState();
    };

    addAndMakeVisible(shortcutsInHintsToggle);
    shortcutsInHintsToggle.setButtonText("Show Shortcuts In Hints");
    shortcutsInHintsToggle.setToggleState(settings.hintPolicy.showShortcutsInHints, juce::dontSendNotification);
    shortcutsInHintsToggle.onClick = [this] {
        settings.hintPolicy.showShortcutsInHints = shortcutsInHintsToggle.getToggleState();
    };

    addAndMakeVisible(pathHeaderLabel);
    pathHeaderLabel.setText("User Data Folder", juce::dontSendNotification);
    pathHeaderLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));

    addAndMakeVisible(currentPathDisplay);
    currentPathDisplay.setText(settings.rootDataPath, juce::dontSendNotification);
    currentPathDisplay.setColour(juce::Label::backgroundColourId, juce::Colours::black.withAlpha(0.2f));
    currentPathDisplay.setColour(juce::Label::outlineColourId, juce::Colours::white.withAlpha(0.1f));

    addAndMakeVisible(browsePathBtn);
    browsePathBtn.setButtonText("Browse...");
    browsePathBtn.onClick = [this] {
        auto chooser = std::make_shared<juce::FileChooser>("Select Data Folder", juce::File(settings.rootDataPath));
        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [this, chooser](const juce::FileChooser& fc) {
                if (fc.getURLResults().size() > 0) {
                    auto path = fc.getResult().getFullPathName().toStdString();
                    settings.rootDataPath = path;
                    currentPathDisplay.setText(path, juce::dontSendNotification);
                }
            });
    };
}

void SettingsWindow::GeneralPage::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void SettingsWindow::GeneralPage::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto row = [&](int h) { return area.removeFromTop(h); };
    auto space = [&](int h) { area.removeFromTop(h); };

    auto r1 = row(30);
    themeLabel.setBounds(r1.removeFromLeft(100));
    r1.removeFromLeft(10);
    themeSelector.setBounds(r1.removeFromLeft(220));

    space(10);

    auto r2 = row(30);
    scaleLabel.setBounds(r2.removeFromLeft(100));
    r2.removeFromLeft(10);
    scaleSlider.setBounds(r2.removeFromLeft(200));

    space(20);


    autoSaveToggle.setBounds(row(30));
    hoverPreviewToggle.setBounds(row(30));
    returnToStartToggle.setBounds(row(30));

    auto r3 = row(30);
    autoSaveIntervalLabel.setBounds(r3.removeFromLeft(120));
    r3.removeFromLeft(10);
    autoSaveIntervalSlider.setBounds(r3.removeFromLeft(200));

    auto r3b = row(30);
    previewLengthLabel.setBounds(r3b.removeFromLeft(120));
    r3b.removeFromLeft(10);
    previewLengthSlider.setBounds(r3b.removeFromLeft(200));

    space(8);
    contextualHintsToggle.setBounds(row(26));
    quickStartToggle.setBounds(row(26));
    shortcutsInHintsToggle.setBounds(row(26));

    space(20);


    pathHeaderLabel.setBounds(row(30));

    auto r4 = row(30);
    browsePathBtn.setBounds(r4.removeFromRight(100));
    r4.removeFromRight(10);
    currentPathDisplay.setBounds(r4);
}



SettingsWindow::PluginPage::PluginPage(Orion::AppSettings& s)
    : settings(s)
{
    addAndMakeVisible(headerLabel);
    headerLabel.setText("Plugin Search Paths", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

    addAndMakeVisible(pathList);
    pathList.setModel(this);
    pathList.setMultipleSelectionEnabled(true);
    pathList.setColour(juce::ListBox::backgroundColourId, juce::Colours::black.withAlpha(0.2f));
    pathList.setOutlineThickness(1);
    pathList.setColour(juce::ListBox::outlineColourId, juce::Colours::white.withAlpha(0.1f));

    addAndMakeVisible(addButton);
    addButton.setButtonText("Add Folder");
    addButton.onClick = [this] { addPath(); };

    addAndMakeVisible(removeButton);
    removeButton.setButtonText("Remove Selected");
    removeButton.onClick = [this] { removePath(); };

    addAndMakeVisible(rescanButton);
    rescanButton.setButtonText("Rescan All");
    rescanButton.onClick = [this] { rescan(); };

    addAndMakeVisible(clearCacheButton);
    clearCacheButton.setButtonText("Clear Cache");
    clearCacheButton.onClick = [this] {
        if (Orion::PluginManager::clearCache()) {
            updateList();
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Plugin Cache", "Plugin cache cleared.");
        } else {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon, "Plugin Cache", "Failed to clear plugin cache.");
        }
    };

    addAndMakeVisible(scanOnStartupToggle);
    scanOnStartupToggle.setButtonText("Scan Plugins on Startup");
    scanOnStartupToggle.setToggleState(settings.scanPluginsAtStartup, juce::dontSendNotification);
    scanOnStartupToggle.onClick = [this] {
        settings.scanPluginsAtStartup = scanOnStartupToggle.getToggleState();
    };

    addAndMakeVisible(cachePathLabel);
    cachePathLabel.setFont(juce::FontOptions(12.0f));
    cachePathLabel.setColour(juce::Label::textColourId, juce::Colours::grey);
    cachePathLabel.setText("Cache: " + juce::String(Orion::PluginManager::getCacheFilePath()), juce::dontSendNotification);

    addAndMakeVisible(cacheStatsLabel);
    cacheStatsLabel.setFont(juce::FontOptions(12.0f));
    cacheStatsLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    updateList();
}

void SettingsWindow::PluginPage::paint(juce::Graphics& g) { juce::ignoreUnused(g); }

void SettingsWindow::PluginPage::resized()
{
    auto area = getLocalBounds().reduced(20);
    headerLabel.setBounds(area.removeFromTop(40));

    auto buttons = area.removeFromBottom(40);
    addButton.setBounds(buttons.removeFromLeft(100).reduced(0, 5));
    buttons.removeFromLeft(10);
    removeButton.setBounds(buttons.removeFromLeft(140).reduced(0, 5));
    buttons.removeFromLeft(10);
    clearCacheButton.setBounds(buttons.removeFromLeft(120).reduced(0, 5));

    rescanButton.setBounds(buttons.removeFromRight(120).reduced(0, 5));

    cacheStatsLabel.setBounds(area.removeFromBottom(24));
    cachePathLabel.setBounds(area.removeFromBottom(24));
    scanOnStartupToggle.setBounds(area.removeFromBottom(30));
    area.removeFromBottom(10);
    pathList.setBounds(area);
}

int SettingsWindow::PluginPage::getNumRows()
{
    return (int)settings.vst3Paths.size();
}

void SettingsWindow::PluginPage::paintListBoxItem(int row, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    auto& paths = settings.vst3Paths;
    if (row < 0 || row >= (int)paths.size()) return;

    if (rowIsSelected) {
        g.fillAll(findColour(juce::TextEditor::highlightColourId));
    }

    g.setColour(findColour(juce::ListBox::textColourId));
    g.setFont(14.0f);
    g.drawText(paths[row], 5, 0, width - 5, height, juce::Justification::centredLeft, true);
}

void SettingsWindow::PluginPage::updateList()
{
    pathList.updateContent();
    auto stats = Orion::PluginManager::getCacheStats();
    int okPaths = 0;
    for (const auto& p : settings.vst3Paths) {
        if (juce::File(p).isDirectory()) ++okPaths;
    }
    cacheStatsLabel.setText(
        "Cache records: " + juce::String(stats.recordCount) +
        " | plugins: " + juce::String(stats.pluginCount) +
        " | hits: " + juce::String(stats.hitCount) +
        " | misses: " + juce::String(stats.missCount) +
        " | path health: " + juce::String(okPaths) + "/" + juce::String((int)settings.vst3Paths.size()),
        juce::dontSendNotification);
}

void SettingsWindow::PluginPage::addPath()
{
    auto chooser = std::make_shared<juce::FileChooser>("Select VST3 Folder", juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this, chooser](const juce::FileChooser& fc)
        {
            if (fc.getURLResults().size() > 0)
            {
                auto path = fc.getResult().getFullPathName().toStdString();
                bool exists = false;
                for (const auto& p : settings.vst3Paths) if (p == path) exists = true;
                if (!exists) {
                    settings.vst3Paths.push_back(path);
                    updateList();
                }
            }
        });
}

void SettingsWindow::PluginPage::removePath()
{
    auto selectedRows = pathList.getSelectedRows();
    if (selectedRows.size() > 0)
    {
        for (int i = selectedRows.size() - 1; i >= 0; --i)
        {
            int row = selectedRows[i];
            if (row >= 0 && row < (int)settings.vst3Paths.size()) settings.vst3Paths.erase(settings.vst3Paths.begin() + row);
        }
        updateList();
    }
}

void SettingsWindow::PluginPage::rescan()
{
    Orion::PluginManager::scanPlugins(true);
}



SettingsWindow::AdvancedPage::AdvancedPage(Orion::AppSettings& s)
    : settings(s)
{
    addAndMakeVisible(devModeToggle);
    devModeToggle.setButtonText("Enable Developer Mode");
    devModeToggle.setToggleState(settings.developerMode, juce::dontSendNotification);
    devModeToggle.onClick = [this] { settings.developerMode = devModeToggle.getToggleState(); };

    addAndMakeVisible(devDescriptionLabel);
    devDescriptionLabel.setText("Enable this to create and manage Extensions for Orion using Lua.", juce::dontSendNotification);
    devDescriptionLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(warningLabel);
    warningLabel.setText("Experimental features can be unstable.", juce::dontSendNotification);
    warningLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
    warningLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(expFeaturesToggle);
    expFeaturesToggle.setButtonText("Enable Experimental Features");
    expFeaturesToggle.setToggleState(settings.experimentalFeatures, juce::dontSendNotification);
    expFeaturesToggle.onClick = [this] { settings.experimentalFeatures = expFeaturesToggle.getToggleState(); };

    addAndMakeVisible(presetMonitoringToggle);
    presetMonitoringToggle.setButtonText("Enable Preset Monitoring");
    presetMonitoringToggle.setToggleState(settings.presetMonitoringEnabled, juce::dontSendNotification);
    presetMonitoringToggle.onClick = [this] { settings.presetMonitoringEnabled = presetMonitoringToggle.getToggleState(); };

    addAndMakeVisible(fluxShaperToggle);
    fluxShaperToggle.setButtonText("Enable Flux Shaper");
    fluxShaperToggle.setToggleState(settings.fluxShaperEnabled, juce::dontSendNotification);
    fluxShaperToggle.onClick = [this] { settings.fluxShaperEnabled = fluxShaperToggle.getToggleState(); };

    addAndMakeVisible(prismStackToggle);
    prismStackToggle.setButtonText("Enable Prism Stack");
    prismStackToggle.setToggleState(settings.prismStackEnabled, juce::dontSendNotification);
    prismStackToggle.onClick = [this] { settings.prismStackEnabled = prismStackToggle.getToggleState(); };

    addAndMakeVisible(pluginCacheToggle);
    pluginCacheToggle.setButtonText("Enable Plugin Cache");
    pluginCacheToggle.setToggleState(settings.pluginCacheEnabled, juce::dontSendNotification);
    pluginCacheToggle.onClick = [this] { settings.pluginCacheEnabled = pluginCacheToggle.getToggleState(); };

    addAndMakeVisible(startupVerifyToggle);
    startupVerifyToggle.setButtonText("Verify Plugin Cache in Background at Startup");
    startupVerifyToggle.setToggleState(settings.pluginBackgroundVerificationOnStartup, juce::dontSendNotification);
    startupVerifyToggle.onClick = [this] { settings.pluginBackgroundVerificationOnStartup = startupVerifyToggle.getToggleState(); };

    addAndMakeVisible(extensionApiExplorerToggle);
    extensionApiExplorerToggle.setButtonText("Extensions IDE: Show API Explorer");
    extensionApiExplorerToggle.setToggleState(settings.extensionIdeShowApiExplorer, juce::dontSendNotification);
    extensionApiExplorerToggle.onClick = [this] { settings.extensionIdeShowApiExplorer = extensionApiExplorerToggle.getToggleState(); };

    addAndMakeVisible(extensionDictionaryToggle);
    extensionDictionaryToggle.setButtonText("Extensions IDE: Show Dictionary");
    extensionDictionaryToggle.setToggleState(settings.extensionIdeShowDictionary, juce::dontSendNotification);
    extensionDictionaryToggle.onClick = [this] { settings.extensionIdeShowDictionary = extensionDictionaryToggle.getToggleState(); };

    addAndMakeVisible(extensionProblemsToggle);
    extensionProblemsToggle.setButtonText("Extensions IDE: Show Problems Pane");
    extensionProblemsToggle.setToggleState(settings.extensionIdeShowProblems, juce::dontSendNotification);
    extensionProblemsToggle.onClick = [this] { settings.extensionIdeShowProblems = extensionProblemsToggle.getToggleState(); };

    addAndMakeVisible(luaLsPathLabel);
    luaLsPathLabel.setText("Lua LS executable (optional override):", juce::dontSendNotification);
    luaLsPathLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(luaLsPathEditor);
    luaLsPathEditor.setText(settings.luaLsExecutablePath, juce::dontSendNotification);
    luaLsPathEditor.onTextChange = [this] { settings.luaLsExecutablePath = luaLsPathEditor.getText().toStdString(); };

    addAndMakeVisible(maintenanceHeaderLabel);
    maintenanceHeaderLabel.setText("Maintenance", juce::dontSendNotification);
    maintenanceHeaderLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));

    addAndMakeVisible(statusLabel);
    statusLabel.setText("Use these actions to recover scans or reinstall bundled extensions.", juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId, juce::Colours::grey);

    addAndMakeVisible(reinstallExtensionsBtn);
    reinstallExtensionsBtn.setButtonText("Reinstall Default Extensions");
    reinstallExtensionsBtn.onClick = [] {
        Orion::ExtensionManager::getInstance().installFactoryExtensions(true);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Maintenance", "Default extensions have been reinstalled.");
    };

    addAndMakeVisible(rescanPluginsBtn);
    rescanPluginsBtn.setButtonText("Force Rescan Plugins");
    rescanPluginsBtn.onClick = [] {
        Orion::PluginManager::scanPlugins(true);
        juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::InfoIcon, "Maintenance", "Plugin rescan started in the background.");
    };
}

void SettingsWindow::AdvancedPage::paint(juce::Graphics& g) { juce::ignoreUnused(g); }

void SettingsWindow::AdvancedPage::resized()
{
    auto area = getLocalBounds().reduced(20);
    devModeToggle.setBounds(area.removeFromTop(28));
    devDescriptionLabel.setBounds(area.removeFromTop(26));
    area.removeFromTop(14);
    warningLabel.setBounds(area.removeFromTop(26));
    expFeaturesToggle.setBounds(area.removeFromTop(26));
    presetMonitoringToggle.setBounds(area.removeFromTop(26));
    fluxShaperToggle.setBounds(area.removeFromTop(26));
    prismStackToggle.setBounds(area.removeFromTop(26));
    pluginCacheToggle.setBounds(area.removeFromTop(26));
    startupVerifyToggle.setBounds(area.removeFromTop(26));
    extensionApiExplorerToggle.setBounds(area.removeFromTop(26));
    extensionDictionaryToggle.setBounds(area.removeFromTop(26));
    extensionProblemsToggle.setBounds(area.removeFromTop(26));
    luaLsPathLabel.setBounds(area.removeFromTop(20));
    luaLsPathEditor.setBounds(area.removeFromTop(28));
    area.removeFromTop(16);
    maintenanceHeaderLabel.setBounds(area.removeFromTop(28));
    statusLabel.setBounds(area.removeFromTop(24));
    area.removeFromTop(10);
    reinstallExtensionsBtn.setBounds(area.removeFromTop(30).removeFromLeft(250));
    area.removeFromTop(8);
    rescanPluginsBtn.setBounds(area.removeFromTop(30).removeFromLeft(250));
}

SettingsWindow::CommandsPage::CommandsPage(Orion::AppSettings& s)
    : settings(s)
{
    addAndMakeVisible(headerLabel);
    headerLabel.setText("Commands & Controls", juce::dontSendNotification);
    headerLabel.setFont(juce::FontOptions(18.0f, juce::Font::bold));

    addAndMakeVisible(conflictLabel);
    conflictLabel.setColour(juce::Label::textColourId, juce::Colours::orange.withAlpha(0.9f));
    conflictLabel.setText("", juce::dontSendNotification);

    addAndMakeVisible(filterBox);
    filterBox.setTextToShowWhenEmpty("Search actions...", juce::Colours::grey);
    filterBox.onTextChange = [this] { applyFilter(); };

    addAndMakeVisible(listViewport);
    listViewport.setViewedComponent(&listContent, false);
    listViewport.setScrollBarsShown(true, false);

    buildRows();
    applyFilter();
}

void SettingsWindow::CommandsPage::buildRows()
{
    struct Def { const char* id; const char* title; const char* def; };
    const std::vector<Def> defs = {
        {"play_stop", "Play / Pause", "Space"},
        {"stop_transport", "Stop", "Esc"},
        {"record_transport", "Record", "R"},
        {"rewind_transport", "Rewind", ","},
        {"fast_forward_transport", "Fast Forward", "."},
        {"panic_transport", "Panic (All Notes Off)", "Shift+P"},
        {"show_command_palette", "Show Command Palette", "Ctrl+K"},
        {"show_guided_start", "Quick Start Setup", "Ctrl+Shift+G"},
        {"open_shortcut_cheatsheet", "Open Shortcut Cheatsheet", "Ctrl+/"},
        {"save_project", "Save Project", "Ctrl+S"},
        {"load_project", "Open Project", "Ctrl+O"},
        {"add_instrument_track", "Add Instrument Track", "Ctrl+I"},
        {"add_audio_track", "Add Audio Track", "Ctrl+T"},
        {"add_bus_track", "Add Bus Track", "Ctrl+B"},
        {"export_audio", "Export Audio", "Ctrl+E"},
        {"undo_action", "Undo", "Ctrl+Z"},
        {"redo_action", "Redo", "Ctrl+Shift+Z"},
        {"delete_selection", "Delete Selection", "Delete"},
        {"toggle_loop", "Toggle Loop", "/"},
        {"toggle_click", "Toggle Metronome", "C"},
        {"tool_select", "Tool: Select", "1"},
        {"tool_draw", "Tool: Draw", "2"},
        {"tool_erase", "Tool: Erase", "3"},
        {"tool_split", "Tool: Split", "4"},
        {"tool_glue", "Tool: Glue", "5"},
        {"tool_zoom", "Tool: Zoom", "6"},
        {"tool_mute", "Tool: Mute", "7"},
        {"tool_listen", "Tool: Listen", "8"},
        {"show_settings", "Open Settings", "Ctrl+P"},
        {"toggle_panel_inspector", "Toggle Inspector Panel", "Alt+1"},
        {"toggle_panel_mixer", "Toggle Mixer Panel", "Alt+2"},
        {"toggle_panel_piano_roll", "Toggle Piano Roll Panel", "Alt+3"},
        {"toggle_panel_browser", "Toggle Browser Panel", "Alt+4"},
        {"workspace_compose", "Workspace Preset: Compose", "Ctrl+1"},
        {"workspace_arrange", "Workspace Preset: Arrange", "Ctrl+2"},
        {"workspace_mix", "Workspace Preset: Mix", "Ctrl+3"}
    };

    rows.clear();
    listContent.removeAllChildren();

    for (const auto& d : defs)
    {
        ActionRow row;
        row.id = d.id;
        row.title = d.title;
        row.defaultShortcut = d.def;
        row.label = std::make_unique<juce::Label>();
        row.editor = std::make_unique<juce::TextEditor>();

        row.label->setText(row.title + " (default: " + row.defaultShortcut + ")", juce::dontSendNotification);
        row.label->setJustificationType(juce::Justification::centredLeft);
        listContent.addAndMakeVisible(row.label.get());

        auto existing = settings.shortcutOverrides.find(row.id);
        juce::String value = existing != settings.shortcutOverrides.end()
            ? juce::String(existing->second)
            : row.defaultShortcut;
        row.editor->setText(value, juce::dontSendNotification);
        row.editor->setTooltip("Format examples: Ctrl+K, Alt+2, Shift+R, /");
        row.editor->onTextChange = [this, id = row.id, editor = row.editor.get()] {
            settings.shortcutOverrides[id] = editor->getText().trim().toStdString();
            updateConflicts();
        };
        listContent.addAndMakeVisible(row.editor.get());

        rows.push_back(std::move(row));
    }

    updateConflicts();
}

void SettingsWindow::CommandsPage::updateConflicts()
{
    std::map<juce::String, int> counts;
    for (auto& row : rows) {
        auto key = row.editor->getText().trim().toLowerCase();
        if (!key.isEmpty()) counts[key]++;
    }

    int conflicts = 0;
    for (auto& row : rows) {
        auto key = row.editor->getText().trim().toLowerCase();
        const bool dupe = !key.isEmpty() && counts[key] > 1;
        row.editor->setColour(juce::TextEditor::backgroundColourId,
                              dupe ? juce::Colour(0x66AA2222) : juce::Colours::black.withAlpha(0.2f));
        if (dupe) ++conflicts;
    }

    if (conflicts > 0)
        conflictLabel.setText("Conflicts detected. Duplicate shortcuts are highlighted.", juce::dontSendNotification);
    else
        conflictLabel.setText("No conflicts.", juce::dontSendNotification);
}

void SettingsWindow::CommandsPage::applyFilter()
{
    auto filter = filterBox.getText().trim().toLowerCase();
    int y = 0;
    const int rowH = 52;

    for (auto& row : rows)
    {
        auto hay = (row.title + " " + juce::String(row.id) + " " + row.defaultShortcut).toLowerCase();
        const bool show = filter.isEmpty() || hay.contains(filter);

        row.label->setVisible(show);
        row.editor->setVisible(show);
        if (!show) continue;

        row.label->setBounds(10, y, 520, 22);
        row.editor->setBounds(10, y + 24, 220, 24);
        y += rowH;
    }

    listContent.setSize(560, std::max(1, y));
}

void SettingsWindow::CommandsPage::paint(juce::Graphics& g)
{
    juce::ignoreUnused(g);
}

void SettingsWindow::CommandsPage::resized()
{
    auto area = getLocalBounds().reduced(20);
    headerLabel.setBounds(area.removeFromTop(30));
    conflictLabel.setBounds(area.removeFromTop(22));
    area.removeFromTop(8);
    filterBox.setBounds(area.removeFromTop(28));
    area.removeFromTop(8);
    listViewport.setBounds(area);
}



SettingsWindow::AppearancePage::AppearancePage(Orion::AppSettings& s, std::function<void(const juce::String&)> themeCallback)
    : settings(s), onThemeChangeCallback(themeCallback)
{
    addAndMakeVisible(themeLabel);
    themeLabel.setText("Active Theme", juce::dontSendNotification);
    themeLabel.setFont(juce::FontOptions(14.0f, juce::Font::bold));

    addAndMakeVisible(themeSelector);
    OrionLookAndFeel tempLnf;
    juce::StringArray themes = tempLnf.getAvailableThemes();

    for (auto it = settings.customThemes.begin(); it != settings.customThemes.end(); ++it) {
        if (!themes.contains(it.key())) themes.add(it.key());
    }

    std::string currentTheme = settings.theme;

    for (int i = 0; i < themes.size(); ++i) {
        themeSelector.addItem(themes[i], i + 1);
        if (themes[i] == juce::String(currentTheme))
            themeSelector.setSelectedId(i + 1, juce::dontSendNotification);
    }

    themeSelector.onChange = [this] {
        int id = themeSelector.getSelectedId();
        if (id > 0) {
            juce::String themeName = themeSelector.getItemText(id - 1);
            settings.theme = themeName.toStdString();
            settings.themeId = Orion::UiThemeRegistry::resolveThemeId(settings.theme);
            if (onThemeChangeCallback) onThemeChangeCallback(themeName);
        }
    };

    addAndMakeVisible(fullColorToggle);
    fullColorToggle.setButtonText("Full Track Coloring (Headers & Mixer)");
    fullColorToggle.setToggleState(settings.fullTrackColoring, juce::dontSendNotification);
    fullColorToggle.onClick = [this] {
        settings.fullTrackColoring = fullColorToggle.getToggleState();
        // Force UI refresh if needed
        if (onThemeChangeCallback) onThemeChangeCallback(settings.theme);
    };

    addAndMakeVisible(colorLabel);
    colorLabel.setText("Accent Color", juce::dontSendNotification);

    addAndMakeVisible(accentColorPreview);
    auto accent = OrionLookAndFeel().findColour(OrionLookAndFeel::accentColourId);
    accentColorPreview.setColour(juce::Label::backgroundColourId, accent);
    accentColorPreview.setTooltip("Click to change accent color");
    accentColorPreview.addMouseListener(this, false);

    addAndMakeVisible(saveThemeBtn);
    saveThemeBtn.setButtonText("Save as Custom Theme");
    saveThemeBtn.onClick = [this] {
        auto* alert = new juce::AlertWindow("Save Custom Theme", "Enter a name for your theme:", juce::MessageBoxIconType::NoIcon);
        alert->addTextEditor("name", "My Custom Theme");
        alert->addButton("OK", 1);
        alert->addButton("Cancel", 0);

        alert->enterModalState(true, juce::ModalCallbackFunction::create([this, alert](int result) {
            if (result == 1) {
                juce::String name = alert->getTextEditorContents("name");
                if (name.isNotEmpty()) {
                    // Capture current palette state
                    OrionLookAndFeel lnf;
                    lnf.setTheme(juce::String(settings.theme));
                    auto palette = lnf.getCurrentPalette();

                    // Update palette with current custom accent
                    palette.accent = accentColorPreview.findColour(juce::Label::backgroundColourId);

                    nlohmann::json themeData;
                    themeData["accent"] = palette.accent.toString().toStdString();
                    themeData["chassis"] = palette.chassis.toString().toStdString();
                    themeData["panel"] = palette.panel.toString().toStdString();

                    settings.customThemes[name.toStdString()] = themeData;
                    settings.theme = name.toStdString();
                    settings.themeId = Orion::UiThemeRegistry::resolveThemeId(settings.theme);

                    if (onThemeChangeCallback) onThemeChangeCallback(name);
                }
            }
            delete alert;
        }));
    };
}

void SettingsWindow::AppearancePage::paint(juce::Graphics& g) { juce::ignoreUnused(g); }

void SettingsWindow::AppearancePage::resized()
{
    auto area = getLocalBounds().reduced(20);
    auto row = [&](int h) { return area.removeFromTop(h); };

    auto r1 = row(30);
    themeLabel.setBounds(r1.removeFromLeft(120));
    themeSelector.setBounds(r1.removeFromLeft(200));

    area.removeFromTop(20);
    fullColorToggle.setBounds(row(30));

    area.removeFromTop(20);
    auto r2 = row(30);
    colorLabel.setBounds(r2.removeFromLeft(120));
    accentColorPreview.setBounds(r2.removeFromLeft(60));

    area.removeFromTop(40);
    saveThemeBtn.setBounds(row(30).removeFromLeft(200));
}

void SettingsWindow::AppearancePage::openColorPicker()
{
    auto chooser = std::make_unique<juce::ColourSelector>();
    chooser->setSize(300, 400);
    chooser->setCurrentColour(accentColorPreview.findColour(juce::Label::backgroundColourId));
    chooser->addChangeListener(this);

    juce::CallOutBox::launchAsynchronously(std::move(chooser), accentColorPreview.getScreenBounds(), nullptr);
}

void SettingsWindow::AppearancePage::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (auto* selector = dynamic_cast<juce::ColourSelector*>(source))
    {
        accentColorPreview.setColour(juce::Label::backgroundColourId, selector->getCurrentColour());
        accentColorPreview.repaint();
    }
}

void SettingsWindow::AppearancePage::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == &accentColorPreview)
    {
        openColorPicker();
    }
}
