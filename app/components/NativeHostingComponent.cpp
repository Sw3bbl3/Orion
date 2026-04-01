#include "NativeHostingComponent.h"

NativeHostingComponent::NativeHostingComponent()
{
    nativeWindow = std::make_unique<NativeWindow>(*this);
    setOpaque(true);
}

NativeHostingComponent::~NativeHostingComponent()
{

    if (nativeWindow)
    {
        nativeWindow->removeFromDesktop();
        nativeWindow = nullptr;
    }
}

void NativeHostingComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
}

void NativeHostingComponent::resized()
{
    updateNativeWindowBounds();
}

void NativeHostingComponent::moved()
{
    updateNativeWindowBounds();
}

void NativeHostingComponent::updateNativeWindowBounds()
{
    if (nativeWindow && isAttached)
    {
        auto area = getScreenBounds();

        if (auto* topLevel = getTopLevelComponent())
        {
            if (auto* peer = topLevel->getPeer())
            {
                auto parentScreen = topLevel->getScreenBounds();
                auto relX = area.getX() - parentScreen.getX();
                auto relY = area.getY() - parentScreen.getY();

                if (nativeWindow->getX() != relX || nativeWindow->getY() != relY ||
                    nativeWindow->getWidth() != area.getWidth() || nativeWindow->getHeight() != area.getHeight())
                {
                    nativeWindow->setBounds(relX, relY, area.getWidth(), area.getHeight());
                }
            }
        }
    }
}

void NativeHostingComponent::ensureNativeWindow()
{
    if (isAttached) return;

    if (auto* topLevel = getTopLevelComponent())
    {
        if (auto* peer = topLevel->getPeer())
        {
            auto parentHandle = peer->getNativeHandle();
            if (parentHandle)
            {


                nativeWindow->addToDesktop(0, parentHandle);
                nativeWindow->setVisible(true);
                isAttached = true;

                updateNativeWindowBounds();
            }
        }
    }
}

void* NativeHostingComponent::getNativeHandle()
{
    if (!isAttached) ensureNativeWindow();

    if (nativeWindow) return nativeWindow->getWindowHandle();
    return nullptr;
}
