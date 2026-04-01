#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/EnvironmentNode.h"

class AdaptiveSpaceWindow : public juce::Component {
public:
    explicit AdaptiveSpaceWindow(Orion::EnvironmentNode* node);
    ~AdaptiveSpaceWindow() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    struct Snapshot {
        bool hasData = false;
        bool active = false;
        Orion::EnvironmentNode::Preset preset = Orion::EnvironmentNode::Preset::Studio;
        float size = 0.0f;
        float crowd = 0.0f;
        float ambience = 0.0f;
        float intensity = 0.0f;
        float tone = 0.0f;
        float width = 0.0f;
        float grit = 0.0f;
        float preDelay = 0.0f;
        float outputTrim = 0.5f;
        bool autoGain = true;
    };

    Orion::EnvironmentNode* node;


    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label versionLabel;
    juce::ToggleButton activeToggle { "Enable Adaptive Space" };


    juce::Label presetsLabel;
    juce::TextButton studioBtn { "Studio" };
    juce::TextButton smallRoomBtn { "Small Room" };
    juce::TextButton hallBtn { "Hall" };
    juce::TextButton clubBtn { "Club" };
    juce::TextButton carBtn { "Car" };
    juce::TextButton outdoorBtn { "Outdoor" };
    juce::TextButton phoneBtn { "Phone" };

    std::vector<juce::TextButton*> presetButtons;

    juce::Label quickMixLabel;
    juce::TextButton vocalBtn { "Vocal" };
    juce::TextButton drumsBtn { "Drums" };
    juce::TextButton busBtn { "Mix Bus" };
    juce::TextButton fxBtn { "FX Send" };


    juce::Slider sizeKnob;
    juce::Label sizeLabel;

    juce::Slider crowdKnob;
    juce::Label crowdLabel;

    juce::Slider ambienceKnob;
    juce::Label ambienceLabel;

    juce::Slider intensityKnob;
    juce::Label intensityLabel;

    juce::Slider toneKnob;
    juce::Label toneLabel;

    juce::Slider widthKnob;
    juce::Label widthLabel;

    juce::Slider gritKnob;
    juce::Label gritLabel;

    juce::Slider preDelayKnob;
    juce::Label preDelayLabel;

    juce::Slider trimKnob;
    juce::Label trimLabel;

    juce::ToggleButton loudnessMatchToggle { "Loudness Match" };
    juce::TextButton resetBtn { "Reset" };
    juce::TextButton storeABtn { "Store A" };
    juce::TextButton recallABtn { "Recall A" };
    juce::TextButton storeBBtn { "Store B" };
    juce::TextButton recallBBtn { "Recall B" };

    Snapshot snapshotA;
    Snapshot snapshotB;

    void refreshFromNode();
    void storeSnapshot(Snapshot& snapshot);
    void loadSnapshot(const Snapshot& snapshot);
    void applyQuickMixProfile(const juce::String& profile);
    void updateButtonStates();
    void setupKnob(juce::Slider& s, juce::Label& l, const juce::String& name);
};
