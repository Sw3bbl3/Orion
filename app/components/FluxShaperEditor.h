#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/FluxShaperNode.h"

class FluxShaperEditor : public juce::Component, public juce::Timer
{
public:
    FluxShaperEditor(std::shared_ptr<Orion::FluxShaperNode> node);
    ~FluxShaperEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    std::shared_ptr<Orion::FluxShaperNode> node;
    std::vector<Orion::FluxShaperNode::Point> points;

    int draggedPointIndex = -1;

    juce::Slider mixSlider;
    juce::Label mixLabel;

    juce::Path curvePath;
    void updatePath();

    // Coordinate transforms
    float toX(float phase) { return phase * getWidth(); }
    float toY(float gain) { return getHeight() - (gain * getHeight()); }
    float fromX(float x) { return juce::jlimit(0.0f, 1.0f, x / getWidth()); }
    float fromY(float y) { return juce::jlimit(0.0f, 1.0f, 1.0f - (y / getHeight())); }
};
