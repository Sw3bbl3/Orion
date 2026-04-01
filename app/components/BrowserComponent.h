#pragma once
#include <JuceHeader.h>
#include "PluginListComponent.h"

class BrowserComponent : public juce::Component,
                         public juce::AudioSource,
                         public juce::Timer
{
public:
    BrowserComponent();
    ~BrowserComponent() override;


    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;


    void timerCallback() override;


    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void releaseResources() override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) override;

    std::string getState() const;
    void restoreState(const std::string& state);

private:
    juce::TabbedComponent tabs { juce::TabbedButtonBar::TabsAtTop };


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    juce::AudioThumbnailCache thumbnailCache { 5 };
    juce::AudioThumbnail thumbnail;


    juce::TimeSliceThread thread { "Browser Thread" };
    juce::File lastSelectedFile;


    class BrowserFilter : public juce::FileFilter
    {
    public:
        BrowserFilter() : juce::FileFilter("Browser Filter") {}

        bool isFileSuitable(const juce::File& file) const override
        {
            if (file.isDirectory()) return true;


            if (searchText.isNotEmpty() && !file.getFileName().containsIgnoreCase(searchText))
                return false;


            if (file.hasFileExtension(".wav") || file.hasFileExtension(".mp3") ||
                file.hasFileExtension(".aif") || file.hasFileExtension(".ogg") ||
                file.hasFileExtension(".mid") || file.hasFileExtension(".midi"))
                return true;

            return false;
        }

        bool isDirectorySuitable(const juce::File& file) const override {
            juce::ignoreUnused(file);
            return true;
        }

        void setSearchText(const juce::String& text) { searchText = text; }

    private:
        juce::String searchText;
    };

    BrowserFilter browserFilter;
    std::unique_ptr<juce::DirectoryContentsList> directoryList;
    std::unique_ptr<juce::FileTreeComponent> fileTree;


    class SidebarComponent : public juce::Component, public juce::ListBoxModel
    {
    public:
        SidebarComponent();
        ~SidebarComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void refresh();
        void addPin(const juce::File& folder);


        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void selectedRowsChanged(int lastRowSelected) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

        std::function<void(const juce::File&)> onPlaceSelected;

    private:
        struct Place {
            juce::String name;
            juce::File location;
            juce::String icon;
            bool isPinned = false;
            bool isHeader = false;
        };
        std::vector<Place> places;
        juce::ListBox listBox;

        void removePin(const juce::String& path);
    };

    std::unique_ptr<SidebarComponent> sidebar;


    class PreviewPanel : public juce::Component, public juce::ChangeListener, public juce::Timer
    {
    public:
        PreviewPanel(BrowserComponent& owner);
        ~PreviewPanel() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        void changeListenerCallback(juce::ChangeBroadcaster* source) override;
        void timerCallback() override;

        void setFile(const juce::File& file);
        void play();
        void stop();

        bool isAutoPlay() const { return autoPlayButton.getToggleState(); }

    private:
        BrowserComponent& owner;
        juce::ShapeButton playButton { "Play", juce::Colours::white, juce::Colours::white, juce::Colours::white };
        juce::ToggleButton autoPlayButton { "Auto" };
        juce::Slider volumeSlider;
        juce::Label filenameLabel;
    };

    std::unique_ptr<PreviewPanel> previewPanel;


    class FileBrowserTab : public juce::Component
    {
    public:
        FileBrowserTab(juce::Component* sidebar, juce::Component* tree, juce::Component* preview, juce::TextEditor& search, juce::Button& upBtn)
            : sidebarComponent(sidebar), treeComponent(tree), previewComponent(preview), searchBar(search), upButton(upBtn)
        {
            addAndMakeVisible(searchBar);
            addAndMakeVisible(upButton);
            if (sidebarComponent) addAndMakeVisible(sidebarComponent);
            if (treeComponent) addAndMakeVisible(treeComponent);
            if (previewComponent) addAndMakeVisible(previewComponent);
        }

        void paint(juce::Graphics& g) override
        {
             // Draw background for top bar
             g.setColour(juce::Colours::black.withAlpha(0.2f));
             g.fillRect(0, 0, getWidth(), 30);

             // Separator under search
             g.setColour(juce::Colours::white.withAlpha(0.1f));
             g.drawHorizontalLine(30, 0.0f, (float)getWidth());
        }

        void resized() override
        {
            auto area = getLocalBounds();
            auto topBar = area.removeFromTop(30).reduced(2);

            upButton.setBounds(topBar.removeFromLeft(30));
            topBar.removeFromLeft(4);
            searchBar.setBounds(topBar);

            auto contentArea = area.removeFromTop((int)(area.getHeight() * 0.75f));


            if (previewComponent) previewComponent->setBounds(area);


            if (sidebarComponent) sidebarComponent->setBounds(contentArea.removeFromLeft((int)(contentArea.getWidth() * 0.35f))); // slightly wider sidebar


            if (treeComponent) treeComponent->setBounds(contentArea);
        }

    private:
        juce::Component* sidebarComponent;
        juce::Component* treeComponent;
        juce::Component* previewComponent;
        juce::TextEditor& searchBar;
        juce::Button& upButton;
    };

    std::unique_ptr<FileBrowserTab> fileBrowserTab;
    juce::TextEditor searchBar;
    juce::TextButton upButton;


    OrionPluginListComponent pluginList;


    void loadFileForPreview(const juce::File& file);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BrowserComponent)
};
