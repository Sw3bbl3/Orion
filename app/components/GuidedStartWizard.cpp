#include "GuidedStartWizard.h"
#include "../OrionLookAndFeel.h"
#include <cmath>

GuidedStartWizard::GuidedStartWizard()
{
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(true);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Start Making Music", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(22.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(subtitleLabel);
    subtitleLabel.setText("Choose a setup, then Orion creates the first working session for you.", juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(13.0f));
    subtitleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(trackTypeLabel);
    trackTypeLabel.setText("Track Type", juce::dontSendNotification);
    trackTypeLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(trackTypeBox);
    trackTypeBox.addItem("Instrument", 1);
    trackTypeBox.addItem("Audio", 2);
    trackTypeBox.setSelectedId(1, juce::dontSendNotification);

    addAndMakeVisible(bpmLabel);
    bpmLabel.setText("BPM", juce::dontSendNotification);
    bpmLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(bpmSlider);
    bpmSlider.setRange(60.0, 200.0, 1.0);
    bpmSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    bpmSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    bpmSlider.setValue(120.0, juce::dontSendNotification);
    bpmSlider.onValueChange = [this] { syncBpmLabel(); };

    addAndMakeVisible(bpmValueLabel);
    bpmValueLabel.setJustificationType(juce::Justification::centredRight);
    syncBpmLabel();

    addAndMakeVisible(loopBarsLabel);
    loopBarsLabel.setText("Loop Length", juce::dontSendNotification);
    loopBarsLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(loopBarsBox);
    loopBarsBox.addItem("2 Bars", 2);
    loopBarsBox.addItem("4 Bars", 4);
    loopBarsBox.addItem("8 Bars", 8);
    loopBarsBox.addItem("16 Bars", 16);
    loopBarsBox.addItem("32 Bars", 32);
    loopBarsBox.setSelectedId(8, juce::dontSendNotification);

    addAndMakeVisible(createButton);
    createButton.onClick = [this] {
        auto state = getState();
        state.dismissed = true;
        if (onApply) onApply(state);
    };

    addAndMakeVisible(dismissButton);
    dismissButton.onClick = [this] {
        if (onDismiss) onDismiss();
    };
}

void GuidedStartWizard::setState(const Orion::GuidedStartState& state)
{
    trackTypeBox.setSelectedId(state.lastTrackType == Orion::GuidedStartTrackType::Audio ? 2 : 1,
                               juce::dontSendNotification);

    const auto bpm = juce::jlimit(60.0f, 200.0f, state.lastBpm);
    bpmSlider.setValue(bpm, juce::dontSendNotification);
    syncBpmLabel();

    int loopBars = juce::jlimit(2, 32, state.lastLoopBars);
    if (loopBars != 2 && loopBars != 4 && loopBars != 8 && loopBars != 16 && loopBars != 32)
        loopBars = 8;
    loopBarsBox.setSelectedId(loopBars, juce::dontSendNotification);
}

Orion::GuidedStartState GuidedStartWizard::getState() const
{
    Orion::GuidedStartState state;
    state.dismissed = false;
    state.lastTrackType = (trackTypeBox.getSelectedId() == 2)
        ? Orion::GuidedStartTrackType::Audio
        : Orion::GuidedStartTrackType::Instrument;
    state.lastBpm = (float)bpmSlider.getValue();
    state.lastLoopBars = loopBarsBox.getSelectedId() > 0 ? loopBarsBox.getSelectedId() : 8;
    return state;
}

void GuidedStartWizard::syncBpmLabel()
{
    bpmValueLabel.setText(juce::String((int)std::round(bpmSlider.getValue())) + " BPM", juce::dontSendNotification);
}

void GuidedStartWizard::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillRect(bounds);

    auto card = bounds.withSizeKeepingCentre(520.0f, 300.0f);
    auto cardColour = findColour(OrionLookAndFeel::dashboardCardBackgroundColourId);
    g.setColour(cardColour);
    g.fillRoundedRectangle(card, 12.0f);

    g.setColour(findColour(OrionLookAndFeel::dashboardAccentColourId).withAlpha(0.18f));
    g.fillRoundedRectangle(card.removeFromTop(4.0f), 12.0f);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRoundedRectangle(bounds.withSizeKeepingCentre(520.0f, 300.0f), 12.0f, 1.0f);
}

void GuidedStartWizard::resized()
{
    auto card = getLocalBounds().withSizeKeepingCentre(520, 300).reduced(24, 18);

    titleLabel.setBounds(card.removeFromTop(34));
    subtitleLabel.setBounds(card.removeFromTop(22));
    card.removeFromTop(14);

    auto row1 = card.removeFromTop(34);
    trackTypeLabel.setBounds(row1.removeFromLeft(110));
    trackTypeBox.setBounds(row1.removeFromLeft(200));

    card.removeFromTop(10);

    auto row2 = card.removeFromTop(34);
    bpmLabel.setBounds(row2.removeFromLeft(110));
    bpmSlider.setBounds(row2.removeFromLeft(280));
    bpmValueLabel.setBounds(row2);

    card.removeFromTop(10);

    auto row3 = card.removeFromTop(34);
    loopBarsLabel.setBounds(row3.removeFromLeft(110));
    loopBarsBox.setBounds(row3.removeFromLeft(200));

    card.removeFromTop(24);

    auto buttons = card.removeFromBottom(34);
    dismissButton.setBounds(buttons.removeFromRight(120));
    buttons.removeFromRight(10);
    createButton.setBounds(buttons.removeFromRight(150));
}
