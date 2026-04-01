#pragma once

#include <JuceHeader.h>
#include "orion/MidiClip.h"

class MidiClipComponent : public juce::Component, public juce::Timer, public juce::TooltipClient
{
public:
    MidiClipComponent(std::shared_ptr<Orion::MidiClip> clip);
    ~MidiClipComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    juce::String getTooltip() override;

    void setSelected(bool selected);
    bool isSelected() const { return selected; }

    std::shared_ptr<Orion::MidiClip> getClip() const { return clip; }

    std::function<void(MidiClipComponent*)> onSelectionChanged;
    std::function<void(MidiClipComponent*, int deltaX)> onDrag;
    std::function<void(MidiClipComponent*)> onResizing;
    std::function<void(MidiClipComponent*)> onDoubleClick;
    std::function<bool(MidiClipComponent*, const juce::MouseEvent&)> onSplit;
    std::function<void(MidiClipComponent*)> onDelete;
    std::function<void(MidiClipComponent*, juce::String)> onRename;


    std::function<bool()> isSplitToolActive;


    std::function<uint64_t(uint64_t)> snapFunction;

private:
    std::shared_ptr<Orion::MidiClip> clip;
    float lastColor[3] = { 0.0f, 0.0f, 0.0f };
    void timerCallback() override;
    bool selected = false;
    int lastMouseX = 0;
    juce::Point<int> lastMousePos;
    bool isMouseInside = false;

    enum ResizeMode { None, Left, Right };
    ResizeMode resizeMode = None;
    bool isStretching = false;
    double currentStretchFactor = 1.0;

    uint64_t originalStart = 0;
    uint64_t originalLength = 0;
    uint64_t originalOffset = 0;
    std::vector<Orion::MidiNote> originalNotes;

    double cachedFramesPerPixel = 0.0;
    int dragStartX = 0;


    int splitX = -1;

    bool shouldNotifySelectionOnMouseUp = false;
    bool isListeningGesture = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MidiClipComponent)
};
