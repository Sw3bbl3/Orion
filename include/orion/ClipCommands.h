#pragma once
#include "Command.h"
#include "AudioTrack.h"
#include "AudioClip.h"
#include "MidiClip.h"
#include "InstrumentTrack.h"
#include <memory>
#include <vector>

namespace Orion {




    class MoveClipCommand : public Command {
    public:
        MoveClipCommand(std::shared_ptr<AudioClip> clip,
                        uint64_t oldStart, uint64_t newStart,
                        std::shared_ptr<AudioTrack> oldTrack,
                        std::shared_ptr<AudioTrack> newTrack)
            : clip(clip),
              oldStart(oldStart), newStart(newStart),
              oldTrack(oldTrack), newTrack(newTrack)
        {}

        void execute() override {
            if (oldTrack != newTrack) {
                oldTrack->removeClip(clip);
                newTrack->addClip(clip);
            }
            clip->setStartFrame(newStart);
        }

        void undo() override {
            if (oldTrack != newTrack) {
                newTrack->removeClip(clip);
                oldTrack->addClip(clip);
            }
            clip->setStartFrame(oldStart);
        }

        std::string getName() const override { return "Move Clip"; }

    private:
        std::shared_ptr<AudioClip> clip;
        uint64_t oldStart, newStart;
        std::shared_ptr<AudioTrack> oldTrack;
        std::shared_ptr<AudioTrack> newTrack;
    };




    class MoveMidiClipCommand : public Command {
    public:
        MoveMidiClipCommand(std::shared_ptr<MidiClip> clip,
                            uint64_t oldStart, uint64_t newStart,
                            std::shared_ptr<InstrumentTrack> oldTrack,
                            std::shared_ptr<InstrumentTrack> newTrack)
            : clip(clip),
              oldStart(oldStart), newStart(newStart),
              oldTrack(oldTrack), newTrack(newTrack)
        {}

        void execute() override {
            if (oldTrack != newTrack) {
                oldTrack->removeMidiClip(clip);
                newTrack->addMidiClip(clip);
            }
            clip->setStartFrame(newStart);
        }

        void undo() override {
            if (oldTrack != newTrack) {
                newTrack->removeMidiClip(clip);
                oldTrack->addMidiClip(clip);
            }
            clip->setStartFrame(oldStart);
        }

        std::string getName() const override { return "Move MIDI Clip"; }

    private:
        std::shared_ptr<MidiClip> clip;
        uint64_t oldStart, newStart;
        std::shared_ptr<InstrumentTrack> oldTrack;
        std::shared_ptr<InstrumentTrack> newTrack;
    };




    class ResizeClipCommand : public Command {
    public:
        ResizeClipCommand(std::shared_ptr<AudioClip> clip,
                          uint64_t oldStart, uint64_t newStart,
                          uint64_t oldLength, uint64_t newLength,
                          uint64_t oldSource, uint64_t newSource)
            : clip(clip),
              oldStart(oldStart), newStart(newStart),
              oldLength(oldLength), newLength(newLength),
              oldSource(oldSource), newSource(newSource)
        {}

        void execute() override {
            clip->setStartFrame(newStart);
            clip->setLengthFrames(newLength);
            clip->setSourceStartFrame(newSource);
        }

        void undo() override {
            clip->setStartFrame(oldStart);
            clip->setLengthFrames(oldLength);
            clip->setSourceStartFrame(oldSource);
        }

        std::string getName() const override { return "Resize Clip"; }

    private:
        std::shared_ptr<AudioClip> clip;
        uint64_t oldStart, newStart;
        uint64_t oldLength, newLength;
        uint64_t oldSource, newSource;
    };




    class ResizeMidiClipCommand : public Command {
    public:
        ResizeMidiClipCommand(std::shared_ptr<MidiClip> clip,
                              uint64_t oldStart, uint64_t newStart,
                              uint64_t oldLength, uint64_t newLength,
                              uint64_t oldOffset, uint64_t newOffset)
            : clip(clip),
              oldStart(oldStart), newStart(newStart),
              oldLength(oldLength), newLength(newLength),
              oldOffset(oldOffset), newOffset(newOffset)
        {}

        void execute() override {
            clip->setStartFrame(newStart);
            clip->setLengthFrames(newLength);
            clip->setOffsetFrames(newOffset);
        }

        void undo() override {
            clip->setStartFrame(oldStart);
            clip->setLengthFrames(oldLength);
            clip->setOffsetFrames(oldOffset);
        }

        std::string getName() const override { return "Resize MIDI Clip"; }

    private:
        std::shared_ptr<MidiClip> clip;
        uint64_t oldStart, newStart;
        uint64_t oldLength, newLength;
        uint64_t oldOffset, newOffset;
    };




    class AddClipCommand : public Command {
    public:
        AddClipCommand(std::shared_ptr<AudioClip> clip, std::shared_ptr<AudioTrack> track)
            : clip(clip), track(track) {}
        void execute() override { track->addClip(clip); }
        void undo() override { track->removeClip(clip); }
        std::string getName() const override { return "Add Clip"; }
    private:
        std::shared_ptr<AudioClip> clip;
        std::shared_ptr<AudioTrack> track;
    };

    class RemoveClipCommand : public Command {
    public:
        RemoveClipCommand(std::shared_ptr<AudioClip> clip, std::shared_ptr<AudioTrack> track)
            : clip(clip), track(track) {}
        void execute() override { track->removeClip(clip); }
        void undo() override { track->addClip(clip); }
        std::string getName() const override { return "Remove Clip"; }
    private:
        std::shared_ptr<AudioClip> clip;
        std::shared_ptr<AudioTrack> track;
    };




    class AddMidiClipCommand : public Command {
    public:
        AddMidiClipCommand(std::shared_ptr<MidiClip> clip, std::shared_ptr<InstrumentTrack> track)
            : clip(clip), track(track) {}
        void execute() override { track->addMidiClip(clip); }
        void undo() override { track->removeMidiClip(clip); }
        std::string getName() const override { return "Add MIDI Clip"; }
    private:
        std::shared_ptr<MidiClip> clip;
        std::shared_ptr<InstrumentTrack> track;
    };

    class RemoveMidiClipCommand : public Command {
    public:
        RemoveMidiClipCommand(std::shared_ptr<MidiClip> clip, std::shared_ptr<InstrumentTrack> track)
            : clip(clip), track(track) {}
        void execute() override { track->removeMidiClip(clip); }
        void undo() override { track->addMidiClip(clip); }
        std::string getName() const override { return "Remove MIDI Clip"; }
    private:
        std::shared_ptr<MidiClip> clip;
        std::shared_ptr<InstrumentTrack> track;
    };

}
