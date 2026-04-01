#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/SamplerNode.h"

class SamplerEditor : public juce::Component
{
public:
    SamplerEditor(Orion::SamplerNode& node);
    ~SamplerEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Orion::SamplerNode& samplerNode;
    juce::TextButton loadButton { "Load Sample" };
    juce::Label sampleNameLabel;

    class WaveformPreview : public juce::Component, private juce::Timer
    {
    public:
        explicit WaveformPreview(Orion::SamplerNode& node);
        void paint(juce::Graphics& g) override;

    private:
        Orion::SamplerNode& samplerNode;
        void timerCallback() override;
        juce::Path waveformPath;
        void rebuildPath();
    };

    struct Knob {
        juce::Label label;
        juce::Slider slider;
    };

    WaveformPreview waveformPreview;

    Knob attackKnob;
    Knob decayKnob;
    Knob sustainKnob;
    Knob releaseKnob;

    Knob rootKnob;
    Knob tuneKnob;

    juce::Slider startSlider;
    juce::Slider endSlider;
    juce::Label startLabel;
    juce::Label endLabel;
    juce::ToggleButton loopToggle { "Loop" };

    void updateSampleName();
    void setupKnob(Knob& knob, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SamplerEditor)
};
