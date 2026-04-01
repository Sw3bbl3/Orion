#include "orion/MidiClip.h"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace Orion {

    void MidiClip::addNote(const MidiNote& note) {
        auto it = std::upper_bound(notes.begin(), notes.end(), note, [](const MidiNote& a, const MidiNote& b) {
            return a.startFrame < b.startFrame;
        });
        notes.insert(it, note);

        uint64_t currentMax = maxNoteLength.load();
        if (note.lengthFrames > currentMax) maxNoteLength.store(note.lengthFrames);
    }

    void MidiClip::sortNotes() {
        std::sort(notes.begin(), notes.end(), [](const MidiNote& a, const MidiNote& b) {
            return a.startFrame < b.startFrame;
        });

        uint64_t max = 0;
        for (const auto& n : notes) if (n.lengthFrames > max) max = n.lengthFrames;
        maxNoteLength.store(max);

        dirty.store(false);
    }

    void MidiClip::quantize(double gridFrames) {
        if (gridFrames <= 0.001) return;

        for (auto& note : notes) {

            double start = (double)note.startFrame;
            double roundedStart = std::round(start / gridFrames) * gridFrames;
            note.startFrame = (uint64_t)roundedStart;


            double len = (double)note.lengthFrames;
            double roundedLen = std::round(len / gridFrames) * gridFrames;
            if (roundedLen < gridFrames) roundedLen = gridFrames;
            note.lengthFrames = (uint64_t)roundedLen;
        }
        sortNotes();

        uint64_t max = 0;
        for (const auto& n : notes) if (n.lengthFrames > max) max = n.lengthFrames;
        maxNoteLength.store(max);
    }

    std::shared_ptr<MidiClip> MidiClip::split(uint64_t splitPoint) {

        uint64_t currentStart = startFrame.load();
        uint64_t currentLength = lengthFrames.load();

        if (splitPoint <= currentStart || splitPoint >= currentStart + currentLength) {
            return nullptr;
        }

        uint64_t splitOffset = splitPoint - currentStart;

        auto newClip = std::make_shared<MidiClip>();
        newClip->setStartFrame(splitPoint);
        newClip->setLengthFrames(currentLength - splitOffset);
        newClip->setName(name);


        std::vector<MidiNote> keptNotes;

        for (const auto& note : notes) {
            uint64_t noteEnd = note.startFrame + note.lengthFrames;

            if (noteEnd <= splitOffset) {

                keptNotes.push_back(note);
            }
            else if (note.startFrame >= splitOffset) {

                MidiNote newNote = note;
                newNote.startFrame = note.startFrame - splitOffset;
                newClip->addNote(newNote);
            }
            else {


                MidiNote leftNote = note;
                leftNote.lengthFrames = splitOffset - note.startFrame;
                keptNotes.push_back(leftNote);


                MidiNote rightNote = note;
                rightNote.startFrame = 0;
                rightNote.lengthFrames = noteEnd - splitOffset;
                newClip->addNote(rightNote);
            }
        }

        this->notes = keptNotes;
        this->lengthFrames.store(splitOffset);

        uint64_t max = 0;
        for (const auto& n : notes) if (n.lengthFrames > max) max = n.lengthFrames;
        maxNoteLength.store(max);

        return newClip;
    }

    void MidiClip::stretch(double factor) {
        if (factor <= 0.0001) return;

        for (auto& note : notes) {
            double start = (double)note.startFrame * factor;
            double len = (double)note.lengthFrames * factor;

            note.startFrame = (uint64_t)start;
            note.lengthFrames = (uint64_t)len;
            if (note.lengthFrames < 1) note.lengthFrames = 1;
        }
        sortNotes();

        uint64_t max = 0;
        for (const auto& n : notes) if (n.lengthFrames > max) max = n.lengthFrames;
        maxNoteLength.store(max);

        double newLen = (double)lengthFrames.load() * factor;
        uint64_t finalLen = (uint64_t)newLen;
        if (finalLen < 100) finalLen = 100;

        lengthFrames.store(finalLen);
    }

    void MidiClip::transpose(int semitones) {
        if (semitones == 0) return;
        for (auto& note : notes) {
            int newNote = note.noteNumber + semitones;
            if (newNote < 0) newNote = 0;
            if (newNote > 127) newNote = 127;
            note.noteNumber = newNote;
        }
        dirty.store(true);
    }

    std::shared_ptr<MidiClip> MidiClip::clone() const {
        auto newClip = std::make_shared<MidiClip>();
        newClip->setStartFrame(startFrame.load());
        newClip->setLengthFrames(lengthFrames.load());
        newClip->setOffsetFrames(offsetFrames.load());
        newClip->setName(name);

        auto& targetNotes = newClip->getNotesMutable();
        targetNotes = notes;


        newClip->sortNotes();

        return newClip;
    }

}
