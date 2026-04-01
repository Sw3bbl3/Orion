#pragma once

#include <JuceHeader.h>
#include <functional>

class StatusBarComponent : public juce::Component, public juce::Timer
{
public:
    StatusBarComponent();
    ~StatusBarComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void setViewButtonStates(bool showInspector, bool showMixer, bool showPianoRoll, bool showBrowser);

    void setOnToggleInspector(std::function<void()> callback) { onToggleInspector = callback; }
    void setOnToggleMixer(std::function<void()> callback) { onToggleMixer = callback; }
    void setOnTogglePianoRoll(std::function<void()> callback) { onTogglePianoRoll = callback; }
    void setOnToggleBrowser(std::function<void()> callback) { onToggleBrowser = callback; }

private:
    class StatusButton : public juce::Button
    {
    public:
        StatusButton(const juce::String& name, const juce::Path& icon);
        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
    private:
        juce::Path icon;
    };

    juce::Label hintLabel;

    std::unique_ptr<StatusButton> inspectorButton;
    std::unique_ptr<StatusButton> mixerButton;
    std::unique_ptr<StatusButton> pianoRollButton;
    std::unique_ptr<StatusButton> browserButton;

    std::function<void()> onToggleInspector;
    std::function<void()> onToggleMixer;
    std::function<void()> onTogglePianoRoll;
    std::function<void()> onToggleBrowser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StatusBarComponent)
};
