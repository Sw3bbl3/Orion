#include "DashboardComponent.h"
#include "../OrionLookAndFeel.h"
#include "../ProjectManager.h"
#include "../../include/orion/SettingsManager.h"
#include "../../include/orion/ProjectSerializer.h"
#include "../OrionIcons.h"
#include "ExtensionEditorComponent.h"
#include "ExtensionViewComponent.h"
#include "DocumentationViewComponent.h"
#include "../ExtensionManager.h"
#include <juce_audio_utils/juce_audio_utils.h>

namespace Orion {



    SidebarItem::SidebarItem(const juce::String& name, const juce::Path& iconPath)
        : juce::Button(name), icon(iconPath)
    {
        activeColour = juce::Colour(0xFF007AFF);
        setClickingTogglesState(true);
        setRadioGroupId(1001);
    }

    void SidebarItem::paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool)
    {
        auto bounds = getLocalBounds().toFloat().reduced(12.0f, 4.0f);
        bool isSelected = getToggleState();
        auto accentColor = findColour(OrionLookAndFeel::dashboardAccentColourId);
        auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);

        if (isSelected)
        {
            juce::ColourGradient grad(accentColor.withAlpha(0.15f), bounds.getTopLeft(),
                                      accentColor.withAlpha(0.00f), bounds.getBottomRight(), false);
            g.setGradientFill(grad);
            g.fillRoundedRectangle(bounds, 8.0f);

            // Left indicator bar
            g.setColour(accentColor);
            g.fillRoundedRectangle(bounds.getX(), bounds.getY() + 6.0f, 4.0f, bounds.getHeight() - 12.0f, 2.0f);
        }
        else if (shouldDrawButtonAsHighlighted || isMouseButtonDown())
        {
            g.setColour(textColor.withAlpha(isMouseButtonDown() ? 0.08f : 0.04f));
            g.fillRoundedRectangle(bounds, 8.0f);
        }

        auto contentArea = bounds.reduced(18.0f, 0.0f);
        auto iconArea = contentArea.removeFromLeft(20.0f).reduced(2.0f);

        // Icon
        auto iconColor = isSelected ? accentColor : textColor.withAlpha(0.75f);
        if (shouldDrawButtonAsHighlighted && !isSelected) iconColor = textColor.withAlpha(0.9f);

        g.setColour(iconColor);
        g.fillPath(icon, icon.getTransformToScaleToFit(iconArea, true));

        // Text
        g.setColour(isSelected ? textColor : textColor.withAlpha(0.8f));
        if (shouldDrawButtonAsHighlighted && !isSelected) g.setColour(textColor.withAlpha(0.9f));

        g.setFont(juce::FontOptions(14.0f, isSelected ? juce::Font::bold : juce::Font::plain));
        contentArea.removeFromLeft(14.0f);
        g.drawText(getButtonText(), contentArea, juce::Justification::centredLeft, true);
    }


    class ProjectCard : public juce::Button, public juce::ChangeListener
    {
    public:
        ProjectCard(const ProjectInfo& info, juce::AudioFormatManager& fm, juce::AudioThumbnailCache& cache)
            : juce::Button(info.name), info(info), thumbnail(512, fm, cache)
        {
            setClickingTogglesState(false);
            thumbnail.addChangeListener(this);


            onClick = [this]() {
                if (onSelect) onSelect(this->info.path);
            };


            addAndMakeVisible(favBtn);
            favBtn.setClickingTogglesState(true);
            favBtn.setToggleState(info.isFavorite, juce::dontSendNotification);
            favBtn.setTooltip("Mark as Favorite");

            favBtn.onClick = [this]() {
                if (onFavoriteToggled) onFavoriteToggled(this->info.path, favBtn.getToggleState());
            };


            juce::File projectFile(info.path);
            juce::File previewFile = projectFile.getParentDirectory().getChildFile("preview.wav");

            if (previewFile.existsAsFile()) {
                thumbnail.setSource(new juce::FileInputSource(previewFile));
                hasPreview = true;
                previewPath = previewFile.getFullPathName().toStdString();

                addAndMakeVisible(playBtn);
                playBtn.setClickingTogglesState(true);
                playBtn.setTooltip("Preview Project");
                playBtn.onClick = [this](){ if(onPlayToggle) onPlayToggle(); };
            }
        }

        ~ProjectCard() override {
            thumbnail.removeChangeListener(this);
        }

        void changeListenerCallback(juce::ChangeBroadcaster*) override { repaint(); }

        std::function<void(const std::string&, bool)> onFavoriteToggled;
        std::function<void()> onPlayToggle;
        std::function<void(double)> onSeek;
        std::function<void(const std::string&)> onSelect;
        std::function<void(const std::string&)> onDelete;
        std::function<void(const std::string&)> onDuplicate;
        std::function<void(const std::string&, const std::string&)> onRename;

        void mouseDown(const juce::MouseEvent& e) override {
            auto bounds = getLocalBounds().toFloat().reduced(6.0f);
            auto thumbArea = bounds.removeFromTop(bounds.getHeight() * 0.65f);

            if (thumbArea.contains(e.position) && !e.mods.isPopupMenu()) {
                double totalTime = thumbnail.getTotalLength();
                if (totalTime > 0) {
                    double seekTime = ((e.position.getX() - thumbArea.getX()) / thumbArea.getWidth()) * totalTime;
                    if (onSeek) onSeek(seekTime);
                }
                return;
            }

            if (e.mods.isPopupMenu()) {
                juce::PopupMenu m;
                m.addItem(1, "Open Project");
                m.addItem(2, "Show in Explorer/Finder");
                m.addSeparator();
                m.addItem(3, info.isFavorite ? "Remove from Favorites" : "Add to Favorites");
                m.addItem(5, "Duplicate Project");
                m.addItem(6, "Rename Project...");
                m.addSeparator();
                m.addItem(4, "Delete Project");

                juce::Component::SafePointer<ProjectCard> sp(this);
                m.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(this), [sp, this](int id) {
                    if (!sp) return;
                    if (id == 1) { if (onSelect) onSelect(info.path); }
                    else if (id == 2) {
                        juce::File(info.path).getParentDirectory().revealToUser();
                    }
                    else if (id == 3) {
                         bool newState = !info.isFavorite;
                         favBtn.setToggleState(newState, juce::dontSendNotification);
                         if (onFavoriteToggled) onFavoriteToggled(info.path, newState);
                    }
                    else if (id == 4) {
                        if (onDelete) onDelete(info.path);
                    }
                    else if (id == 5) {
                        if (onDuplicate) onDuplicate(info.path);
                    }
                    else if (id == 6) {
                        auto* w = new juce::AlertWindow("Rename Project", "Enter new name:", juce::AlertWindow::NoIcon);
                        w->addTextEditor("name", info.name);
                        w->addButton("OK", 1, juce::KeyPress(juce::KeyPress::returnKey));
                        w->addButton("Cancel", 0, juce::KeyPress(juce::KeyPress::escapeKey));

                        juce::Component::SafePointer<juce::AlertWindow> sw(w);
                        w->enterModalState(true, juce::ModalCallbackFunction::create([sp, sw](int result) {
                            if (sp && sw && result == 1) {
                                auto newName = sw->getTextEditorContents("name");
                                if (newName.isNotEmpty() && sp->onRename)
                                    sp->onRename(sp->info.path, newName.toStdString());
                            }
                        }), true);
                    }
                });
                return;
            }
            juce::Button::mouseDown(e);
        }

        void mouseEnter(const juce::MouseEvent& e) override {
            juce::Button::mouseEnter(e);
            if (Orion::SettingsManager::get().hoverPreviewEnabled) {
                if (hasPreview && !isPlaying) {
                    if (onPlayToggle) onPlayToggle();
                }
            }
        }

        void mouseExit(const juce::MouseEvent& e) override {
            juce::Button::mouseExit(e);
            if (Orion::SettingsManager::get().hoverPreviewEnabled) {
                if (isPlaying) {
                    if (onPlayToggle) onPlayToggle();
                }
            }
        }

        void paintButton(juce::Graphics& g, bool shouldDrawButtonAsHighlighted, bool) override
        {
            auto bounds = getLocalBounds().toFloat();
            float cornerSize = 4.0f; // Sharper corners for pro DAW look

            juce::Colour base = findColour(OrionLookAndFeel::dashboardCardBackgroundColourId);
            if (shouldDrawButtonAsHighlighted) base = base.brighter(0.04f);

            g.setColour(base);
            g.fillRoundedRectangle(bounds, cornerSize);

            // 1px inner stroke for crisp edge
            g.setColour(juce::Colours::white.withAlpha(0.05f));
            g.drawRoundedRectangle(bounds.reduced(0.5f), cornerSize, 1.0f);

            auto thumbArea = bounds.removeFromLeft(bounds.getHeight()); // Square thumbnail on the left for list view

            // Thumbnail Background - solid dark grey
            g.setColour(juce::Colour(0xFF141414));
            g.fillRoundedRectangle(thumbArea.reduced(2.0f), cornerSize);
            g.setColour(juce::Colours::white.withAlpha(0.03f));
            g.drawRoundedRectangle(thumbArea.reduced(2.0f), cornerSize, 1.0f);

            bool drawnContent = false;
            // Draw thumbnail or icon in the reduced square area
            auto iconArea = thumbArea.reduced(10.0f);

            if (thumbnail.getNumChannels() > 0 && thumbnail.getTotalLength() > 0) {
                 drawWaveform(g, thumbArea.reduced(4.0f));
                 drawnContent = true;
            }

            if (!drawnContent) {
                 if (std::filesystem::exists(info.thumbnailPath)) {
                    juce::File thumbFile(info.thumbnailPath);
                    auto image = juce::ImageFileFormat::loadFrom(thumbFile);
                    if (image.isValid()) {
                         g.drawImage(image, iconArea, juce::RectanglePlacement::stretchToFit);
                         g.setColour(juce::Colours::black.withAlpha(0.2f));
                         g.fillRoundedRectangle(iconArea, cornerSize);
                         drawnContent = true;
                    }
                }
            }

            if (!drawnContent) {
                OrionIcons::drawIcon(g, OrionIcons::getFolderIcon(), thumbArea.reduced(thumbArea.getWidth() * 0.3f), findColour(OrionLookAndFeel::dashboardTextColourId).withAlpha(0.12f));
            }

            auto infoArea = bounds.reduced(12, 4);

            g.setColour(findColour(OrionLookAndFeel::dashboardTextColourId).withAlpha(0.95f));
            g.setFont(juce::FontOptions(15.0f, juce::Font::bold));
            g.drawText(info.name, infoArea.removeFromTop(20), juce::Justification::centredLeft, true);

            g.setFont(juce::FontOptions(12.0f));
            g.setColour(findColour(OrionLookAndFeel::dashboardTextColourId).withAlpha(0.75f));

            juce::String meta = "Modified: " + juce::String(info.date) + "   |   " + (info.bpm > 0 ? juce::String(info.bpm) + " BPM" : "120 BPM");
            g.drawText(meta, infoArea.removeFromTop(16), juce::Justification::centredLeft, true);

            if (isPlaying) {
                g.setColour(findColour(OrionLookAndFeel::dashboardAccentColourId).withAlpha(0.8f));
                g.drawRoundedRectangle(bounds, cornerSize, 1.5f);
            } else if (shouldDrawButtonAsHighlighted) {
                g.setColour(juce::Colours::white.withAlpha(0.1f));
                g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            } else {
                g.setColour(juce::Colours::black.withAlpha(0.6f));
                g.drawRoundedRectangle(bounds, cornerSize, 1.0f);
            }
        }

        void drawWaveform(juce::Graphics& g, juce::Rectangle<float> area) {
            auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);

            double totalTime = thumbnail.getTotalLength();
            int numBars = (int)area.getWidth() / 4;
            float barWidth = 2.0f;
            float midY = area.getCentreY();
            float maxHeight = area.getHeight() * 0.75f;

            for (int i = 0; i < numBars; ++i) {
                double time = (double)i / numBars * totalTime;
                float minVal, maxVal;
                thumbnail.getApproximateMinMax(time, time + (totalTime / numBars), 0, minVal, maxVal);
                float level = juce::jmax(std::abs(minVal), std::abs(maxVal));
                float h = std::max(2.0f, std::pow(level, 0.5f) * maxHeight);

                float x = area.getX() + i * 4.0f + 1.0f;
                float progress = (float)(playbackTime / totalTime);

                bool isPlayed = isPlaying && (float)i / numBars < progress;

                if (isPlayed)
                {
                    g.setColour(accent);
                    g.fillRoundedRectangle(x, midY - h/2.0f, barWidth, h, 1.0f);
                }
                else
                {
                    g.setColour(accent.withAlpha(0.25f));
                    g.fillRoundedRectangle(x, midY - h/2.0f, barWidth, h, 1.0f);
                }
            }

            // Playhead status line
            if (isPlaying || playbackTime > 0) {
                float px = area.getX() + (float)(playbackTime / totalTime) * area.getWidth();

                // Glow around playhead
                juce::ColourGradient glow(accent.withAlpha(0.3f), px, area.getCentreY(),
                                          accent.withAlpha(0.0f), px + 15, area.getCentreY(), true);
                g.setGradientFill(glow);
                g.fillRect(px - 15, area.getY(), 30.0f, area.getHeight());

                g.setColour(juce::Colours::white);
                g.drawVerticalLine((int)px, area.getY(), area.getBottom());
            }
        }

        void resized() override
        {
            favBtn.setBounds(getWidth() - 32, (getHeight() - 20) / 2, 20, 20);

            float thumbSize = (float)getHeight();
            playBtn.setBounds((int)(thumbSize / 2 - 16), (int)(thumbSize / 2 - 16), 32, 32);
        }

        void resetPlayback() {
            setPreviewState(false);
            playbackTime = 0.0;
        }

        void setPreviewState(bool playing) {
            isPlaying = playing;
            playBtn.setToggleState(playing, juce::dontSendNotification);
            repaint();
        }

        void setPlaybackTime(double t) {
            playbackTime = t;
            repaint();
        }

        bool hasPreview = false;
        std::string previewPath;
        bool isPlaying = false;
        double playbackTime = 0.0;
        ProjectInfo info;

    private:
        juce::AudioThumbnail thumbnail;


        class FavButton : public juce::Button
        {
        public:
            FavButton() : juce::Button("Fav") {}
            void paintButton(juce::Graphics& g, bool highlighted, bool ) override
            {
                auto bounds = getLocalBounds().toFloat();
                juce::Path star = OrionIcons::getStarIcon();

                if (getToggleState()) {
                    g.setColour(juce::Colours::orange);
                    g.fillPath(star, star.getTransformToScaleToFit(bounds.reduced(2), true));
                } else {
                    g.setColour(juce::Colours::white.withAlpha(0.3f));
                    if (highlighted) g.setColour(juce::Colours::white.withAlpha(0.8f));
                    auto t = star.getTransformToScaleToFit(bounds.reduced(2), true);
                    g.strokePath(star, juce::PathStrokeType(1.5f), t);
                }
            }
        };


        class PlayButton : public juce::Button
        {
        public:
            PlayButton() : juce::Button("Play") {}
            void paintButton(juce::Graphics& g, bool highlighted, bool ) override
            {
                auto bounds = getLocalBounds().toFloat();
                bool isParentHovered = getParentComponent() && getParentComponent()->isMouseOver(true);
                bool buttonPlaying = getToggleState();

                if (!buttonPlaying && !isParentHovered) return;

                juce::Colour bg = findColour(OrionLookAndFeel::dashboardAccentColourId);
                if (!buttonPlaying) bg = juce::Colours::black.withAlpha(0.4f);
                if (highlighted) bg = bg.brighter(0.1f);

                g.setColour(bg);
                g.fillEllipse(bounds);

                // Subtle white ring
                g.setColour(juce::Colours::white.withAlpha(0.15f));
                g.drawEllipse(bounds, 1.0f);

                g.setColour(juce::Colours::white);
                juce::Path icon;
                if (buttonPlaying) {
                     float w = bounds.getWidth() * 0.3f;
                     float h = bounds.getHeight() * 0.3f;
                     float x = bounds.getCentreX() - w/2;
                     float y = bounds.getCentreY() - h/2;
                     icon.addRoundedRectangle(x, y, w/3.0f, h, 1.0f);
                     icon.addRoundedRectangle(x + w - w/3.0f, y, w/3.0f, h, 1.0f);
                } else {
                     icon = OrionIcons::getPlayIcon();
                     auto t = icon.getTransformToScaleToFit(bounds.reduced(16), true);
                     t = t.translated(1.5f, 0);
                     icon.applyTransform(t);
                }
                g.fillPath(icon);
            }
        };

        FavButton favBtn;
        PlayButton playBtn;
    };


    DashboardComponent::SetupWizard::SetupWizard(std::function<void()> onComplete) : onComplete(onComplete)
    {
        addAndMakeVisible(welcomeLabel);
        welcomeLabel.setFont(juce::FontOptions(24.0f, juce::Font::bold));
        welcomeLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(descLabel);
        descLabel.setFont(juce::FontOptions(16.0f));
        descLabel.setJustificationType(juce::Justification::centred);

        addAndMakeVisible(pathEditor);
        pathEditor.setText(juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Orion").getFullPathName());

        addAndMakeVisible(browseBtn);
        browseBtn.onClick = [this]() {
            fileChooser = std::make_unique<juce::FileChooser>("Select User Data Folder", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
            auto folderFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

            fileChooser->launchAsync(folderFlags, [this](const juce::FileChooser& chooser) {
                auto result = chooser.getResult();
                if (result.exists()) {
                    pathEditor.setText(result.getFullPathName());
                }
            });
        };

        addAndMakeVisible(finishBtn);
        finishBtn.onClick = [this]() {

            juce::File root(pathEditor.getText());
            if (!root.exists()) root.createDirectory();

            SettingsManager::setInstanceRoot(root.getFullPathName().toStdString());

            auto s = SettingsManager::get();
            s.setupComplete = true;
            s.rootDataPath = root.getFullPathName().toStdString();
            SettingsManager::set(s);

            if (this->onComplete) this->onComplete();
        };
    }

    void DashboardComponent::SetupWizard::paint(juce::Graphics& g)
    {
        g.fillAll(findColour(OrionLookAndFeel::dashboardBackgroundColourId).withAlpha(0.95f));

        auto area = getLocalBounds().toFloat();
        auto box = area.withSizeKeepingCentre(500, 350);

        g.setColour(findColour(OrionLookAndFeel::dashboardCardBackgroundColourId));
        g.fillRoundedRectangle(box, 12.0f);

        g.setColour(juce::Colours::black.withAlpha(0.2f));
        g.drawRoundedRectangle(box, 12.0f, 1.0f);
    }

    void DashboardComponent::SetupWizard::resized()
    {
        auto area = getLocalBounds().withSizeKeepingCentre(500, 350);
        area.reduce(40, 40);

        welcomeLabel.setBounds(area.removeFromTop(40));
        descLabel.setBounds(area.removeFromTop(60));

        area.removeFromTop(20);

        auto row = area.removeFromTop(30);
        pathEditor.setBounds(row.removeFromLeft(row.getWidth() - 100));
        row.removeFromLeft(10);
        browseBtn.setBounds(row);

        area.removeFromTop(40);
        finishBtn.setBounds(area.removeFromTop(40).withSizeKeepingCentre(150, 40));
    }




    DashboardComponent::DashboardComponent(ProjectManager& pm, juce::AudioDeviceManager& dm, std::function<void(const juce::String&)> onTheme)
        : projectManager(pm), deviceManager(dm), onThemeChange(onTheme)
    {
        // Load Logo
        auto execFile = juce::File::getSpecialLocation(juce::File::currentExecutableFile);
        auto logoFile = execFile.getParentDirectory().getChildFile("assets/orion/Orion_logo_transpant_bg.png");

        // Try other common locations if not found
        if (!logoFile.exists())
            logoFile = juce::File::getCurrentWorkingDirectory().getChildFile("assets/orion/Orion_logo_transpant_bg.png");
        if (!logoFile.exists())
            logoFile = execFile.getParentDirectory().getChildFile("../assets/orion/Orion_logo_transpant_bg.png");

        if (logoFile.exists())
            logoImage = juce::ImageFileFormat::loadFrom(logoFile);

        formatManager.registerBasicFormats();


        addAndMakeVisible(homeBtn);
        addAndMakeVisible(favoritesBtn);
        addAndMakeVisible(allProjectsBtn);

        if (Orion::SettingsManager::get().developerMode) {
            addAndMakeVisible(extensionsBtn);
        }

        addAndMakeVisible(settingsBtn);
        addAndMakeVisible(learnBtn);
        addAndMakeVisible(communityBtn);

        homeBtn.setToggleState(true, juce::dontSendNotification);


        addAndMakeVisible(titleLabel);
        titleLabel.setFont(juce::FontOptions(28.0f, juce::Font::bold));
        titleLabel.setColour(juce::Label::textColourId, findColour(OrionLookAndFeel::dashboardTextColourId));

        addAndMakeVisible(searchBox);
        searchBox.setTextToShowWhenEmpty("Search projects...", findColour(OrionLookAndFeel::dashboardTextColourId).withAlpha(0.5f));
        searchBox.setColour(juce::TextEditor::backgroundColourId, juce::Colours::black.withAlpha(0.1f));
        searchBox.setColour(juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
        searchBox.setColour(juce::TextEditor::focusedOutlineColourId, findColour(OrionLookAndFeel::dashboardAccentColourId));
        searchBox.onTextChange = [this](){ refreshProjects(); };

        addAndMakeVisible(newProjectBtn);
        newProjectBtn.setColour(juce::TextButton::buttonColourId, findColour(OrionLookAndFeel::dashboardAccentColourId));
        newProjectBtn.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
        newProjectBtn.onClick = [this] { createNewProject(); };

        addAndMakeVisible(openProjectBtn);
        openProjectBtn.setColour(juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        openProjectBtn.setColour(juce::TextButton::textColourOffId, findColour(OrionLookAndFeel::dashboardAccentColourId));
        openProjectBtn.setButtonText("Open Project");
        openProjectBtn.onClick = [this]() {
            auto chooser = std::make_shared<juce::FileChooser>("Open Project", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory), "*.orion");

            juce::Component::SafePointer<DashboardComponent> sp(this);
            chooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [sp, chooser](const juce::FileChooser& fc) {
                if (sp) {
                    auto result = fc.getResult();
                    if (result.existsAsFile()) {
                        if (sp->onProjectSelected) sp->onProjectSelected(result.getFullPathName().toStdString());
                    }
                }
            });
        };


        addAndMakeVisible(viewport);
        contentComponent = std::make_unique<juce::Component>();
        viewport.setViewedComponent(contentComponent.get(), false);
        viewport.setScrollBarsShown(true, false);


        viewport.setScrollBarsShown(true, false, true, true);


        homeBtn.onClick = [this](){ currentCategory="Home"; refreshProjects(); };
        favoritesBtn.onClick = [this](){ currentCategory="Favorites"; refreshProjects(); };
        allProjectsBtn.onClick = [this](){ currentCategory="All Projects"; refreshProjects(); };
        learnBtn.onClick = [this](){ currentCategory="Learn"; refreshProjects(); };
        extensionsBtn.onClick = [this](){
            if (!Orion::SettingsManager::get().developerMode)
                return;
            currentCategory="Extensions";
            refreshProjects();
        };


        settingsBtn.onClick = [this](){ currentCategory="Settings"; refreshProjects(); };

        extensionsBtn.setVisible(Orion::SettingsManager::get().developerMode);


        if (!SettingsManager::get().setupComplete) {
            showSetupWizard();
        } else {
            refreshProjects();
        }
    }

    void DashboardComponent::createNewProject()
    {
        newProjectDialog = std::make_unique<NewProjectDialog>(
            [this](const NewProjectSettings& s) {
                if (onNewProjectCreated) onNewProjectCreated(s);
                newProjectDialog = nullptr;
                resized();
            },
            [this]() {
                newProjectDialog = nullptr;
                resized();
            }
        );
        addAndMakeVisible(newProjectDialog.get());
        resized();
    }

    DashboardComponent::~DashboardComponent() {}

    void DashboardComponent::paint(juce::Graphics& g)
    {
        auto bg = findColour(OrionLookAndFeel::dashboardBackgroundColourId);
        g.fillAll(bg);

        // Sidebar background
        auto sidebarBg = findColour(OrionLookAndFeel::dashboardSidebarBackgroundColourId);
        g.setColour(sidebarBg);
        g.fillRect(0, 0, 240, getHeight());

        // Sidebar separator with gradient
        juce::Colour separatorColor = juce::Colours::black.withAlpha(0.3f);
        juce::ColourGradient sepGrad(separatorColor.withAlpha(0.0f), 240.0f, 0.0f,
                                     separatorColor, 240.0f, (float)getHeight() * 0.1f, false);
        sepGrad.addColour(0.9f, separatorColor);
        sepGrad.addColour(1.0f, separatorColor.withAlpha(0.0f));
        g.setGradientFill(sepGrad);
        g.fillRect(240.0f, 0.0f, 1.0f, (float)getHeight());

        // Modern Logo & Title
        float logoX = 28.0f;
        float logoY = 32.0f;
        float logoSize = 34.0f;

        if (logoImage.isValid())
        {
            g.setOpacity(1.0f);
            g.drawImageWithin(logoImage, (int)logoX, (int)logoY, (int)logoSize, (int)logoSize, juce::RectanglePlacement::centred);
            g.drawImageWithin(logoImage, (int)logoX, (int)logoY, (int)logoSize, (int)logoSize, juce::RectanglePlacement::centred);
        }
        else
        {
            g.setColour(findColour(OrionLookAndFeel::dashboardAccentColourId));
            g.fillRoundedRectangle(logoX, logoY, (float)logoSize, (float)logoSize, 8.0f);
        }

        float textX = logoX + logoSize + 12.0f;
        auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);

        g.setColour(textColor);
        g.setFont(juce::FontOptions(20.0f, juce::Font::bold).withHorizontalScale(0.9f));
        g.drawText("ORION", (int)textX, (int)logoY, 100, (int)logoSize, juce::Justification::centredLeft);

        // Beta Badge
        float badgeX = textX;
        float badgeY = logoY + logoSize - 8.0f;

        juce::String badgeText = "BETA";
        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));

        juce::GlyphArrangement ga;
        ga.addLineOfText (g.getCurrentFont(), badgeText, 0.0f, 0.0f);
        float badgeTextWidth = ga.getBoundingBox (0, -1, true).getWidth();

        juce::Rectangle<float> badgeRect(badgeX, badgeY, badgeTextWidth + 8.0f, 14.0f);

        auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);
        juce::ColourGradient badgeGrad(accent.brighter(0.2f), badgeRect.getTopLeft(), accent.darker(0.1f), badgeRect.getBottomRight(), false);
        g.setGradientFill(badgeGrad);
        g.fillRoundedRectangle(badgeRect, 3.0f);
        
        g.setColour(juce::Colours::white.withAlpha(0.2f));
        g.drawRoundedRectangle(badgeRect, 3.0f, 1.0f);

        g.setColour(juce::Colours::white);
        g.drawText(badgeText, badgeRect, juce::Justification::centred);
    }

    void DashboardComponent::resized()
    {
        auto area = getLocalBounds();

        if (setupWizard) {
             setupWizard->setBounds(area);
             return;
        }

        auto sidebarArea = area.removeFromLeft(240);
        sidebarArea.removeFromTop(100); // More space for logo

        juce::FlexBox sidebarFlex;
        sidebarFlex.flexDirection = juce::FlexBox::Direction::column;
        sidebarFlex.items.add(juce::FlexItem(homeBtn).withHeight(36));
        sidebarFlex.items.add(juce::FlexItem(favoritesBtn).withHeight(36));
        sidebarFlex.items.add(juce::FlexItem(allProjectsBtn).withHeight(36));
        if (extensionsBtn.isVisible()) sidebarFlex.items.add(juce::FlexItem(extensionsBtn).withHeight(36));

        sidebarFlex.items.add(juce::FlexItem().withFlex(1.0f)); // Spacer

        sidebarFlex.items.add(juce::FlexItem(settingsBtn).withHeight(36));
        sidebarFlex.items.add(juce::FlexItem(learnBtn).withHeight(36));
        sidebarFlex.items.add(juce::FlexItem(communityBtn).withHeight(36));
        sidebarFlex.items.add(juce::FlexItem().withHeight(32));

        sidebarFlex.performLayout(sidebarArea);

        auto headerArea = area.removeFromTop(90).reduced(40, 25);
        titleLabel.setBounds(headerArea.removeFromLeft(200));

        newProjectBtn.setBounds(headerArea.removeFromRight(130));
        headerArea.removeFromRight(12);
        openProjectBtn.setBounds(headerArea.removeFromRight(130));
        headerArea.removeFromRight(24);
        searchBox.setBounds(headerArea.removeFromRight(280));

        viewport.setBounds(area);

        if (contentComponent) {
            buildGrid();
        }

        if (newProjectDialog) {
            newProjectDialog->setBounds(getLocalBounds());
            newProjectDialog->toFront(true);
        }
    }

    void DashboardComponent::showSetupWizard()
    {
        setupWizard = std::make_unique<SetupWizard>([this]() {
            setupWizard = nullptr;
            refreshProjects();
            resized();
        });
        addAndMakeVisible(setupWizard.get());
        resized();
    }

    void DashboardComponent::refreshProjects()
    {
        if (currentCategory == "Extensions" && !Orion::SettingsManager::get().developerMode)
            currentCategory = "Home";

        titleLabel.setText(currentCategory, juce::dontSendNotification);

        if (currentCategory == "Home") currentProjects = projectManager.getRecentProjects();
        else if (currentCategory == "Favorites") currentProjects = projectManager.getFavoriteProjects();
        else currentProjects = projectManager.getAllProjects();


        juce::String filter = searchBox.getText();
        if (filter.isNotEmpty()) {
            std::vector<ProjectInfo> filtered;
            for(const auto& p : currentProjects) {
                if (juce::String(p.name).containsIgnoreCase(filter)) filtered.push_back(p);
            }
            currentProjects = filtered;
        }

        buildGrid();
    }


    void DashboardComponent::clearContentComponent()
    {
        if (settingsComponent && settingsComponent->getParentComponent() == contentComponent.get()) {
            contentComponent->removeChildComponent(settingsComponent.get());
        }
        contentComponent->deleteAllChildren();
    }

    void DashboardComponent::buildGrid()
    {
        if (currentCategory == "Extensions" && !Orion::SettingsManager::get().developerMode)
            currentCategory = "Home";

        clearContentComponent();

        if (currentCategory == "Home")
        {
            buildHomeLayout();
            return;
        }

        if (currentCategory == "Learn")
        {
            buildLearnLayout();
            return;
        }

        if (currentCategory == "Extensions")
        {
            buildExtensionsLayout();
            return;
        }

        if (currentCategory == "Settings")
        {
            buildSettingsLayout();
            return;
        }

        int cardWidth = 280;
        int cardHeight = 260;
        int gap = 32;

        int w = viewport.getWidth();
        if (w <= 0) w = 800;


        int cols = std::max(1, (w - 80) / (cardWidth + gap));

        int x = 40;
        int y = 20;
        int col = 0;

        for (const auto& p : currentProjects) {
            auto* card = new ProjectCard(p, formatManager, thumbnailCache);

            card->onDuplicate = [this](const std::string& path) {
                if (projectManager.duplicateProject(path)) refreshProjects();
            };

            card->onRename = [this](const std::string& path, const std::string& newName) {
                if (projectManager.renameProject(path, newName)) refreshProjects();
            };

            contentComponent->addAndMakeVisible(card);
            card->setBounds(x, y, cardWidth, cardHeight);


            if (p.path == currentPlayingPath) {

                 card->isPlaying = true;
            }

            card->onSelect = [this](const std::string& path) {
                if (onProjectSelected) onProjectSelected(path);
            };

            card->onSeek = [this](double pos) {
                if (onProjectPreviewSeek) onProjectPreviewSeek(pos);
            };

            card->onFavoriteToggled = [this](const std::string& path, bool ) {
                projectManager.toggleFavorite(path);
                refreshProjects();
            };

            card->onDelete = [this](const std::string& path) {
                requestProjectDeletion(path);
            };

            card->onPlayToggle = [this, p, card]() {
                if (currentPlayingPath == p.path && card->isPlaying) {

                    if (onProjectPreviewStop) onProjectPreviewStop();
                    card->isPlaying = false;
                    currentPlayingPath = "";
                    card->repaint();
                } else {

                    if (currentPlayingPath != p.path) {
                         currentPlayingPath = p.path;


                        for (auto* child : contentComponent->getChildren()) {
                            if (auto* c = dynamic_cast<ProjectCard*>(child)) {
                                if (c != card) c->resetPlayback();
                            }
                        }
                    }
                    if (onProjectPreviewPlay) onProjectPreviewPlay(p.path);
                    card->isPlaying = true;
                    card->repaint();
                }
            };

            x += cardWidth + gap;
            col++;
            if (col >= cols) {
                col = 0;
                x = 40;
                y += cardHeight + gap;
            }
        }

        contentComponent->setSize(w, y + cardHeight + 40);
    }

    void DashboardComponent::requestProjectDeletion(const std::string& path)
    {
        juce::File projectFile(path);
        if (!projectFile.existsAsFile())
        {
            juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                   "Delete Project",
                                                   "This project no longer exists on disk.");
            refreshProjects();
            return;
        }

        juce::File parentDir = projectFile.getParentDirectory();
        bool willDeleteFolder = (parentDir.getFileName() == projectFile.getFileNameWithoutExtension());

        juce::String message = "Delete \"" + projectFile.getFileNameWithoutExtension() + "\"?\n\n";
        if (willDeleteFolder)
        {
            message += "This will permanently delete the full project folder and all its contents:\n";
            message += parentDir.getFullPathName();
        }
        else
        {
            message += "Folder name does not match project name, so only the project file will be deleted:\n";
            message += projectFile.getFullPathName();
        }

        juce::Component::SafePointer<DashboardComponent> sp(this);
        juce::AlertWindow::showOkCancelBox(juce::AlertWindow::WarningIcon,
                                           "Delete Project",
                                           message,
                                           "Delete",
                                           "Cancel",
                                           this,
                                           juce::ModalCallbackFunction::create([sp, path](int result) {
                                               if (!sp || result == 0) return;

                                               if (sp->currentPlayingPath == path) {
                                                   if (sp->onProjectPreviewStop) sp->onProjectPreviewStop();
                                                   sp->currentPlayingPath.clear();
                                               }

                                               bool deletedFolder = false;
                                               std::string error;
                                               if (!sp->projectManager.deleteProject(path, &deletedFolder, &error)) {
                                                   juce::String body = "Could not delete project.";
                                                   if (!error.empty()) body += "\n\n" + juce::String(error);
                                                   juce::AlertWindow::showMessageBoxAsync(juce::AlertWindow::WarningIcon,
                                                                                          "Delete Failed",
                                                                                          body);
                                               }

                                               sp->refreshProjects();
                                           }));
    }

    void DashboardComponent::setPlaybackPosition(double seconds, bool isFinished)
    {
        if (currentPlayingPath.empty()) return;


        for (auto* child : contentComponent->getChildren()) {
            if (auto* card = dynamic_cast<ProjectCard*>(child)) {

                if (card->hasPreview && card->previewPath == currentPlayingPath) {
                    if (isFinished) {
                        card->resetPlayback();
                        currentPlayingPath = "";
                    } else {
                        if (!card->isPlaying) card->isPlaying = true;
                        card->setPlaybackTime(seconds);
                    }
                }
            }
        }
    }

    void DashboardComponent::stopPlayback()
    {
        currentPlayingPath = "";
        for (auto* child : contentComponent->getChildren()) {
            if (auto* card = dynamic_cast<ProjectCard*>(child)) {
                card->resetPlayback();
            }
        }
    }

    void DashboardComponent::buildHomeLayout()
    {
        clearContentComponent();

        auto bounds = viewport.getBounds();
        int w = bounds.getWidth();
        if (w <= 0) w = 800;
        int margin = 45;
        int availableW = w - (margin * 2);

        int leftColW = (int)(availableW * 0.65f);
        int rightColW = availableW - leftColW - 40; // 40px gap
        int leftX = margin;
        int rightX = margin + leftColW + 40;
        int y = 30;

        auto textColor = findColour(OrionLookAndFeel::dashboardTextColourId);
        auto accentColor = findColour(OrionLookAndFeel::dashboardAccentColourId);
        auto cardColor = findColour(OrionLookAndFeel::dashboardCardBackgroundColourId);

        // --- LEFT COLUMN: RECENT PROJECTS ---
        auto* recentsHeader = new juce::Label("recentsHeader", "Recent Projects");
        recentsHeader->setFont(juce::FontOptions(18.0f, juce::Font::bold));
        recentsHeader->setColour(juce::Label::textColourId, textColor.withAlpha(0.9f));
        contentComponent->addAndMakeVisible(recentsHeader);
        recentsHeader->setBounds(leftX, y, 300, 30);
        
        y += 40;
        int startY = y;

        int cardWidth = leftColW;
        int cardHeight = 80; // Compact horizontal list style
        int gap = 8;
        
        int count = 0;
        for (const auto& p : currentProjects) {
            if (count >= 12) break; // Allow more items in list form
            auto* card = new ProjectCard(p, formatManager, thumbnailCache);

            card->onDuplicate = [this](const std::string& path) {
                if (projectManager.duplicateProject(path)) refreshProjects();
            };

            card->onRename = [this](const std::string& path, const std::string& newName) {
                if (projectManager.renameProject(path, newName)) refreshProjects();
            };

            contentComponent->addAndMakeVisible(card);
            card->setBounds(leftX, y, cardWidth, cardHeight);
            
            card->onSelect = [this](const std::string& path) { if (onProjectSelected) onProjectSelected(path); };
            card->onFavoriteToggled = [this](const std::string& path, bool) {
                projectManager.toggleFavorite(path);
                refreshProjects();
            };
            card->onDelete = [this](const std::string& path) {
                requestProjectDeletion(path);
            };
            if (p.path == currentPlayingPath) card->setPreviewState(true);
            card->onPlayToggle = [this, p, card]() {
                if (currentPlayingPath == p.path && card->isPlaying) {
                    if (onProjectPreviewStop) onProjectPreviewStop();
                    card->setPreviewState(false);
                    currentPlayingPath = "";
                } else {
                    if (currentPlayingPath != p.path) {
                        currentPlayingPath = p.path;
                        for (auto* child : contentComponent->getChildren()) {
                            if (auto* c = dynamic_cast<ProjectCard*>(child)) if (c != card) c->resetPlayback();
                        }
                    }
                    if (onProjectPreviewPlay) onProjectPreviewPlay(p.path);
                    card->setPreviewState(true);
                }
            };
            
            y += cardHeight + gap;
            count++;
        }

        if (count == 0) {
            auto* emptyLabel = new juce::Label("empty", "No recent projects found.");
            emptyLabel->setColour(juce::Label::textColourId, textColor.withAlpha(0.6f));
            contentComponent->addAndMakeVisible(emptyLabel);
            emptyLabel->setBounds(leftX, startY, leftColW, 60);
        }

        // --- SETUP OR TEMPLATES ---
        int maxLeftY = y + 40;
        y = 30; // Reset Y for right column

        auto* setupHeader = new juce::Label("setupHeader", "Studio Setup");
        setupHeader->setFont(juce::FontOptions(18.0f, juce::Font::bold));
        setupHeader->setColour(juce::Label::textColourId, textColor.withAlpha(0.9f));
        contentComponent->addAndMakeVisible(setupHeader);
        setupHeader->setBounds(rightX, y, rightColW, 30);
        y += 40;



        // --- SYSTEM STATUS WIDGET ---
        auto* statusBg = new juce::DrawableRectangle();
        statusBg->setFill(cardColor);
        statusBg->setCornerSize({ 6.0f, 6.0f });
        contentComponent->addAndMakeVisible(statusBg);
        statusBg->setBounds(rightX, y, rightColW, 100);

        // Sharp top border indicator
        auto* statusLine = new juce::DrawableRectangle();
        statusLine->setFill(accentColor);
        contentComponent->addAndMakeVisible(statusLine);
        statusLine->setBounds(rightX, y, rightColW, 2);

        // Inner 1px border
        class FrameOutline : public juce::Component {
            void paint(juce::Graphics& g) override {
                g.setColour(juce::Colours::white.withAlpha(0.04f));
                g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(0.5f), 6.0f, 1.0f);
            }
        };
        auto* frame = new FrameOutline();
        contentComponent->addAndMakeVisible(frame);
        frame->setBounds(rightX, y, rightColW, 100);

        auto* statusLabel = new juce::Label("status", "Audio Engine Active");
        statusLabel->setFont(juce::FontOptions(12.0f, juce::Font::bold));
        statusLabel->setColour(juce::Label::textColourId, textColor.withAlpha(0.7f));
        contentComponent->addAndMakeVisible(statusLabel);
        statusLabel->setBounds(rightX + 16, y + 16, 200, 20);

        auto* deviceLabel = new juce::Label("device", deviceManager.getCurrentAudioDeviceType());
        deviceLabel->setFont(juce::FontOptions(14.0f, juce::Font::bold));
        deviceLabel->setColour(juce::Label::textColourId, textColor.withAlpha(0.9f));
        contentComponent->addAndMakeVisible(deviceLabel);
        deviceLabel->setBounds(rightX + 16, y + 42, rightColW - 32, 20);

        auto setup = deviceManager.getAudioDeviceSetup();
        auto* settingsLabel = new juce::Label("settings", juce::String(setup.sampleRate) + " Hz   •   " + juce::String(setup.bufferSize) + " spls");
        settingsLabel->setFont(juce::FontOptions(12.0f));
        settingsLabel->setColour(juce::Label::textColourId, textColor.withAlpha(0.7f));
        contentComponent->addAndMakeVisible(settingsLabel);
        settingsLabel->setBounds(rightX + 16, y + 64, rightColW - 32, 20);

        y += 140;

        // --- QUICK START SECTION ---
        auto* qsHeader = new juce::Label("qsHeader", "Create New");
        qsHeader->setFont(juce::FontOptions(18.0f, juce::Font::bold));
        qsHeader->setColour(juce::Label::textColourId, textColor.withAlpha(0.9f));
        contentComponent->addAndMakeVisible(qsHeader);
        qsHeader->setBounds(rightX, y, rightColW, 30);
        y += 40;

        struct TemplateInfo { juce::String name; juce::Path icon; };
        std::vector<TemplateInfo> templates = {
            { "Empty Project", OrionIcons::getPlusIcon() },
            { "Vocal Session", OrionIcons::getRecordIcon() },
            { "Beat Making", OrionIcons::getPianoRollIcon() },
            { "Podcasting", OrionIcons::getCommunityIcon() }
        };

        class QuickStartItem : public juce::Button {
        public:
            juce::Path icon;
            QuickStartItem(const juce::String& n, const juce::Path& i) : juce::Button(n), icon(i) {}
            void paintButton(juce::Graphics& g, bool isMouseOverButton, bool) override {
                auto b = getLocalBounds().toFloat();
                float corner = 4.0f;
                
                juce::Colour base = findColour(OrionLookAndFeel::dashboardCardBackgroundColourId);
                if (isMouseOverButton) base = base.brighter(0.05f);

                g.setColour(base);
                g.fillRoundedRectangle(b, corner);

                g.setColour(juce::Colours::white.withAlpha(isMouseOverButton ? 0.08f : 0.04f));
                g.drawRoundedRectangle(b.reduced(0.5f), corner, 1.0f);

                auto accent = findColour(OrionLookAndFeel::dashboardAccentColourId);
                auto textC = findColour(OrionLookAndFeel::dashboardTextColourId);

                g.setColour(isMouseOverButton ? accent : textC.withAlpha(0.7f));
                auto iconArea = b.removeFromLeft(40).withSizeKeepingCentre(16, 16);
                g.fillPath(icon, icon.getTransformToScaleToFit(iconArea, true));

                g.setColour(textC.withAlpha(0.85f));
                g.setFont(juce::FontOptions(13.0f, juce::Font::bold));
                g.drawText(getName(), b, juce::Justification::centredLeft);
            }
        };

        for (const auto& t : templates) {
            auto* btn = new QuickStartItem(t.name, t.icon);
            contentComponent->addAndMakeVisible(btn);
            btn->setBounds(rightX, y, rightColW, 44);
            y += 52;
        }
        
        y += 40;

        contentComponent->setSize(w, std::max(y, maxLeftY));
    }

    void DashboardComponent::buildLearnLayout()
    {
        clearContentComponent();

        auto* layout = new DocumentationViewComponent();
        contentComponent->addAndMakeVisible(layout);

        layout->setBounds(0, 0, viewport.getWidth(), viewport.getHeight());
        contentComponent->setSize(viewport.getWidth(), viewport.getHeight());
    }

    void DashboardComponent::buildExtensionsLayout()
    {
        clearContentComponent();

        auto* layout = new ExtensionViewComponent();
        contentComponent->addAndMakeVisible(layout);


        layout->setBounds(0, 0, viewport.getWidth(), viewport.getHeight());


        contentComponent->setSize(viewport.getWidth(), viewport.getHeight());
    }

    void DashboardComponent::showSettingsTab()
    {
        settingsBtn.setToggleState(true, juce::dontSendNotification);
        currentCategory = "Settings";
        refreshProjects();
    }

    void DashboardComponent::buildSettingsLayout()
    {
        clearContentComponent();






        if (!settingsComponent) {
            settingsComponent = std::make_unique<SettingsWindow>(deviceManager, onThemeChange, SettingsWindow::CloseMode::Embedded);
        }

        contentComponent->addAndMakeVisible(settingsComponent.get());

        int w = viewport.getWidth();
        int h = viewport.getHeight();
        if (w <= 0) w = 800;
        if (h <= 0) h = 600;





        settingsComponent->setBounds(0, 0, w, h);
        contentComponent->setSize(w, h);
    }

}
