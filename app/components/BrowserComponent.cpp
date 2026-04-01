#include "BrowserComponent.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"
#include "orion/SettingsManager.h"

// =================================================================================================
// SIDEBAR COMPONENT
// =================================================================================================

BrowserComponent::SidebarComponent::SidebarComponent()
{
    listBox.setModel(this);
    listBox.setRowHeight(36); // Taller rows for better touch/click targets
    listBox.setOutlineThickness(0);
    listBox.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(listBox);

    refresh();
}

BrowserComponent::SidebarComponent::~SidebarComponent() {}

void BrowserComponent::SidebarComponent::refresh()
{
    places.clear();

    // Section Header
    places.push_back({ "PLACES", juce::File(), "", false, true });

    juce::File root = juce::File::getSpecialLocation(juce::File::userHomeDirectory).getParentDirectory().getParentDirectory();
    #if JUCE_WINDOWS
    root = juce::File("C:\\");
    #else
    root = juce::File("/");
    #endif

    places.push_back({ "Computer", root, "Root" });
    places.push_back({ "Home", juce::File::getSpecialLocation(juce::File::userHomeDirectory), "Home" });
    places.push_back({ "Desktop", juce::File::getSpecialLocation(juce::File::userDesktopDirectory), "Desktop" });
    places.push_back({ "Documents", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "Documents" });
    places.push_back({ "Music", juce::File::getSpecialLocation(juce::File::userMusicDirectory), "Music" });
    places.push_back({ "Downloads", juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("Downloads"), "Downloads" });

    // Section Header
    places.push_back({ "PINNED", juce::File(), "", false, true });

    auto pinned = Orion::SettingsManager::get().pinnedLocations;
    for (const auto& path : pinned) {
        juce::File f(path);
        if (f.exists()) {
            places.push_back({ f.getFileName(), f, "Folder", true });
        }
    }

    places.push_back({ "+ Add Folder...", juce::File(), "Add", false });

    listBox.updateContent();
}

void BrowserComponent::SidebarComponent::addPin(const juce::File& folder)
{
    if (!folder.isDirectory()) return;

    auto settings = Orion::SettingsManager::get();
    std::string path = folder.getFullPathName().toStdString();

    for (const auto& p : settings.pinnedLocations) {
        if (p == path) return;
    }

    settings.pinnedLocations.push_back(path);
    Orion::SettingsManager::set(settings);
    refresh();
}

void BrowserComponent::SidebarComponent::removePin(const juce::String& path)
{
    auto settings = Orion::SettingsManager::get();
    auto& pins = settings.pinnedLocations;

    pins.erase(std::remove(pins.begin(), pins.end(), path.toStdString()), pins.end());

    Orion::SettingsManager::set(settings);
    refresh();
}

void BrowserComponent::SidebarComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::browserSidebarBackgroundColourId));

    // Right border
    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawRect(getLocalBounds().removeFromRight(1));
}

void BrowserComponent::SidebarComponent::resized()
{
    listBox.setBounds(getLocalBounds());
}

int BrowserComponent::SidebarComponent::getNumRows()
{
    return (int)places.size();
}

void BrowserComponent::SidebarComponent::paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected)
{
    if (rowNumber >= (int)places.size()) return;
    const auto& item = places[rowNumber];

    if (item.isHeader)
    {
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.4f));
        g.setFont(juce::FontOptions(11.0f, juce::Font::bold));
        g.drawText(item.name, 16, 0, width - 16, height, juce::Justification::centredLeft);
        return;
    }

    if (rowIsSelected) {
        g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.2f));
        g.fillRect(0, 0, width, height);

        g.setColour(findColour(OrionLookAndFeel::accentColourId));
        g.fillRect(0, 0, 3, height); // Selection indicator bar
    }

    // Icon handling
    float iconX = 16.0f;
    float textX = 40.0f;

    if (item.icon == "Add") {
         g.setColour(findColour(juce::Label::textColourId).withAlpha(0.5f));
         g.setFont(13.0f);
         g.drawText(item.name, 20, 0, width - 20, height, juce::Justification::centredLeft);
         return;
    }

    juce::Colour iconCol = findColour(juce::Label::textColourId).withAlpha(0.7f);
    if (item.isPinned) iconCol = juce::Colours::orange;
    if (rowIsSelected) iconCol = findColour(OrionLookAndFeel::accentColourId);

    g.setColour(iconCol);

    juce::Path iconPath;
    if (item.icon == "Folder" || item.isPinned) iconPath = OrionIcons::getFolderIcon();
    else if (item.icon == "Root") iconPath = OrionIcons::getBrowserIcon();
    else if (item.icon == "Home") iconPath = OrionIcons::getHomeIcon();
    else if (item.icon == "Desktop") iconPath = OrionIcons::getBrowserIcon();
    else if (item.icon == "Documents") iconPath = OrionIcons::getDocumentationIcon();
    else if (item.icon == "Music") iconPath = OrionIcons::getPianoRollIcon();
    else if (item.icon == "Downloads") iconPath = OrionIcons::getImportIcon();
    else iconPath = OrionIcons::getClockIcon();

    auto iconBounds = juce::Rectangle<float>(iconX, (height - 14.0f) * 0.5f, 14.0f, 14.0f);
    g.fillPath(iconPath, iconPath.getTransformToScaleToFit(iconBounds, true));

    g.setColour(rowIsSelected ? findColour(juce::Label::textColourId) : findColour(juce::Label::textColourId).withAlpha(0.9f));
    g.setFont(14.0f);
    g.drawText(item.name, (int)textX, 0, width - (int)textX, height, juce::Justification::centredLeft);
}

void BrowserComponent::SidebarComponent::selectedRowsChanged(int lastRowSelected)
{
    if (lastRowSelected >= 0 && lastRowSelected < (int)places.size()) {
        const auto& item = places[lastRowSelected];
        if (!item.isHeader && item.location.exists()) {
             if (onPlaceSelected) onPlaceSelected(item.location);
        }
    }
}

void BrowserComponent::SidebarComponent::listBoxItemClicked(int row, const juce::MouseEvent& e)
{
    if (row < 0 || row >= (int)places.size()) return;
    const auto& item = places[row];

    if (item.icon == "Add") {
        auto chooser = std::make_shared<juce::FileChooser>("Select Folder to Pin", juce::File::getSpecialLocation(juce::File::userHomeDirectory));
        auto safeThis = juce::Component::SafePointer<SidebarComponent>(this);

        chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [safeThis, chooser](const juce::FileChooser& fc)
            {
                if (safeThis != nullptr && fc.getResults().size() > 0)
                {
                    safeThis->addPin(fc.getResult());
                }
            });
        return;
    }

    if (e.mods.isPopupMenu() && item.isPinned) {
        juce::PopupMenu m;
        m.addItem(1, "Remove Pin");
        m.showMenuAsync(juce::PopupMenu::Options(), [this, item](int result) {
            if (result == 1) {
                removePin(item.location.getFullPathName());
            }
        });
    }
}


// =================================================================================================
// PREVIEW PANEL
// =================================================================================================

BrowserComponent::PreviewPanel::PreviewPanel(BrowserComponent& o) : owner(o)
{
    // Styling the play button
    auto playPath = OrionIcons::getPlayIcon();
    playButton.setShape(playPath, true, true, false);

    // Will set colors in paint or dynamically
    playButton.onClick = [this] {
        if (owner.transportSource.isPlaying()) owner.transportSource.stop();
        else owner.transportSource.start();
    };
    addAndMakeVisible(playButton);

    autoPlayButton.setButtonText("Auto");
    autoPlayButton.setToggleState(true, juce::dontSendNotification);
    addAndMakeVisible(autoPlayButton);

    volumeSlider.setSliderStyle(juce::Slider::LinearBar);
    volumeSlider.setRange(0.0, 1.0);
    volumeSlider.setValue(0.8);
    volumeSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    volumeSlider.onValueChange = [this] {
        owner.transportSource.setGain((float)volumeSlider.getValue());
    };
    addAndMakeVisible(volumeSlider);

    filenameLabel.setFont(juce::FontOptions(13.0f, juce::Font::bold));
    filenameLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(filenameLabel);

    owner.thumbnail.addChangeListener(this);
    startTimer(30);
}

BrowserComponent::PreviewPanel::~PreviewPanel()
{
    owner.thumbnail.removeChangeListener(this);
}

void BrowserComponent::PreviewPanel::changeListenerCallback(juce::ChangeBroadcaster* source)
{
    if (source == &owner.thumbnail) repaint();
}

void BrowserComponent::PreviewPanel::timerCallback()
{
    if (owner.transportSource.isPlaying()) repaint();
}

void BrowserComponent::PreviewPanel::setFile(const juce::File& file)
{
    filenameLabel.setText(file.getFileName(), juce::dontSendNotification);
    owner.loadFileForPreview(file);

    if (autoPlayButton.getToggleState()) {
        owner.transportSource.setPosition(0.0);
        owner.transportSource.start();
    }
}

void BrowserComponent::PreviewPanel::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::browserPreviewBackgroundColourId));

    // Separator line
    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawLine(0, 0, (float)getWidth(), 0);

    auto area = getLocalBounds().reduced(8);

    // Bottom controls
    auto controlsArea = area.removeFromBottom(24);

    // Waveform Area
    auto waveArea = area.reduced(0, 4);

    // Draw background for waveform
    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(waveArea.toFloat(), 4.0f);

    g.setColour(findColour(OrionLookAndFeel::accentColourId));

    if (owner.thumbnail.getTotalLength() > 0.0)
    {
        owner.thumbnail.drawChannels(g, waveArea, 0.0, owner.thumbnail.getTotalLength(), 1.0f);

        if (owner.transportSource.getLengthInSeconds() > 0)
        {
            double pos = owner.transportSource.getCurrentPosition();
            double len = owner.transportSource.getLengthInSeconds();
            float x = waveArea.getX() + (float)((pos / len) * waveArea.getWidth());

            g.setColour(juce::Colours::white);
            g.drawLine(x, (float)waveArea.getY(), x, (float)waveArea.getBottom(), 1.5f);
        }
    }
    else
    {
        g.setColour(findColour(juce::Label::textColourId).withAlpha(0.5f));
        g.drawText("No Audio Selected", waveArea, juce::Justification::centred, true);
    }

    // Update button colors based on lookandfeel
    playButton.setColours(findColour(OrionLookAndFeel::accentColourId), juce::Colours::white, findColour(OrionLookAndFeel::accentColourId));
    autoPlayButton.setColour(juce::ToggleButton::textColourId, findColour(juce::Label::textColourId));
    autoPlayButton.setColour(juce::ToggleButton::tickColourId, findColour(OrionLookAndFeel::accentColourId));
    filenameLabel.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId));
}

void BrowserComponent::PreviewPanel::resized()
{
    auto area = getLocalBounds().reduced(8);
    auto controls = area.removeFromBottom(24);

    // Layout controls
    playButton.setBounds(controls.removeFromLeft(24));
    controls.removeFromLeft(8);

    autoPlayButton.setBounds(controls.removeFromLeft(50));
    controls.removeFromLeft(8);

    volumeSlider.setBounds(controls.removeFromRight(60));

    filenameLabel.setBounds(controls.reduced(4, 0));
}


// =================================================================================================
// BROWSER COMPONENT
// =================================================================================================

BrowserComponent::BrowserComponent()
    : thumbnail(512, formatManager, thumbnailCache)
{
    formatManager.registerBasicFormats();

    searchBar.setTextToShowWhenEmpty("Search...", juce::Colours::grey);
    searchBar.setJustification(juce::Justification::centredLeft);
    searchBar.setColour(juce::TextEditor::backgroundColourId, juce::Colours::transparentBlack);
    searchBar.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);

    searchBar.onTextChange = [this] {
        browserFilter.setSearchText(searchBar.getText());
        if (directoryList) directoryList->refresh();
    };

    // Navigation Up Button
    upButton.setButtonText("^");
    upButton.setTooltip("Up one level");
    upButton.onClick = [this] {
        if (directoryList) {
            auto parent = directoryList->getDirectory().getParentDirectory();
            if (parent.isDirectory()) directoryList->setDirectory(parent, true, true);
        }
    };

    sidebar = std::make_unique<SidebarComponent>();
    sidebar->onPlaceSelected = [this](const juce::File& loc) {
        if (directoryList) directoryList->setDirectory(loc, true, true);
    };

    directoryList = std::make_unique<juce::DirectoryContentsList>(&browserFilter, thread);
    thread.startThread();
    directoryList->setDirectory(juce::File::getSpecialLocation(juce::File::userHomeDirectory), true, true);

    fileTree = std::make_unique<juce::FileTreeComponent>(*directoryList);
    fileTree->setDragAndDropDescription("AudioFile");
    // Make file tree look nicer
    fileTree->setColour(juce::FileTreeComponent::backgroundColourId, juce::Colours::transparentBlack);

    previewPanel = std::make_unique<PreviewPanel>(*this);

    fileBrowserTab = std::make_unique<FileBrowserTab>(sidebar.get(), fileTree.get(), previewPanel.get(), searchBar, upButton);

    tabs.addTab("Files", juce::Colours::transparentBlack, fileBrowserTab.get(), false);
    tabs.addTab("Plugins", juce::Colours::transparentBlack, &pluginList, false);

    addAndMakeVisible(tabs);

    startTimer(100);
}

BrowserComponent::~BrowserComponent()
{
    stopTimer();
    transportSource.setSource(nullptr);
    tabs.clearTabs();

    if (fileBrowserTab)
    {
        fileBrowserTab->removeAllChildren();
    }

    fileBrowserTab.reset();
    previewPanel.reset();
    fileTree.reset();
    directoryList.reset();
    sidebar.reset();
    thread.stopThread(1000);
}

void BrowserComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::browserBackgroundColourId));
    g.setColour(findColour(OrionLookAndFeel::trackHeaderSeparatorColourId));
    g.drawRect(getLocalBounds().removeFromLeft(1));
}

void BrowserComponent::resized()
{
    tabs.setBounds(getLocalBounds());
}

void BrowserComponent::lookAndFeelChanged()
{
    auto tabColor = findColour(OrionLookAndFeel::trackHeaderPanelColourId);
    if (tabs.getNumTabs() > 0) tabs.setTabBackgroundColour(0, tabColor);
    if (tabs.getNumTabs() > 1) tabs.setTabBackgroundColour(1, tabColor);

    // Style the navigation bar
    upButton.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    upButton.setColour(juce::TextButton::textColourOffId, findColour(juce::Label::textColourId));
}


void BrowserComponent::timerCallback()
{
    if (fileTree) {
        auto file = fileTree->getSelectedFile();
        if (file.existsAsFile() && file != lastSelectedFile) {
            lastSelectedFile = file;
            if (previewPanel) previewPanel->setFile(file);
        }
    }
}

void BrowserComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void BrowserComponent::releaseResources()
{
    transportSource.releaseResources();
}

void BrowserComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{
    if (transportSource.isPlaying())
    {
        transportSource.getNextAudioBlock(bufferToFill);
    }
}

void BrowserComponent::loadFileForPreview(const juce::File& file)
{
    transportSource.stop();
    transportSource.setSource(nullptr);
    readerSource.reset();

    auto* reader = formatManager.createReaderFor(file);
    if (reader)
    {
        readerSource = std::make_unique<juce::AudioFormatReaderSource>(reader, true);
        transportSource.setSource(readerSource.get(), 0, nullptr, reader->sampleRate);
        thumbnail.setSource(new juce::FileInputSource(file));
    }
}

std::string BrowserComponent::getState() const
{
    if (directoryList)
    {
        return directoryList->getDirectory().getFullPathName().toStdString();
    }
    return "";
}

void BrowserComponent::restoreState(const std::string& state)
{
    if (!state.empty() && directoryList)
    {
        juce::File f(state);
        if (f.isDirectory())
        {
            directoryList->setDirectory(f, true, true);
        }
    }
}
