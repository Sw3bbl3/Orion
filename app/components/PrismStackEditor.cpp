#include "PrismStackEditor.h"
#include "../OrionLookAndFeel.h"

PrismStackEditor::PrismStackEditor(std::shared_ptr<Orion::PrismStackNode> n)
    : node(n)
{
    addAndMakeVisible(spreadSlider);
    spreadSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    spreadSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    spreadSlider.setRange(0.0, 1.0);
    spreadSlider.setValue(node->getSpread());
    spreadSlider.onValueChange = [this] {
        node->setSpread((float)spreadSlider.getValue());
    };

    addAndMakeVisible(spreadLabel);
    spreadLabel.setText("Spread", juce::dontSendNotification);
    spreadLabel.setJustificationType(juce::Justification::centred);
    spreadLabel.setFont(juce::FontOptions(12.0f));

    addAndMakeVisible(chordSelector);
    chordSelector.addItem("Octaves", 1);
    chordSelector.addItem("Major", 2);
    chordSelector.addItem("Minor", 3);
    chordSelector.addItem("Fifth", 4);
    chordSelector.addItem("Sus4", 5);
    chordSelector.addItem("SuperSaw Stack", 6);

    chordSelector.setSelectedId(node->getChordType() + 1, juce::dontSendNotification);
    chordSelector.onChange = [this] {
        node->setChordType(chordSelector.getSelectedId() - 1);
    };

    addAndMakeVisible(chordLabel);
    chordLabel.setText("Type", juce::dontSendNotification);
    chordLabel.setJustificationType(juce::Justification::centred);
    chordLabel.setFont(juce::FontOptions(12.0f));

    setSize(250, 150);
}

PrismStackEditor::~PrismStackEditor() {}

void PrismStackEditor::paint(juce::Graphics& g)
{
    g.fillAll(findColour(OrionLookAndFeel::mixerChassisColourId).darker(0.2f));

    g.setColour(findColour(OrionLookAndFeel::accentColourId).withAlpha(0.1f));
    g.fillRoundedRectangle(getLocalBounds().toFloat().reduced(4), 8.0f);

    g.setColour(findColour(OrionLookAndFeel::accentColourId));
    g.drawRoundedRectangle(getLocalBounds().toFloat().reduced(4), 8.0f, 1.0f);

    g.setFont(juce::FontOptions(16.0f));
    g.drawText("PRISM STACK", 0, 10, getWidth(), 20, juce::Justification::centred);
}

void PrismStackEditor::resized()
{
    auto area = getLocalBounds().reduced(20);
    area.removeFromTop(20);

    auto left = area.removeFromLeft(getWidth()/2 - 20);
    auto right = area.removeFromRight(getWidth()/2 - 20);

    chordLabel.setBounds(left.removeFromTop(20));
    chordSelector.setBounds(left.removeFromTop(30));

    spreadLabel.setBounds(right.removeFromTop(20));
    spreadSlider.setBounds(right.withSizeKeepingCentre(50, 50));
}
