#pragma once
#include <JuceHeader.h>
#include "../OrionLookAndFeel.h"

class OrionSplashScreen : public juce::Component, public juce::Timer
{
public:
    OrionSplashScreen();
    ~OrionSplashScreen() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setStatus(const juce::String& status);
    void setProgress(float progress);

private:
    juce::Image logo;
    juce::String statusText;
    float currentProgress = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OrionSplashScreen)
};
