#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "../CommandManager.h"
#include "KeyWheelComponent.h"

class HeaderComponent : public juce::Component,
                        public juce::Timer
{
public:
    HeaderComponent(Orion::AudioEngine& engine, Orion::CommandManager& commandManager);
    ~HeaderComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;


    std::function<int()> getCurrentToolId;

private:
    Orion::AudioEngine& engine;
    Orion::CommandManager& commandManager;


    std::unique_ptr<juce::Button> playButton;
    std::unique_ptr<juce::Button> stopButton;
    std::unique_ptr<juce::Button> recButton;
    std::unique_ptr<juce::Button> loopButton;
    std::unique_ptr<juce::Button> rewindButton;
    std::unique_ptr<juce::Button> ffButton;
    std::unique_ptr<juce::Button> panicButton;
    std::unique_ptr<juce::Button> metroButton;


    std::unique_ptr<juce::Button> toolSelect;
    std::unique_ptr<juce::Button> toolDraw;
    std::unique_ptr<juce::Button> toolErase;
    std::unique_ptr<juce::Button> toolSplit;
    std::unique_ptr<juce::Button> toolGlue;
    std::unique_ptr<juce::Button> toolZoom;
    std::unique_ptr<juce::Button> toolMute;
    std::unique_ptr<juce::Button> toolListen;


    class LiveWaveformComponent;
    std::unique_ptr<LiveWaveformComponent> waveform;

    juce::Label timeDisplay;
    juce::Label keyDisplay;

    class PerformanceDisplayComponent;
    std::unique_ptr<PerformanceDisplayComponent> perfDisplay;

    juce::Label bpmLabel;
    juce::Label bpmDisplay;

    float bpmAtDragStart = 0.0f;

    void createButton(std::unique_ptr<juce::Button>& btn, const juce::String& tooltip, const juce::Path& icon, int commandID, juce::Colour colour = juce::Colours::white);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HeaderComponent)
};
