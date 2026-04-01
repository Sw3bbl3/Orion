#pragma once

#include <JuceHeader.h>
#include "orion/UxState.h"

class GuidedStartWizard : public juce::Component
{
public:
    GuidedStartWizard();

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setState(const Orion::GuidedStartState& state);
    Orion::GuidedStartState getState() const;

    std::function<void(const Orion::GuidedStartState&)> onApply;
    std::function<void()> onDismiss;

private:
    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Label trackTypeLabel;
    juce::ComboBox trackTypeBox;

    juce::Label bpmLabel;
    juce::Slider bpmSlider;
    juce::Label bpmValueLabel;

    juce::Label loopBarsLabel;
    juce::ComboBox loopBarsBox;

    juce::TextButton createButton { "Create Setup" };
    juce::TextButton dismissButton { "Not Now" };

    void syncBpmLabel();
};

