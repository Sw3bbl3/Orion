#pragma once

#include <JuceHeader.h>
#include "orion/AudioEngine.h"

class MarkerListComponent : public juce::Component,
                            public juce::ListBoxModel,
                            public juce::Timer
{
public:
    MarkerListComponent(Orion::AudioEngine& engine);
    ~MarkerListComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;


    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void deleteKeyPressed(int lastRowSelected) override;

    void timerCallback() override;

    void updateContent();

private:
    Orion::AudioEngine& engine;
    juce::ListBox listBox;
    std::vector<Orion::AudioEngine::RegionMarker> markers;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MarkerListComponent)
};
