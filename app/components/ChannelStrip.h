#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/AudioTrack.h"
#include "orion/MasterNode.h"
#include "orion/CommandHistory.h"
#include "../OrionIcons.h"
#include "InsertSlot.h"
#include "SendSlot.h"

namespace Orion { struct PluginInfo; }

class MeterComponent : public juce::Component
{
public:
    void setLevel(float l, float r);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    void resetPeak();
    bool showPeakText = false;

private:
    float levelL = 0.0f;
    float levelR = 0.0f;
    float peakHold = 0.0f;
    int peakHoldCounter = 0;
    float maxPeak = -100.0f;
    int clipHoldCounter = 0;
};

class ChannelStrip : public juce::Component,
                     public juce::Timer,
                     public juce::DragAndDropTarget,
                     public juce::ChangeListener
{
public:

    ChannelStrip(Orion::AudioEngine& engine, Orion::CommandHistory& history, std::shared_ptr<Orion::AudioTrack> track);

    ChannelStrip(Orion::AudioEngine& engine, Orion::CommandHistory& history, Orion::MasterNode* masterNode);

    ~ChannelStrip() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void changeListenerCallback(juce::ChangeBroadcaster* source) override;

    void setSelected(bool s) { selected = s; repaint(); }
    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onSelected;
    std::shared_ptr<Orion::AudioTrack> getTrack() const { return track; }

    bool isInterestedInDragSource(const SourceDetails& dragSourceDetails) override;
    void itemDropped(const SourceDetails& dragSourceDetails) override;

    void lookAndFeelChanged() override;

    void updateInserts();

private:
    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;
    std::shared_ptr<Orion::AudioTrack> track;
    Orion::MasterNode* masterNode = nullptr;

    bool isMaster = false;
    bool selected = false;

    juce::Slider fader;
    juce::Slider panKnob;
    juce::Label nameLabel;
    juce::Label valueLabel;
    juce::ComboBox inputSelector;
    juce::ComboBox outputSelector;


    std::unique_ptr<juce::Button> muteButton;
    std::unique_ptr<juce::Button> soloButton;
    std::unique_ptr<juce::Button> recButton;
    std::unique_ptr<juce::Button> monoButton;
    std::unique_ptr<juce::Button> phaseButton;


    juce::Viewport insertsViewport;
    juce::Component insertsContainer;
    std::vector<std::unique_ptr<InsertSlot>> insertSlots;
    juce::TextButton addInsertButton { "+" };


    juce::Viewport sendsViewport;
    juce::Component sendsContainer;
    std::vector<std::unique_ptr<SendSlot>> sendSlots;
    juce::TextButton addSendButton { "+" };


    std::unique_ptr<juce::Button> monitoringButton;
    std::unique_ptr<juce::Button> monitoringSettingsButton;
    void showMonitoringSettings();

    void handleSendSelection(SendSlot* slot);
    void deleteSend(SendSlot* slot);


    std::unique_ptr<InsertSlot> instrumentSlot;


    MeterComponent meter;

    float lastColor[3] = {0.0f, 0.0f, 0.0f};

    void setupControls();
    void setupButton(std::unique_ptr<juce::Button>& btn, const juce::String& text, juce::Colour onColour, const juce::String& tooltip);
    void updateOutputRouting();
    void updateSends();
    void showAddInsertMenu();
    void showStripContextMenu();
    void attachContextMenuForwarding(juce::Component& component);
    bool addInsertPlugin(const Orion::PluginInfo& info);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ChannelStrip)
};
