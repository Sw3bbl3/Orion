#pragma once

#include <JuceHeader.h>
#include "orion/UxState.h"

class QuickStartRail : public juce::Component
{
public:
    struct Actions {
        std::function<void()> onAddInstrumentTrack;
        std::function<void()> onCreateMidiClip;
        std::function<void()> onOpenPianoRoll;
        std::function<void()> onToggleLoop;
        std::function<void()> onExportPreview;
        std::function<void()> onDismiss;
    };

    QuickStartRail();

    void setState(const Orion::QuickStartState& newState);
    void setActions(Actions newActions);

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    Orion::QuickStartState state;
    Actions actions;

    juce::Label titleLabel;
    juce::Label progressLabel;
    juce::TextButton dismissButton { "Hide" };

    juce::TextButton addTrackButton { "1. Add Instrument Track" };
    juce::TextButton createClipButton { "2. Create MIDI Clip" };
    juce::TextButton addNotesButton { "3. Open Piano Roll" };
    juce::TextButton enableLoopButton { "4. Enable Loop" };
    juce::TextButton exportButton { "5. Export Preview" };

    void refreshStateVisuals();
    void styleTaskButton(juce::TextButton& button, bool done);
};
