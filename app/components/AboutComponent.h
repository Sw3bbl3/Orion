#pragma once
#include <JuceHeader.h>

class AboutComponent : public juce::Component, public juce::Timer
{
public:
    AboutComponent();
    ~AboutComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

private:
    juce::Image logo;
    float gradientOffset = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutComponent)
};
