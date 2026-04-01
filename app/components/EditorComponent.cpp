#include "EditorComponent.h"

EditorComponent::EditorComponent(Orion::AudioEngine& e, Orion::CommandHistory& history)
    : engine(e), pianoRoll(e, history)
{
    addAndMakeVisible(pianoRoll);
}

EditorComponent::~EditorComponent()
{
}

void EditorComponent::showPianoRoll(std::shared_ptr<Orion::MidiClip> clip, std::shared_ptr<Orion::InstrumentTrack> track)
{
    pianoRoll.setClip(clip, track);
    pianoRoll.grabKeyboardFocus();
    repaint();
}

void EditorComponent::paint(juce::Graphics& g)
{
    g.fillAll(findColour(juce::ResizableWindow::backgroundColourId).darker(0.2f));
}

void EditorComponent::resized()
{
    pianoRoll.setBounds(getLocalBounds());
}
