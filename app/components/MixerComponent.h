#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/CommandHistory.h"
#include "ChannelStrip.h"

class MixerComponent : public juce::Component, public juce::Timer
{
public:
    MixerComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~MixerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void updateTrackList();

    void refresh() {
        updateTrackList();
    }

    std::function<void(std::shared_ptr<Orion::AudioTrack>)> onTrackSelected;
    void setSelectedTrack(std::shared_ptr<Orion::AudioTrack> track);

    int getScrollX() const { return viewport.getViewPositionX(); }
    void setViewPosition(int x) { viewport.setViewPosition(x, 0); }

private:
    Orion::AudioEngine& engine;
    Orion::CommandHistory& commandHistory;

    std::shared_ptr<Orion::AudioTrack> selectedTrack;

    juce::Viewport viewport;
    juce::Component content;

    std::vector<std::unique_ptr<ChannelStrip>> strips;


    std::unique_ptr<ChannelStrip> masterStrip;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MixerComponent)
};
