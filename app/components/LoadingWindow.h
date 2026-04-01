#pragma once
#include <JuceHeader.h>

class LoadingWindow : public juce::DocumentWindow
{
public:
    LoadingWindow();

    void closeButtonPressed() override {}

    void show();
    void hide();

private:
    class Content;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LoadingWindow)
};
