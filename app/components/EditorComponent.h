#pragma once
#include <JuceHeader.h>
#include "orion/AudioEngine.h"
#include "orion/CommandHistory.h"
#include "PianoRollComponent.h"
#include "orion/MidiClip.h"
#include "orion/InstrumentTrack.h"

class EditorComponent : public juce::Component
{
public:
    EditorComponent(Orion::AudioEngine& engine, Orion::CommandHistory& history);
    ~EditorComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void showPianoRoll(std::shared_ptr<Orion::MidiClip> clip, std::shared_ptr<Orion::InstrumentTrack> track = nullptr);

    std::shared_ptr<Orion::MidiClip> getCurrentClip() const { return pianoRoll.getCurrentClip(); }
    std::shared_ptr<Orion::InstrumentTrack> getCurrentTrack() const { return pianoRoll.getCurrentTrack(); }

    const PianoRollComponent& getPianoRoll() const { return pianoRoll; }

private:
    Orion::AudioEngine& engine;

    PianoRollComponent pianoRoll;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorComponent)
};
