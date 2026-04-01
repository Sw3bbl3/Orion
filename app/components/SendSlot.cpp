#include "SendSlot.h"
#include "../OrionLookAndFeel.h"

SendSlot::SendSlot(std::shared_ptr<Orion::GainNode> gn, std::shared_ptr<Orion::AudioTrack> t)
    : gainNode(gn), targetTrack(t)
{
    setWantsKeyboardFocus(true);

    addAndMakeVisible(nameLabel);
    nameLabel.setText(targetTrack->getName(), juce::dontSendNotification);
    nameLabel.setFont(juce::FontOptions("Segoe UI", 10.0f, juce::Font::plain));
    nameLabel.setJustificationType(juce::Justification::centredLeft);
    nameLabel.setColour(juce::Label::textColourId, findColour(juce::Label::textColourId).withAlpha(0.9f));
    nameLabel.setInterceptsMouseClicks(false, false);

    addAndMakeVisible(fader);
    fader.setSliderStyle(juce::Slider::LinearBar);
    fader.setRange(0.0, 1.5, 0.01);
    fader.setValue(gainNode->getGain(), juce::dontSendNotification);
    fader.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fader.setColour(juce::Slider::trackColourId, findColour(juce::Slider::trackColourId).withAlpha(0.65f));
    fader.onValueChange = [this] {
        if (gainNode) gainNode->setGain((float)fader.getValue());
    };


    fader.addMouseListener(this, false);
}

SendSlot::~SendSlot()
{
    fader.removeMouseListener(this);
}

void SendSlot::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    const float corner = 5.0f;

    // Base hover/selection background
    if (selected)
    {
        auto accent = findColour(OrionLookAndFeel::accentColourId);
        g.setColour(accent.withAlpha(0.16f));
        g.fillRoundedRectangle(bounds, corner);

        // Focus outline
        g.setColour(accent.withAlpha(0.55f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.0f);
    }
    else if (isMouseOver(true))
    {
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds, corner);
    }

    // Track tint overlay
    if (! trackTint.isTransparent())
    {
        g.setColour(trackTint.withAlpha(selected ? 0.14f : 0.08f));
        g.fillRoundedRectangle(bounds, corner);
    }
}

void SendSlot::resized()
{
    auto area = getLocalBounds().reduced(2);
    nameLabel.setBounds(area.removeFromLeft(area.getWidth() / 2));
    area.removeFromLeft(2);
    fader.setBounds(area);
}

void SendSlot::mouseDown(const juce::MouseEvent& e)
{
    if (e.eventComponent == this) {
        grabKeyboardFocus();
    }
    if (onSelect) onSelect();
}

void SendSlot::setSelected(bool s)
{
    if (selected != s) {
        selected = s;
        repaint();
    }
}

bool SendSlot::keyPressed(const juce::KeyPress& key)
{
    if (selected && (key == juce::KeyPress::deleteKey || key == juce::KeyPress::backspaceKey)) {
        if (onDelete) onDelete();
        return true;
    }
    return false;
}

void SendSlot::focusGained(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    if (!selected && onSelect) onSelect();
    repaint();
}

void SendSlot::focusLost(juce::Component::FocusChangeType cause)
{
    juce::ignoreUnused(cause);
    repaint();
}
