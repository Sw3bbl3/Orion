#pragma once

#include <JuceHeader.h>
#include <functional>

class CustomTitleBar : public juce::Component
{
public:
    CustomTitleBar();
    ~CustomTitleBar() override;

    void paint(juce::Graphics& g) override;
    void resized() override;


    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDoubleClick(const juce::MouseEvent& event) override;


    void setModel(juce::MenuBarModel* model);
    void setTitle(const juce::String& title);

    void setShowMenuBar(bool show);
    void setShowMinMax(bool show);
    void setShowPin(bool show);
    void setShowViewButtons(bool show);
    void setViewButtonStates(bool showEditor, bool showMixer, bool showPianoRoll, bool showBrowser);
    void setProjectName(const juce::String& name);
    void setOnClose(std::function<void()> callback);
    void setOnPinToggled(std::function<void(bool)> callback);
    void setOnToggleEditor(std::function<void()> callback);
    void setOnToggleMixer(std::function<void()> callback);
    void setOnTogglePianoRoll(std::function<void()> callback);
    void setOnToggleBrowser(std::function<void()> callback);

private:
    class TitleBarButton : public juce::Button
    {
    public:
        enum Type { Min, Max, Close, Pin };
        TitleBarButton(const juce::String& name, Type type);
        void paintButton(juce::Graphics& g, bool isMouseOverButton, bool isButtonDown) override;
    private:
        Type type;
    };

    juce::MenuBarComponent menuBar;
    juce::Label titleLabel;

    std::unique_ptr<juce::Drawable> logoDrawable;

    TitleBarButton minButton;
    TitleBarButton maxButton;
    TitleBarButton closeButton;
    TitleBarButton pinButton;

    juce::ComponentDragger dragger;

    bool showMenuBar = true;
    bool showMinMax = true;
    bool showPin = false;
    bool showViewButtons = false;
    std::function<void()> onCloseCallback;
    std::function<void(bool)> onPinCallback;
    std::function<void()> onToggleEditorCallback;
    std::function<void()> onToggleMixerCallback;
    std::function<void()> onTogglePianoRollCallback;
    std::function<void()> onToggleBrowserCallback;


    juce::ResizableWindow* getWindow();
    void toggleMaximized();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CustomTitleBar)
};
