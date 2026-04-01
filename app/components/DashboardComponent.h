#pragma once
#include <JuceHeader.h>
#include "../ProjectManager.h"
#include "../OrionLookAndFeel.h"
#include "../OrionIcons.h"
#include "NewProjectDialog.h"
#include "MarkdownViewer.h"
#include "SettingsWindow.h"

namespace Orion {


    class SidebarItem : public juce::Button
    {
    public:
        SidebarItem(const juce::String& name, const juce::Path& iconPath);
        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
    private:
        juce::Path icon;
        juce::Colour activeColour;
    };

    class DashboardComponent : public juce::Component
    {
    public:
        DashboardComponent(ProjectManager& pm, juce::AudioDeviceManager& deviceManager, std::function<void(const juce::String&)> onThemeChange);
        ~DashboardComponent() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

        std::function<void(const std::string&)> onProjectSelected;
        std::function<void(const NewProjectSettings&)> onNewProjectCreated;
        std::function<void(const std::string&)> onProjectPreviewPlay;
        std::function<void(void)> onProjectPreviewStop;
        std::function<void(double)> onProjectPreviewSeek;
        std::function<void()> onSettingsRequested;

        void refreshProjects();
        void setPlaybackPosition(double seconds, bool isFinished = false);
        void stopPlayback();

        void showSettingsTab();
        void createNewProject();

    private:
        juce::AudioFormatManager formatManager;
        juce::AudioThumbnailCache thumbnailCache { 20 };


        SidebarItem homeBtn { "Home", OrionIcons::getHomeIcon() };
        SidebarItem favoritesBtn { "Favorites", OrionIcons::getStarIcon() };
        SidebarItem allProjectsBtn { "All Projects", OrionIcons::getFolderIcon() };
        SidebarItem settingsBtn { "Settings", OrionIcons::getSettingsIcon() };
        SidebarItem learnBtn { "Learn", OrionIcons::getDocumentationIcon() };
        SidebarItem communityBtn { "Community", OrionIcons::getCommunityIcon() };
        SidebarItem extensionsBtn { "Extensions", OrionIcons::getPluginIcon() };


        juce::Label titleLabel { "Dashboard", "Home" };
        juce::TextEditor searchBox;
        juce::TextButton newProjectBtn { "New Project" };
        juce::TextButton openProjectBtn { "Open Project" };

        juce::Viewport viewport;
        std::unique_ptr<juce::Component> contentComponent;


        class SetupWizard : public juce::Component
        {
        public:
            SetupWizard(std::function<void()> onComplete);
            void paint(juce::Graphics& g) override;
            void resized() override;
        private:
            juce::Label welcomeLabel { "Welcome", "Welcome to Orion" };
            juce::Label descLabel { "Desc", "Let's get you set up. Please choose a location for your projects and settings." };
            juce::TextEditor pathEditor;
            juce::TextButton browseBtn { "Browse..." };
            juce::TextButton finishBtn { "Finish Setup" };
            std::function<void()> onComplete;
            std::unique_ptr<juce::FileChooser> fileChooser;
        };

        std::unique_ptr<SetupWizard> setupWizard;
        std::unique_ptr<NewProjectDialog> newProjectDialog;

        ProjectManager& projectManager;
        std::vector<ProjectInfo> currentProjects;
        juce::String currentCategory = "Home";
        std::string currentPlayingPath;

        void buildGrid();
        void buildHomeLayout();
        void buildLearnLayout();
        void buildExtensionsLayout();
        void buildSettingsLayout();
        void showSetupWizard();
        void clearContentComponent();
        void requestProjectDeletion(const std::string& path);

        juce::AudioDeviceManager& deviceManager;
        std::function<void(const juce::String&)> onThemeChange;
        std::unique_ptr<SettingsWindow> settingsComponent;

        juce::Image logoImage;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DashboardComponent)
    };

}
