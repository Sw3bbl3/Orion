#pragma once

#include "MidiStructures.h"
#include <vector>
#include <string>
#include <algorithm>
#include <atomic>
#include <memory>

namespace Orion {

    class MidiClip {
    public:
        MidiClip() : startFrame(0), lengthFrames(44100 * 4), name("Midi Clip") {}





        void addNote(const MidiNote& note);
        void sortNotes();

        void removeNote(const MidiNote& note) {
            auto it = std::lower_bound(notes.begin(), notes.end(), note, [](const MidiNote& a, const MidiNote& b) {
                return a.startFrame < b.startFrame;
            });
            while (it != notes.end() && it->startFrame == note.startFrame) {
                if (*it == note) {
                    notes.erase(it);
                    return;
                }
                ++it;
            }
        }

        void removeNote(int index) {
            if (index >= 0 && index < (int)notes.size()) {
                notes.erase(notes.begin() + index);
            }
        }

        void clearNotes() {
            notes.clear();
        }

        void updateNote(int index, const MidiNote& note) {
            if (index >= 0 && index < (int)notes.size()) {
                notes[index] = note;
            }
        }

        const std::vector<MidiNote>& getNotes() const { return notes; }
        std::vector<MidiNote>& getNotesMutable() { return notes; }

        void setStartFrame(uint64_t start) { startFrame.store(start); }
        uint64_t getStartFrame() const { return startFrame.load(); }

        void setLengthFrames(uint64_t len) { lengthFrames.store(len); }
        uint64_t getLengthFrames() const { return lengthFrames.load(); }

        void setOffsetFrames(uint64_t offset) { offsetFrames.store(offset); }
        uint64_t getOffsetFrames() const { return offsetFrames.load(); }


        void setSourceStartFrame(uint64_t offset) { offsetFrames.store(offset); }
        uint64_t getSourceStartFrame() const { return offsetFrames.load(); }

        uint64_t getMaxNoteLength() const { return maxNoteLength.load(); }


        void setDirty(bool d) { dirty.store(d); }
        bool isDirty() const { return dirty.load(); }

        void setName(const std::string& n) { name = n; }
        std::string getName() const { return name; }


        const float* getColor() const { return color; }
        void setColor(float r, float g, float b) {
            color[0] = r; color[1] = g; color[2] = b;
        }


        void quantize(double gridFrames);


        void stretch(double factor);
        void transpose(int semitones);


        std::shared_ptr<MidiClip> split(uint64_t splitPoint);


        std::shared_ptr<MidiClip> clone() const;

    private:
        std::atomic<uint64_t> startFrame;
        std::atomic<uint64_t> lengthFrames;
        std::atomic<uint64_t> offsetFrames { 0 };
        std::atomic<uint64_t> maxNoteLength { 0 };
        std::atomic<bool> dirty { false };
        std::string name;
        float color[3] = { 0.0f, 0.8f, 0.0f };
        std::vector<MidiNote> notes;
    };

}
