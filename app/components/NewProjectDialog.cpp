#include "NewProjectDialog.h"
#include "../OrionLookAndFeel.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"
#include "orion/SettingsManager.h"

namespace Orion {

    namespace {
        constexpr const char* kInvalidFileNameChars = "\\/:*?\"<>|";
    }

    NewProjectDialog::NewProjectDialog(std::function<void(const NewProjectSettings&)> onCreate, std::function<void()> onCancel)
        : onCreateCallback(onCreate), onCancelCallback(onCancel)
    {
        const auto& ds = Orion::UI::DesignSystem::instance();

        addAndMakeVisible(titleLabel);
        titleLabel.setFont(ds.fonts().sans(ds.type.hero, juce::Font::bold));
        titleLabel.setJustificationType(juce::Justification::centred);


        addAndMakeVisible(nameLabel);
        addAndMakeVisible(nameEditor);
        Orion::UI::Primitives::styleSectionLabel(nameLabel);
        nameEditor.setText("Untitled Project");
        nameEditor.onTextChange = [this]() { updateCreateButtonState(); };


        addAndMakeVisible(pathLabel);
        addAndMakeVisible(pathEditor);
        Orion::UI::Primitives::styleSectionLabel(pathLabel);

        std::string defaultPath = SettingsManager::get().rootDataPath + "/Projects";
        if (defaultPath.empty() || defaultPath == "/Projects") {
             defaultPath = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Orion").getFullPathName().toStdString();
        }
        pathEditor.setText(defaultPath);
        pathEditor.onTextChange = [this]() { updateCreateButtonState(); };

        addAndMakeVisible(browseBtn);
        Orion::UI::Primitives::styleSecondaryButton(browseBtn);
        browseBtn.onClick = [this]() {
            fileChooser = std::make_unique<juce::FileChooser>("Select Project Location", juce::File::getSpecialLocation(juce::File::userDocumentsDirectory));
            auto folderFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories;

            fileChooser->launchAsync(folderFlags, [this](const juce::FileChooser& chooser) {
                auto result = chooser.getResult();
                if (result.exists()) {
                    pathEditor.setText(result.getFullPathName());
                    updateCreateButtonState();
                }
            });
        };


        addAndMakeVisible(authorLabel);
        addAndMakeVisible(authorEditor);
        Orion::UI::Primitives::styleSectionLabel(authorLabel);
        authorEditor.setText(juce::SystemStats::getLogonName());

        addAndMakeVisible(genreLabel);
        addAndMakeVisible(genreEditor);
        Orion::UI::Primitives::styleSectionLabel(genreLabel);


        addAndMakeVisible(templateLabel);
        addAndMakeVisible(templateBox);
        Orion::UI::Primitives::styleSectionLabel(templateLabel);
        templateBox.addItem("Empty Project", 1);
        templateBox.addItem("Basic Band Recording", 2);
        templateBox.addItem("Electronic Production", 3);
        templateBox.addItem("Orchestral Setup", 4);
        templateBox.setSelectedId(1);
        templateBox.onChange = [this]() { updateCreateButtonState(); };

        addAndMakeVisible(quickStartLabel);
        Orion::UI::Primitives::styleSectionLabel(quickStartLabel);

        addAndMakeVisible(quickStartTrackBox);
        quickStartTrackBox.addItem("Instrument Track", 1);
        quickStartTrackBox.addItem("Audio Track", 2);
        quickStartTrackBox.setSelectedId(1, juce::dontSendNotification);

        addAndMakeVisible(quickStartLoopBox);
        quickStartLoopBox.addItem("2 Bars Loop", 2);
        quickStartLoopBox.addItem("4 Bars Loop", 4);
        quickStartLoopBox.addItem("8 Bars Loop", 8);
        quickStartLoopBox.addItem("16 Bars Loop", 16);
        quickStartLoopBox.setSelectedId(8, juce::dontSendNotification);

        addAndMakeVisible(starterClipToggle);
        starterClipToggle.setButtonText("Create starter MIDI clip for instrument track");
        starterClipToggle.setToggleState(true, juce::dontSendNotification);


        addAndMakeVisible(bpmLabel);
        addAndMakeVisible(bpmSlider);
        Orion::UI::Primitives::styleSectionLabel(bpmLabel);
        bpmSlider.setRange(20, 300, 1);
        bpmSlider.setValue(120);
        bpmSlider.setTextValueSuffix(" BPM");
        bpmSlider.setSliderStyle(juce::Slider::LinearBarVertical);
        bpmSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 80, 20);

        addAndMakeVisible(srLabel);
        addAndMakeVisible(srBox);
        Orion::UI::Primitives::styleSectionLabel(srLabel);
        srBox.addItem("44100 Hz", 1);
        srBox.addItem("48000 Hz", 2);
        srBox.addItem("88200 Hz", 3);
        srBox.addItem("96000 Hz", 4);
        srBox.setSelectedId(1);
        srBox.onChange = [this]() { updateCreateButtonState(); };

        addAndMakeVisible(errorLabel);
        errorLabel.setColour(juce::Label::textColourId, ds.currentTheme().colors.danger.withAlpha(0.95f));
        errorLabel.setJustificationType(juce::Justification::centredLeft);
        errorLabel.setVisible(false);


        addAndMakeVisible(createBtn);
        Orion::UI::Primitives::stylePrimaryButton(createBtn);
        createBtn.onClick = [this]() {
            if (!validateInputs(true))
                return;

            NewProjectSettings s;
            s.name = getNormalizedProjectName().toStdString();
            s.path = pathEditor.getText().trim().toStdString();
            s.templateName = templateBox.getText().toStdString();
            s.bpm = (int)bpmSlider.getValue();
            s.sampleRate = srBox.getText().getIntValue();
            s.author = authorEditor.getText().toStdString();
            s.genre = genreEditor.getText().toStdString();
            s.quickStartTrackType = quickStartTrackBox.getSelectedId() == 2
                ? Orion::GuidedStartTrackType::Audio
                : Orion::GuidedStartTrackType::Instrument;
            s.quickStartLoopBars = quickStartLoopBox.getSelectedId() > 0 ? quickStartLoopBox.getSelectedId() : 8;
            s.quickStartCreateStarterClip = starterClipToggle.getToggleState();

            if (onCreateCallback) onCreateCallback(s);
        };

        addAndMakeVisible(cancelBtn);
        Orion::UI::Primitives::styleGhostButton(cancelBtn);
        cancelBtn.onClick = [this]() {
            if (onCancelCallback) onCancelCallback();
        };

        updateCreateButtonState();
    }

    NewProjectDialog::~NewProjectDialog() {}

    void NewProjectDialog::paint(juce::Graphics& g)
    {

        g.fillAll(juce::Colours::black.withAlpha(0.6f));


        auto area = getLocalBounds().toFloat().withSizeKeepingCentre(660, 560);

        juce::Path p;
        p.addRoundedRectangle(area.translated(0, 10), 12.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillPath(p);


        Orion::UI::Primitives::paintPanelSurface(g, area, 12.0f, false);


        g.setColour(juce::Colours::black.withAlpha(0.1f));
        g.drawHorizontalLine((int)area.getY() + 60, area.getX(), area.getRight());


        g.drawHorizontalLine((int)area.getBottom() - 70, area.getX(), area.getRight());


        g.setColour(Orion::UI::DesignSystem::instance().currentTheme().colors.border.withAlpha(0.8f));
        g.drawRoundedRectangle(area, 12.0f, 1.0f);
    }

    void NewProjectDialog::resized()
    {
        auto area = getLocalBounds().withSizeKeepingCentre(660, 560);
        auto mainArea = area;


        titleLabel.setBounds(mainArea.removeFromTop(60));


        auto footer = mainArea.removeFromBottom(70);
        footer.reduce(20, 15);

        auto errorArea = footer;
        errorArea.removeFromRight(260);
        errorLabel.setBounds(errorArea);

        createBtn.setBounds(footer.removeFromRight(140));
        footer.removeFromRight(10);
        cancelBtn.setBounds(footer.removeFromRight(100));


        mainArea.reduce(40, 20);

        int rowH = 30;
        int gap = 15;
        int labelW = 100;

        auto addRow = [&](juce::Label& l, juce::Component& c, int extraW = 0) {
            auto row = mainArea.removeFromTop(rowH);
            l.setBounds(row.removeFromLeft(labelW));
            c.setBounds(row.removeFromLeft(row.getWidth() - extraW));
            if (extraW > 0) return row;
            return juce::Rectangle<int>();
        };

        addRow(nameLabel, nameEditor);
        mainArea.removeFromTop(gap);

        auto pathRow = addRow(pathLabel, pathEditor, 40);
        browseBtn.setBounds(pathRow.removeFromRight(35));
        mainArea.removeFromTop(gap);


        addRow(authorLabel, authorEditor);
        mainArea.removeFromTop(gap);
        addRow(genreLabel, genreEditor);
        mainArea.removeFromTop(gap);


        mainArea.removeFromTop(10);


        addRow(templateLabel, templateBox);
        mainArea.removeFromTop(gap);

        auto quickRow = mainArea.removeFromTop(rowH);
        quickStartLabel.setBounds(quickRow.removeFromLeft(labelW));
        quickStartTrackBox.setBounds(quickRow.removeFromLeft(180));
        quickRow.removeFromLeft(10);
        quickStartLoopBox.setBounds(quickRow.removeFromLeft(160));
        mainArea.removeFromTop(8);
        starterClipToggle.setBounds(mainArea.removeFromTop(26).withTrimmedLeft(labelW + 2));
        mainArea.removeFromTop(gap);


        auto audioRow = mainArea.removeFromTop(rowH);
        bpmLabel.setBounds(audioRow.removeFromLeft(50));
        bpmSlider.setBounds(audioRow.removeFromLeft(160));

        audioRow.removeFromLeft(30);
        srLabel.setBounds(audioRow.removeFromLeft(90));
        srBox.setBounds(audioRow);
    }

    void NewProjectDialog::updateCreateButtonState()
    {
        createBtn.setEnabled(validateInputs(false));
    }

    bool NewProjectDialog::validateInputs(bool showErrors)
    {
        const auto projectName = getNormalizedProjectName();
        const auto projectPath = pathEditor.getText().trim();

        juce::String errorMessage;

        if (projectName.isEmpty()) {
            errorMessage = "Project name is required.";
        } else if (hasInvalidProjectNameChars(projectName)) {
            errorMessage = "Project name contains invalid filesystem characters.";
        } else if (projectPath.isEmpty()) {
            errorMessage = "Project location is required.";
        } else {
            juce::File directory(projectPath);
            if (!(directory.exists() && directory.isDirectory()) && !isDirectoryPathCreatable(directory)) {
                errorMessage = "Project location must be an existing folder or a creatable path.";
            }
        }

        const auto isValid = errorMessage.isEmpty();
        if (showErrors) {
            errorLabel.setText(errorMessage, juce::dontSendNotification);
            errorLabel.setVisible(!isValid);
        } else if (isValid) {
            errorLabel.setText({}, juce::dontSendNotification);
            errorLabel.setVisible(false);
        }

        return isValid;
    }

    juce::String NewProjectDialog::getNormalizedProjectName() const
    {
        auto name = nameEditor.getText().trim();
        name = name.replaceCharacters("\n\r\t", "   ");
        while (name.contains("  "))
            name = name.replace("  ", " ");
        return name;
    }

    bool NewProjectDialog::hasInvalidProjectNameChars(const juce::String& projectName) const
    {
        return projectName.containsAnyOf(kInvalidFileNameChars);
    }

    bool NewProjectDialog::isDirectoryPathCreatable(const juce::File& directory) const
    {
        if (directory.getFullPathName().trim().isEmpty())
            return false;

        juce::File current = directory;
        while (!current.exists()) {
            auto parent = current.getParentDirectory();
            if (parent == current)
                return false;
            current = parent;
        }

        return current.isDirectory() && current.hasWriteAccess();
    }

}
