#include "QuickStartRail.h"

QuickStartRail::QuickStartRail()
{
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Quick Start", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    titleLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(progressLabel);
    progressLabel.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(dismissButton);
    dismissButton.onClick = [this] { if (actions.onDismiss) actions.onDismiss(); };

    addAndMakeVisible(addTrackButton);
    addAndMakeVisible(createClipButton);
    addAndMakeVisible(addNotesButton);
    addAndMakeVisible(enableLoopButton);
    addAndMakeVisible(exportButton);

    addTrackButton.onClick = [this] { if (actions.onAddInstrumentTrack) actions.onAddInstrumentTrack(); };
    createClipButton.onClick = [this] { if (actions.onCreateMidiClip) actions.onCreateMidiClip(); };
    addNotesButton.onClick = [this] { if (actions.onOpenPianoRoll) actions.onOpenPianoRoll(); };
    enableLoopButton.onClick = [this] { if (actions.onToggleLoop) actions.onToggleLoop(); };
    exportButton.onClick = [this] { if (actions.onExportPreview) actions.onExportPreview(); };

    refreshStateVisuals();
}

void QuickStartRail::setState(const Orion::QuickStartState& newState)
{
    state = newState;
    refreshStateVisuals();
}

void QuickStartRail::setActions(Actions newActions)
{
    actions = std::move(newActions);
}

void QuickStartRail::styleTaskButton(juce::TextButton& button, bool done)
{
    button.setColour(juce::TextButton::buttonColourId, done ? juce::Colour(0xFF2E7D32) : juce::Colour(0xFF2A2A2A));
    button.setColour(juce::TextButton::textColourOffId, juce::Colours::white);
}

void QuickStartRail::refreshStateVisuals()
{
    const int done = state.completedCount();
    progressLabel.setText(juce::String(done) + "/5 complete", juce::dontSendNotification);

    styleTaskButton(addTrackButton, state.firstTrackAdded);
    styleTaskButton(createClipButton, state.firstClipCreated);
    styleTaskButton(addNotesButton, state.firstNotesAdded);
    styleTaskButton(enableLoopButton, state.loopEnabled);
    styleTaskButton(exportButton, state.firstExport);

    addTrackButton.setEnabled(!state.firstTrackAdded);
    createClipButton.setEnabled(!state.firstClipCreated);
    addNotesButton.setEnabled(!state.firstNotesAdded);
    enableLoopButton.setEnabled(!state.loopEnabled);
    exportButton.setEnabled(!state.firstExport);

    repaint();
}

void QuickStartRail::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    g.setColour(juce::Colours::black.withAlpha(0.72f));
    g.fillRoundedRectangle(bounds, 8.0f);

    g.setColour(juce::Colours::white.withAlpha(0.15f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 8.0f, 1.0f);
}

void QuickStartRail::resized()
{
    auto area = getLocalBounds().reduced(10);
    auto header = area.removeFromTop(24);

    dismissButton.setBounds(header.removeFromRight(52));
    titleLabel.setBounds(header.removeFromLeft(120));
    progressLabel.setBounds(header);

    area.removeFromTop(8);

    const int rowH = 28;
    addTrackButton.setBounds(area.removeFromTop(rowH));
    area.removeFromTop(6);
    createClipButton.setBounds(area.removeFromTop(rowH));
    area.removeFromTop(6);
    addNotesButton.setBounds(area.removeFromTop(rowH));
    area.removeFromTop(6);
    enableLoopButton.setBounds(area.removeFromTop(rowH));
    area.removeFromTop(6);
    exportButton.setBounds(area.removeFromTop(rowH));
}
