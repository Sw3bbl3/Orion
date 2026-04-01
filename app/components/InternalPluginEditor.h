#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/EffectNode.h"
#include "orion/AudioEngine.h"
#include "orion/AudioTrack.h"
#include "SamplerEditor.h"
#include "SynthEditor.h"
#include <memory>
#include <vector>

class InternalPluginEditor : public juce::Component
{
public:
    InternalPluginEditor(std::shared_ptr<Orion::EffectNode> node,
                         Orion::AudioEngine* engine = nullptr,
                         Orion::AudioTrack* ownerTrack = nullptr);
    ~InternalPluginEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    std::shared_ptr<Orion::EffectNode> effectNode;


    class ParameterControl : public juce::Component
    {
    public:
        ParameterControl(const juce::String& name);
        void resized() override;

        juce::Label label;
        juce::Slider slider;
    };

    std::vector<std::unique_ptr<ParameterControl>> controls;


    std::unique_ptr<SamplerEditor> samplerEditor;
    std::unique_ptr<SynthEditor> synthEditor;
    std::unique_ptr<juce::Component> pluginVisualizer;


    std::unique_ptr<juce::ComboBox> sidechainSelector;
    std::unique_ptr<juce::Label> sidechainLabel;
    std::vector<Orion::AudioTrack*> sidechainTrackOptions;

    Orion::AudioEngine* engine = nullptr;
    Orion::AudioTrack* ownerTrack = nullptr;

    void createControls();
    void rebuildSidechainSelector();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InternalPluginEditor)
};
