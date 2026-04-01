#include "WindowWrapper.h"
#include "../ui/OrionDesignSystem.h"
#include "../ui/OrionUIPrimitives.h"

WindowWrapper::WindowWrapper(juce::Component* contentToWrap,
                             const juce::String& title,
                             bool showMenu,
                             bool showMinMax,
                             bool ownsContent)
    : contentComponent(contentToWrap)
{
    if (ownsContent && contentToWrap)
    {
        contentOwner.reset(contentToWrap);
    }


    if (contentComponent)
    {
        addAndMakeVisible(contentComponent);
    }


    addAndMakeVisible(titleBar);
    titleBar.setTitle(title);
    titleBar.setShowMenuBar(showMenu);
    titleBar.setShowMinMax(showMinMax);

    setSize(600, 400);
}

WindowWrapper::~WindowWrapper()
{


}

void WindowWrapper::paint(juce::Graphics& g)
{
    Orion::UI::Primitives::paintWindowBackdrop(g, getLocalBounds().toFloat());
    g.setColour(Orion::UI::DesignSystem::instance().currentTheme().colors.border.withAlpha(0.8f));
    g.drawRect(getLocalBounds(), 1);
}

void WindowWrapper::resized()
{
    auto area = getLocalBounds();

    if (windowResizer)
    {
        windowResizer->setBounds(getLocalBounds());
        windowResizer->toFront(false);
    }


    titleBar.setBounds(area.removeFromTop(34));
    titleBar.toFront(false);


    if (contentComponent)
    {
        contentComponent->setBounds(area);
    }


    if (windowResizer)
    {
        windowResizer->toFront(false);
    }
}

void WindowWrapper::parentHierarchyChanged()
{
    if (auto* w = getTopLevelComponent())
    {
        bool resizable = true;
        if (auto* rw = dynamic_cast<juce::ResizableWindow*>(w))
        {
            resizable = rw->isResizable();
        }

        if (resizable && !windowResizer)
        {
            windowResizer = std::make_unique<juce::ResizableBorderComponent>(w, nullptr);
            windowResizer->setBorderThickness(juce::BorderSize<int>(4));
            addAndMakeVisible(windowResizer.get());
            windowResizer->toFront(false);
        }
    }
}
