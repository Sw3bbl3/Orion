#pragma once
#include <JuceHeader.h>

class DockablePanel : public juce::Component
{
public:
    DockablePanel(const juce::String& title, juce::Component& content, std::function<void()> onClose, std::function<void()> onDetach = nullptr);
    ~DockablePanel() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void lookAndFeelChanged() override;

private:
    juce::Component& content;
    juce::Label titleLabel;
    juce::TextButton closeButton;
    juce::ShapeButton detachButton { "Detach", juce::Colours::transparentBlack, juce::Colours::transparentBlack, juce::Colours::transparentBlack };

    std::function<void()> onCloseCallback;
    std::function<void()> onDetachCallback;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DockablePanel)
};
