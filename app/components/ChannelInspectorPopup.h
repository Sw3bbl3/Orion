#pragma once

#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/AudioTrack.h"
#include "orion/CommandHistory.h"
#include "orion/PluginManager.h"
#include "InsertSlot.h"
#include "SendSlot.h"

class ChannelInspectorPopup : public juce::Component,
                             public juce::Timer
{
public:
    ChannelInspectorPopup(Orion::AudioEngine& engine,
                         Orion::CommandHistory& history,
                         std::shared_ptr<Orion::AudioTrack> track);
    ~ChannelInspectorPopup() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    void updateInserts();
    void updateSends();
    void updateOutputRouting();
    void updateInputSelector();
    void showAddInsertMenu();
    bool addInsertPlugin(const Orion::PluginInfo& info);

    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;
    std::shared_ptr<Orion::AudioTrack> track;

    juce::Label titleLabel;
    juce::TextEditor nameEditor;

    juce::Slider volumeSlider;
    juce::Slider panSlider;
    std::unique_ptr<juce::Button> muteButton;
    std::unique_ptr<juce::Button> soloButton;
    std::unique_ptr<juce::Button> armButton;
    std::unique_ptr<juce::Button> phaseButton;
    std::unique_ptr<juce::Button> monoButton;
    juce::ComboBox inputSelector;
    juce::ComboBox outputSelector;
    juce::Label volLabel, panLabel;

    juce::Viewport insertsViewport;
    juce::Component insertsContainer;
    std::vector<std::unique_ptr<InsertSlot>> insertSlots;
    juce::TextButton addInsertButton { "+" };

    juce::Viewport sendsViewport;
    juce::Component sendsContainer;
    std::vector<std::unique_ptr<SendSlot>> sendSlots;
    juce::TextButton addSendButton { "+" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelInspectorPopup)
};
