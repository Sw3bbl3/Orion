#pragma once
#include "Command.h"
#include "AudioEngine.h"
#include "AudioTrack.h"
#include "InstrumentTrack.h"
#include "AudioClip.h"
#include "MidiClip.h"
#include "ClipCommands.h"
#include <memory>
#include <vector>
#include <algorithm>

namespace Orion {





    class SetTrackVolumeCommand : public Command {
    public:
        SetTrackVolumeCommand(std::shared_ptr<AudioTrack> track, float newVol, float oldVol)
            : track(track), newVol(newVol), oldVol(oldVol) {}

        void execute() override { track->setVolume(newVol); }
        void undo() override { track->setVolume(oldVol); }
        std::string getName() const override { return "Set Volume"; }

        bool mergeWith(const Command* other) override {
            const auto* typed = dynamic_cast<const SetTrackVolumeCommand*>(other);
            if (typed && typed->track == track) {
                newVol = typed->newVol;
                return true;
            }
            return false;
        }

    private:
        std::shared_ptr<AudioTrack> track;
        float newVol, oldVol;
    };

    class SetTrackPanCommand : public Command {
    public:
        SetTrackPanCommand(std::shared_ptr<AudioTrack> track, float newPan, float oldPan)
            : track(track), newPan(newPan), oldPan(oldPan) {}

        void execute() override { track->setPan(newPan); }
        void undo() override { track->setPan(oldPan); }
        std::string getName() const override { return "Set Pan"; }

        bool mergeWith(const Command* other) override {
            const auto* typed = dynamic_cast<const SetTrackPanCommand*>(other);
            if (typed && typed->track == track) {
                newPan = typed->newPan;
                return true;
            }
            return false;
        }

    private:
        std::shared_ptr<AudioTrack> track;
        float newPan, oldPan;
    };

    class SetTrackMuteCommand : public Command {
    public:
        SetTrackMuteCommand(std::shared_ptr<AudioTrack> track, bool newState)
            : track(track), newState(newState) {}
        void execute() override { track->setMute(newState); }
        void undo() override { track->setMute(!newState); }
        std::string getName() const override { return newState ? "Mute Track" : "Unmute Track"; }
    private:
        std::shared_ptr<AudioTrack> track;
        bool newState;
    };

    class SetTrackSoloCommand : public Command {
    public:
        SetTrackSoloCommand(AudioEngine& engine, std::shared_ptr<AudioTrack> track, bool newState)
            : engine(engine), track(track), newState(newState) {}
        void execute() override {
            track->setSolo(newState);
            engine.updateSoloState();
        }
        void undo() override {
            track->setSolo(!newState);
            engine.updateSoloState();
        }
        std::string getName() const override { return newState ? "Solo Track" : "Unsolo Track"; }
    private:
        AudioEngine& engine;
        std::shared_ptr<AudioTrack> track;
        bool newState;
    };

    class SetTrackArmCommand : public Command {
    public:
        SetTrackArmCommand(std::shared_ptr<AudioTrack> track, bool newState)
            : track(track), newState(newState) {}
        void execute() override { track->setArmed(newState); }
        void undo() override { track->setArmed(!newState); }
        std::string getName() const override { return newState ? "Arm Track" : "Disarm Track"; }
    private:
        std::shared_ptr<AudioTrack> track;
        bool newState;
    };

    class SetTrackNameCommand : public Command {
    public:
        SetTrackNameCommand(std::shared_ptr<AudioTrack> track, const std::string& newName, const std::string& oldName)
            : track(track), newName(newName), oldName(oldName) {}
        void execute() override { track->setName(newName); }
        void undo() override { track->setName(oldName); }
        std::string getName() const override { return "Rename Track"; }
    private:
        std::shared_ptr<AudioTrack> track;
        std::string newName, oldName;
    };

    class MoveTrackCommand : public Command {
    public:
        MoveTrackCommand(AudioEngine& engine, std::shared_ptr<AudioTrack> track, size_t fromIndex, size_t toIndex)
            : engine(engine), track(track), fromIndex(fromIndex), toIndex(toIndex) {}

        void execute() override { engine.moveTrack(track, toIndex); }
        void undo() override { engine.moveTrack(track, fromIndex); }
        std::string getName() const override { return "Move Track"; }

    private:
        AudioEngine& engine;
        std::shared_ptr<AudioTrack> track;
        size_t fromIndex;
        size_t toIndex;
    };
















    class AddTrackCommand : public Command {
    public:
        AddTrackCommand(std::shared_ptr<AudioTrack> track, std::function<void(std::shared_ptr<AudioTrack>)> addFn, std::function<void(std::shared_ptr<AudioTrack>)> removeFn)
            : track(track), addFn(addFn), removeFn(removeFn) {}
        void execute() override { addFn(track); }
        void undo() override { removeFn(track); }
        std::string getName() const override { return "Add Track"; }
    private:
        std::shared_ptr<AudioTrack> track;
        std::function<void(std::shared_ptr<AudioTrack>)> addFn;
        std::function<void(std::shared_ptr<AudioTrack>)> removeFn;
    };

    class DeleteTrackCommand : public Command {
    public:
        DeleteTrackCommand(std::shared_ptr<AudioTrack> track, std::function<void(std::shared_ptr<AudioTrack>)> removeFn, std::function<void(std::shared_ptr<AudioTrack>)> addFn)
            : track(track), removeFn(removeFn), addFn(addFn) {}
        void execute() override { removeFn(track); }
        void undo() override { addFn(track); }
        std::string getName() const override { return "Delete Track"; }
    private:
        std::shared_ptr<AudioTrack> track;
        std::function<void(std::shared_ptr<AudioTrack>)> removeFn;
        std::function<void(std::shared_ptr<AudioTrack>)> addFn;
    };






    class AddMidiNoteCommand : public Command {
    public:
        AddMidiNoteCommand(std::shared_ptr<MidiClip> clip, const MidiNote& note)
            : clip(clip), note(note) {}

        void execute() override {
            clip->addNote(note);
        }

        void undo() override {
            clip->removeNote(note);
        }

        std::string getName() const override { return "Add Note"; }

    private:
        std::shared_ptr<MidiClip> clip;
        MidiNote note;
    };

    class RemoveMidiNoteCommand : public Command {
    public:
        RemoveMidiNoteCommand(std::shared_ptr<MidiClip> clip, const MidiNote& note)
            : clip(clip), note(note) {}

        void execute() override {
            clip->removeNote(note);
        }

        void undo() override {
            clip->addNote(note);
        }

        std::string getName() const override { return "Delete Note"; }

    private:
        std::shared_ptr<MidiClip> clip;
        MidiNote note;
    };

    class MoveMidiNoteCommand : public Command {
    public:
        MoveMidiNoteCommand(std::shared_ptr<MidiClip> clip, const MidiNote& oldNote, const MidiNote& newNote)
            : clip(clip), oldNote(oldNote), newNote(newNote) {}

        void execute() override {
            clip->removeNote(oldNote);
            clip->addNote(newNote);
        }

        void undo() override {
            clip->removeNote(newNote);
            clip->addNote(oldNote);
        }

        std::string getName() const override { return "Move Note"; }

        bool mergeWith(const Command* other) override {
            const auto* typed = dynamic_cast<const MoveMidiNoteCommand*>(other);
            if (typed && typed->clip == clip &&
                typed->oldNote.noteNumber == newNote.noteNumber &&
                typed->oldNote.startFrame == newNote.startFrame)
            {
                newNote = typed->newNote;
                return true;
            }
            return false;
        }

    private:
        std::shared_ptr<MidiClip> clip;
        MidiNote oldNote, newNote;
    };

    class ResizeMidiNoteCommand : public Command {
    public:
        ResizeMidiNoteCommand(std::shared_ptr<MidiClip> clip, const MidiNote& oldNote, const MidiNote& newNote)
            : clip(clip), oldNote(oldNote), newNote(newNote) {}

        void execute() override {
            clip->removeNote(oldNote);
            clip->addNote(newNote);
        }

        void undo() override {
            clip->removeNote(newNote);
            clip->addNote(oldNote);
        }

        std::string getName() const override { return "Resize Note"; }

        bool mergeWith(const Command* other) override {
            const auto* typed = dynamic_cast<const ResizeMidiNoteCommand*>(other);
            if (typed && typed->clip == clip &&
                typed->oldNote.lengthFrames == newNote.lengthFrames &&
                typed->oldNote.startFrame == newNote.startFrame &&
                typed->oldNote.noteNumber == newNote.noteNumber)
            {
                newNote = typed->newNote;
                return true;
            }
            return false;
        }

    private:
        std::shared_ptr<MidiClip> clip;
        MidiNote oldNote, newNote;
    };

}
