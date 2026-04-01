#pragma once

#include <JuceHeader.h>
#include "orion/AudioEngine.h"

class RulerComponent : public juce::Component, public juce::Timer
{
public:
    RulerComponent(Orion::AudioEngine& engine);
    ~RulerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setPixelsPerSecond(double pps);

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

    std::function<void()> onCursorMove;
    std::function<uint64_t(uint64_t)> snapFunction;

private:
    Orion::AudioEngine& engine;
    double pixelsPerSecond = 50.0;


    int draggedMarkerIndex = -1;
    uint64_t draggedMarkerPos = 0;
    std::string draggedMarkerName;

    int hitTestMarker(int x);


    double getXForFrame(uint64_t frame) const;
    uint64_t getFrameForX(double x) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RulerComponent)
};
