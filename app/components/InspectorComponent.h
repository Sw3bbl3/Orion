#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/AudioClip.h"
#include "orion/AudioTrack.h"
#include "orion/PluginManager.h"

class InspectorComponent : public juce::Component,
                           public juce::DragAndDropTarget,
                           public juce::Timer
{
public:
    InspectorComponent(Orion::AudioEngine& engine);
    ~InspectorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void inspectClip(std::shared_ptr<Orion::AudioClip> clip);
    void inspectTrack(std::shared_ptr<Orion::AudioTrack> track);

    std::function<void()> onClipChanged;

    bool isInterestedInDragSource(const DragAndDropTarget::SourceDetails& dragSourceDetails) override;
    void itemDropped(const DragAndDropTarget::SourceDetails& dragSourceDetails) override;

private:
    class InspectorContent : public juce::Component,
                             public juce::ListBoxModel
    {
    public:
        InspectorContent(Orion::AudioEngine& engine, InspectorComponent& parent);
        ~InspectorContent() override;

        void resized() override;

        int getNumRows() override;
        void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
        void deleteKeyPressed(int lastRowSelected) override;
        void listBoxItemDoubleClicked(int row, const juce::MouseEvent& e) override;
        void listBoxItemClicked(int row, const juce::MouseEvent& e) override;

        void updateSelection(std::shared_ptr<Orion::AudioClip> clip, std::shared_ptr<Orion::AudioTrack> track);

        Orion::AudioEngine& engine;
        InspectorComponent& parent;
        std::shared_ptr<Orion::AudioClip> currentClip;
        std::shared_ptr<Orion::AudioTrack> currentTrack;

        juce::TextEditor nameEditor;
        juce::Slider volumeSlider;
        juce::Slider panSlider;
        std::unique_ptr<juce::Button> muteButton;
        std::unique_ptr<juce::Button> soloButton;
        std::unique_ptr<juce::Button> armButton;

        juce::Label effectsLabel;
        juce::ListBox effectsList;
        juce::TextButton addInsertButton { "+" };
        std::vector<std::shared_ptr<Orion::EffectNode>> currentEffects;

        void showAddInsertMenu();
        bool addInsertPlugin(const Orion::PluginInfo& info);

        juce::Label colorLabel;
        juce::ComboBox colorSelector;

        juce::Label gainLabel, pitchLabel, fineTuneLabel;
        juce::Slider clipGainSlider;
        juce::Slider clipPitchSlider;
        juce::Slider clipFineTuneSlider;
        juce::TextButton gainResetButton { "Reset" };
        juce::TextButton pitchResetButton { "Reset" };
        juce::ToggleButton preservePitchToggle;

        juce::Label syncLabel, bpmLabel;
        juce::ComboBox syncModeSelector;
        juce::TextEditor originalBpmField;

        juce::Label fadeInLabel, fadeOutLabel;
        juce::Slider fadeInSlider, fadeOutSlider;

        std::unique_ptr<juce::Button> reverseButton;
        std::unique_ptr<juce::Button> loopButton;

        juce::Label startLabel, lenLabel;
        juce::Label startVal, lenVal;

        juce::GroupComponent trackGroup, clipAudioGroup, clipTimeGroup;
    };

    Orion::AudioEngine& engine;
    juce::Viewport viewport;
    std::unique_ptr<InspectorContent> content;

    std::shared_ptr<Orion::AudioClip> currentClip;
    std::shared_ptr<Orion::AudioTrack> currentTrack;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InspectorComponent)
};
