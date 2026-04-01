#pragma once
#include <JuceHeader.h>
#include "orion/AudioTrack.h"
#include "orion/GainNode.h"

class SendSlot : public juce::Component
{
public:
    SendSlot(std::shared_ptr<Orion::GainNode> gainNode, std::shared_ptr<Orion::AudioTrack> targetTrack);
    ~SendSlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;
    void focusGained(juce::Component::FocusChangeType cause) override;
    void focusLost(juce::Component::FocusChangeType cause) override;

    void setSelected(bool selected);
    bool isSelected() const { return selected; }
    void setTrackTint(juce::Colour tint) { trackTint = tint; repaint(); }

    std::function<void()> onSelect;
    std::function<void()> onDelete;

    std::shared_ptr<Orion::GainNode> getGainNode() { return gainNode; }
    std::shared_ptr<Orion::AudioTrack> getTargetTrack() { return targetTrack; }

private:
    std::shared_ptr<Orion::GainNode> gainNode;
    std::shared_ptr<Orion::AudioTrack> targetTrack;

    juce::Label nameLabel;
    juce::Slider fader;

    bool selected = false;
    juce::Colour trackTint { juce::Colours::transparentBlack };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SendSlot)
};
