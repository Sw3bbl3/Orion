#pragma once

#include <JuceHeader.h>
#include "CustomTitleBar.h"

class WindowWrapper : public juce::Component
{
public:
    WindowWrapper(juce::Component* contentToWrap,
                  const juce::String& title,
                  bool showMenu = false,
                  bool showMinMax = true,
                  bool ownsContent = true);
    ~WindowWrapper() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void parentHierarchyChanged() override;

    CustomTitleBar& getTitleBar() { return titleBar; }

private:
    CustomTitleBar titleBar;


    juce::Component* contentComponent = nullptr;


    std::unique_ptr<juce::Component> contentOwner;

    std::unique_ptr<juce::ResizableBorderComponent> windowResizer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WindowWrapper)
};
