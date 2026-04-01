#include "orion/PrismStackNode.h"
#include <random>
#include <algorithm>

namespace Orion {

    PrismStackNode::PrismStackNode() {
        // Initialize active notes to -1
        for(auto& n : activeNotes) n.store(-1);
    }

    std::string PrismStackNode::getName() const {
        return "Prism Stack";
    }

    PrismStackNode::~PrismStackNode() {}

    void PrismStackNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t projectTimeFrames) {
        (void)block;
        (void)cacheKey;
        (void)projectTimeFrames;
        // Pass-through audio, this is a MIDI processor mainly
    }

    void PrismStackNode::processMidi(juce::MidiBuffer& midi, uint64_t) {
        if (isBypassed()) return;

        juce::MidiBuffer processed;

        for (const auto metadata : midi) {
            auto msg = metadata.getMessage();
            int samplePos = metadata.samplePosition;

            if (msg.isNoteOn()) {
                int root = msg.getNoteNumber();
                int vel = msg.getVelocity();
                int ch = msg.getChannel();

                int type = chordType.load();

                // Store state for note-off without locking
                int key = (ch - 1) * 128 + root; // MIDI channels are 1-16
                if (key >= 0 && key < (int)activeNotes.size()) {
                    activeNotes[key].store(type);
                }

                // Add Root
                processed.addEvent(msg, samplePos);

                std::vector<int> offsets;
                if (type == Octaves) offsets = { -12, 12 };
                else if (type == Major) offsets = { 4, 7, 12 };
                else if (type == Minor) offsets = { 3, 7, 12 };
                else if (type == Fifth) offsets = { 7 };
                else if (type == Sus4) offsets = { 5, 7 };
                else if (type == SuperSawStack) offsets = { -12, -24, 12, 24, 7, 19 };

                float currentSpread = spread.load();

                for (int offset : offsets) {
                    int note = root + offset;
                    if (note >= 0 && note <= 127) {
                        // Deterministic RNG
                        int seed = (note * 13 + offset * 7) & 0xFFFF;
                        float randomVal = (float)(seed % 100) / 100.0f;

                        int jitter = 0;
                        if (currentSpread > 0.0f) {
                            jitter = (int)(randomVal * currentSpread * 100.0f);
                        }

                        int newVel = vel;
                        if (currentSpread > 0.0f) {
                             newVel = vel + (int)(randomVal * currentSpread * 40.0f - currentSpread * 20.0f);
                             newVel = std::max(1, std::min(127, newVel));
                        }

                        processed.addEvent(juce::MidiMessage::noteOn(ch, note, (juce::uint8)newVel), samplePos + jitter);
                    }
                }

            } else if (msg.isNoteOff()) {
                int root = msg.getNoteNumber();
                int ch = msg.getChannel();

                processed.addEvent(msg, samplePos);

                // Retrieve type used at On
                int key = (ch - 1) * 128 + root;
                int type = chordType.load(); // Default

                if (key >= 0 && key < (int)activeNotes.size()) {
                    int stored = activeNotes[key].exchange(-1); // Reset to -1
                    if (stored != -1) type = stored;
                }

                std::vector<int> offsets;
                if (type == Octaves) offsets = { -12, 12 };
                else if (type == Major) offsets = { 4, 7, 12 };
                else if (type == Minor) offsets = { 3, 7, 12 };
                else if (type == Fifth) offsets = { 7 };
                else if (type == Sus4) offsets = { 5, 7 };
                else if (type == SuperSawStack) offsets = { -12, -24, 12, 24, 7, 19 };

                for (int offset : offsets) {
                    int note = root + offset;
                    if (note >= 0 && note <= 127) {
                        processed.addEvent(juce::MidiMessage::noteOff(ch, note), samplePos);
                    }
                }
            } else {
                processed.addEvent(msg, samplePos);
            }
        }

        midi = processed;
    }

    bool PrismStackNode::getChunk(std::vector<char>& data) {
        float s = spread.load();
        float d = detune.load();
        int t = chordType.load();

        data.resize(sizeof(float) * 2 + sizeof(int));
        char* ptr = data.data();
        std::memcpy(ptr, &s, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &d, sizeof(float)); ptr += sizeof(float);
        std::memcpy(ptr, &t, sizeof(int));
        return true;
    }

    bool PrismStackNode::setChunk(const std::vector<char>& data) {
        if (data.size() < sizeof(float) * 2 + sizeof(int)) return false;
        const char* ptr = data.data();
        float s, d;
        int t;
        std::memcpy(&s, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&d, ptr, sizeof(float)); ptr += sizeof(float);
        std::memcpy(&t, ptr, sizeof(int));

        spread.store(s);
        detune.store(d);
        chordType.store(t);
        return true;
    }

    void PrismStackNode::addNote(juce::MidiBuffer& buffer, int note, int velocity, int sampleOffset, int channel) {
        // Not used in this iteration, logic is embedded in loop for simplicity and context access
        (void)buffer; (void)note; (void)velocity; (void)sampleOffset; (void)channel;
    }

}
