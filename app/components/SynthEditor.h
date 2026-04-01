#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/SynthesizerNode.h"

class SynthEditor : public juce::Component
{
public:
    explicit SynthEditor(Orion::SynthesizerNode& node);
    ~SynthEditor() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Orion::SynthesizerNode& synthNode;

    struct Knob {
        juce::Label label;
        juce::Slider slider;
    };

    class WavePreview : public juce::Component, private juce::Timer
    {
    public:
        explicit WavePreview(Orion::SynthesizerNode& node);
        void paint(juce::Graphics& g) override;

    private:
        Orion::SynthesizerNode& synthNode;
        float phase = 0.0f;
        void timerCallback() override;
    };

    juce::Label titleLabel;
    juce::ComboBox waveformBox;
    juce::Label waveformLabel;

    Knob cutoffKnob;
    Knob resonanceKnob;
    Knob attackKnob;
    Knob decayKnob;
    Knob sustainKnob;
    Knob releaseKnob;
    Knob lfoRateKnob;
    Knob lfoDepthKnob;

    juce::ComboBox lfoTargetBox;
    juce::Label lfoTargetLabel;

    WavePreview wavePreview;

    void setupKnob(Knob& knob, const juce::String& name);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SynthEditor)
};
