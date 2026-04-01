#pragma once
#include <JuceHeader.h>
#include "orion/EffectNode.h"

class InsertSlot : public juce::Component,
                  public juce::Timer
{
public:
    InsertSlot(std::shared_ptr<Orion::EffectNode> effect);
    ~InsertSlot() override;

    void paint(juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
    void mouseUp(const juce::MouseEvent& event) override;

    std::shared_ptr<Orion::EffectNode> getEffect() { return effect; }
    void setTrackTint(juce::Colour tint) { trackTint = tint; repaint(); }

    std::function<void()> onOpenEditor;
    std::function<void()> onRemove;
    std::function<void(juce::PopupMenu&)> onMenu;

private:
    std::shared_ptr<Orion::EffectNode> effect;
    juce::Colour trackTint { juce::Colours::transparentBlack };

    std::unique_ptr<juce::Button> powerButton;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(InsertSlot)
};
