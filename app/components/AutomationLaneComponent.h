#pragma once

#include <JuceHeader.h>
#include "orion/Automation.h"
#include "orion/AudioTrack.h"

class AutomationLaneComponent : public juce::Component
{
public:
    AutomationLaneComponent(std::shared_ptr<Orion::AudioTrack> track, Orion::AutomationParameter parameter);
    ~AutomationLaneComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    void setPixelsPerSecond(double pps);
    void setSampleRate(double sr) { sampleRate = sr; }

private:
    std::shared_ptr<Orion::AudioTrack> track;
    Orion::AutomationParameter parameter;
    Orion::AutomationParameterSpec parameterSpec;
    double pixelsPerSecond = 50.0;
    double sampleRate = 44100.0;

    int draggedPointIndex = -1;
    int selectedPointIndex = -1;

    int findPointAtPosition(const Orion::AutomationLane& lane, int x, int y) const;
    void showContextMenu(const juce::MouseEvent& e);
    void clearCurrentLane();

    std::shared_ptr<Orion::AutomationLane> getCurrentLane();


    void commitLane(std::shared_ptr<Orion::AutomationLane> lane);

    float valueToY(float value) const;
    float yToValue(float y) const;

    uint64_t xToFrame(int x) const;
    int frameToX(uint64_t frame) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AutomationLaneComponent)
};
