#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

class NativeHostingComponent : public juce::Component
{
public:
    NativeHostingComponent();
    ~NativeHostingComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void moved() override;


    void ensureNativeWindow();

    void* getNativeHandle();


    std::function<void(int w, int h)> onResizeRequest;

private:
    class NativeWindow : public juce::Component
    {
    public:
        NativeWindow(NativeHostingComponent& owner) : owner(owner) {
            setOpaque(true);
        }
        void paint(juce::Graphics& g) override {
            g.fillAll(juce::Colours::black);
        }
        void resized() override {
            if (owner.onResizeRequest) {


                int w = getWidth();
                int h = getHeight();
                juce::MessageManager::callAsync([this, w, h] {
                     if (owner.onResizeRequest) owner.onResizeRequest(w, h);
                });
            }
        }
    private:
        NativeHostingComponent& owner;
    };

    std::unique_ptr<NativeWindow> nativeWindow;
    bool isAttached = false;

    void updateNativeWindowBounds();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NativeHostingComponent)
};
