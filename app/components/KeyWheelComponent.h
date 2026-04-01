#pragma once

#include <JuceHeader.h>
#include "orion/AudioEngine.h"

class KeyWheelComponent : public juce::Component
{
public:
    KeyWheelComponent(Orion::AudioEngine& engine, std::function<void()> onKeyChanged);
    ~KeyWheelComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;

private:
    Orion::AudioEngine& engine;
    std::function<void()> onKeyChanged;

    struct KeyInfo {
        juce::String name;
        float angle; // centre angle in radians
    };

    std::vector<KeyInfo> keys;
    int hoveredIndex = -1;
    int selectedIndex = -1;

    void setupKeys();
    int getIndexAt(juce::Point<int> p);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(KeyWheelComponent)
};
