#pragma once
#include "EffectNode.h"
#include <vector>
#include <atomic>
#include <cmath>
#include <array>
#include "MidiStructures.h"
#include <juce_audio_basics/juce_audio_basics.h>

namespace Orion {

    class PrismStackNode : public EffectNode {
    public:
        PrismStackNode();
        ~PrismStackNode() override;

        std::string getName() const override;

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t projectTimeFrames) override;
        void processMidi(juce::MidiBuffer& midi, uint64_t frameCount) override;

        bool isInstrument() const override { return false; }

        // Parameters
        void setSpread(float newSpread) { this->spread.store(newSpread); }
        float getSpread() const { return spread.load(); }

        void setDetune(float newDetune) { this->detune.store(newDetune); }
        float getDetune() const { return detune.load(); }

        void setChordType(int type) { chordType.store(type); }
        int getChordType() const { return chordType.load(); }

        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

        enum ChordTypes {
            Octaves = 0,
            Major,
            Minor,
            Fifth,
            Sus4,
            SuperSawStack
        };

    private:
        std::atomic<float> spread { 0.0f }; // Velocity/Time spread
        std::atomic<float> detune { 0.0f }; // Not used for MIDI notes directly unless synth supports MPE/Detune CC, mainly visual/future
        std::atomic<int> chordType { 0 };

        // Helper to generate notes
        void addNote(juce::MidiBuffer& buffer, int note, int velocity, int sampleOffset, int channel);

        // Track note state for correct note-offs
        // Map: Note Number (key) -> Chord Type used when Note On occurred
        // Key: channel * 128 + note. Max index: 16*128 = 2048.
        // Using a fixed size array of atomics avoids locking and allocation.
        // -1 indicates no active note (or default type)
        std::array<std::atomic<int>, 2048> activeNotes;
    };
}
