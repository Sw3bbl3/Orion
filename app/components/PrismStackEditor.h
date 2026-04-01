#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include "orion/PrismStackNode.h"

class PrismStackEditor : public juce::Component
{
public:
    PrismStackEditor(std::shared_ptr<Orion::PrismStackNode> node);
    ~PrismStackEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    std::shared_ptr<Orion::PrismStackNode> node;

    juce::Slider spreadSlider;
    juce::Label spreadLabel;

    juce::ComboBox chordSelector;
    juce::Label chordLabel;
};
