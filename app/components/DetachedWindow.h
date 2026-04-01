#pragma once
#include <JuceHeader.h>
#include "WindowWrapper.h"
#include "../ui/OrionDesignSystem.h"

class DetachedWindow : public juce::DocumentWindow
{
public:
    DetachedWindow(const juce::String& name, juce::Component* content, std::function<void()> onClose)
        : DocumentWindow(name, Orion::UI::DesignSystem::instance().currentTheme().colors.appBg, DocumentWindow::allButtons),
          onCloseCallback(onClose)
    {
        setUsingNativeTitleBar(false);
        setTitleBarHeight(0);

        // Enforce minimal initial size for detached windows to keep them compact but usable
        int minW = 300;
        int minH = 200;

        // If content has preferred size, use it, but constrained
        if (content) {
            minW = juce::jmax(minW, content->getWidth());
            minH = juce::jmax(minH, content->getHeight());
        }

        auto wrapper = std::make_unique<WindowWrapper>(content, name, false, true, false);

        setContentOwned(wrapper.release(), true);

        setResizable(true, false);
        setResizeLimits(200, 150, 1920, 1080); // Set hard limits for minimal size

        centreWithSize(minW, minH + 30); // +30 for title bar
        setVisible(true);
    }

    void closeButtonPressed() override
    {
        if (onCloseCallback)
            onCloseCallback();
    }

private:
    std::function<void()> onCloseCallback;
};
