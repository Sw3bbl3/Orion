#pragma once
#include <JuceHeader.h>
#include "../ProjectManager.h"
#include "../OrionLookAndFeel.h"
#include "orion/UxState.h"

namespace Orion {

    struct NewProjectSettings {
        std::string name;
        std::string path;
        std::string templateName;
        int bpm;
        int sampleRate;
        std::string author;
        std::string genre;
        Orion::GuidedStartTrackType quickStartTrackType = Orion::GuidedStartTrackType::Instrument;
        int quickStartLoopBars = 8;
        bool quickStartCreateStarterClip = true;
    };

    class NewProjectDialog : public juce::Component
    {
    public:
        NewProjectDialog(std::function<void(const NewProjectSettings&)> onCreate, std::function<void()> onCancel);
        ~NewProjectDialog() override;

        void paint(juce::Graphics& g) override;
        void resized() override;

    private:
        void updateCreateButtonState();
        bool validateInputs(bool showErrors);
        juce::String getNormalizedProjectName() const;
        bool hasInvalidProjectNameChars(const juce::String& projectName) const;
        bool isDirectoryPathCreatable(const juce::File& directory) const;

        std::function<void(const NewProjectSettings&)> onCreateCallback;
        std::function<void()> onCancelCallback;

        juce::Label titleLabel { "New Project", "New Project" };

        juce::Label nameLabel { "Name", "Project Name:" };
        juce::TextEditor nameEditor;

        juce::Label pathLabel { "Location", "Location:" };
        juce::TextEditor pathEditor;
        juce::TextButton browseBtn { "..." };

        juce::Label authorLabel { "Author", "Author:" };
        juce::TextEditor authorEditor;

        juce::Label genreLabel { "Genre", "Genre:" };
        juce::TextEditor genreEditor;

        juce::Label templateLabel { "Template", "Template:" };
        juce::ComboBox templateBox;
        juce::Label quickStartLabel { "QuickStart", "Quick Start:" };
        juce::ComboBox quickStartTrackBox;
        juce::ComboBox quickStartLoopBox;
        juce::ToggleButton starterClipToggle;

        juce::Label bpmLabel { "BPM", "BPM:" };
        juce::Slider bpmSlider;

        juce::Label srLabel { "SampleRate", "Sample Rate:" };
        juce::ComboBox srBox;

        juce::TextButton createBtn { "Create Project" };
        juce::TextButton cancelBtn { "Cancel" };
        juce::Label errorLabel;

        std::unique_ptr<juce::FileChooser> fileChooser;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NewProjectDialog)
    };

}
