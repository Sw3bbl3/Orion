#pragma once

#include <JuceHeader.h>
#include "orion/AudioClip.h"

class ClipComponent : public juce::Component, public juce::Timer, public juce::TooltipClient
{
public:
    ClipComponent(std::shared_ptr<Orion::AudioClip> clip);
    ~ClipComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    std::shared_ptr<Orion::AudioClip> getClip() const { return clip; }

    std::function<void(ClipComponent*)> onSelectionChanged;
    std::function<void(ClipComponent*, int deltaX)> onDrag;
    std::function<void(ClipComponent*)> onDragEnd;
    std::function<void(ClipComponent*)> onResizing;
    std::function<bool(ClipComponent*, const juce::MouseEvent&)> onSplit;
    std::function<void(ClipComponent*)> onDelete;
    std::function<void(ClipComponent*, juce::String)> onRename;


    std::function<bool()> isSplitToolActive;


    std::function<uint64_t(uint64_t)> snapFunction;

private:
    std::shared_ptr<Orion::AudioClip> clip;
    float lastColor[3] = { 0.0f, 0.0f, 0.0f };
    void timerCallback() override;
    bool selected = false;
    int lastMouseX = 0;
    juce::Point<int> lastMousePos;
    bool isMouseInside = false;

    enum DragMode { None, ResizeLeft, ResizeRight, FadeIn, FadeOut, Gain, Slip };
    DragMode dragMode = None;


    uint64_t originalStart = 0;
    uint64_t originalLength = 0;
    uint64_t originalSourceStart = 0;
    double cachedFramesPerPixel = 0.0;
    int dragStartX = 0;
    int dragStartY = 0;


    uint64_t originalFadeIn = 0;
    uint64_t originalFadeOut = 0;
    float originalGain = 1.0f;


    int splitX = -1;

    bool shouldNotifySelectionOnMouseUp = false;
    bool isListeningGesture = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClipComponent)
};
