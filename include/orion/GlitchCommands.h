#pragma once
#include "Command.h"
#include "AudioClip.h"
#include "AudioTrack.h"
#include <memory>

namespace Orion {

    class SetClipLengthCommand : public Command {
    public:
        SetClipLengthCommand(std::shared_ptr<AudioClip> clip, uint64_t newLen, uint64_t oldLen)
            : clip(clip), newLen(newLen), oldLen(oldLen) {}

        void execute() override { clip->setLengthFrames(newLen); }
        void undo() override { clip->setLengthFrames(oldLen); }
        std::string getName() const override { return "Set Clip Length"; }

    private:
        std::shared_ptr<AudioClip> clip;
        uint64_t newLen, oldLen;
    };

    class SetClipStartCommand : public Command {
    public:
        SetClipStartCommand(std::shared_ptr<AudioClip> clip, uint64_t newStart, uint64_t oldStart)
            : clip(clip), newStart(newStart), oldStart(oldStart) {}

        void execute() override { clip->setStartFrame(newStart); }
        void undo() override { clip->setStartFrame(oldStart); }
        std::string getName() const override { return "Set Clip Start"; }

    private:
        std::shared_ptr<AudioClip> clip;
        uint64_t newStart, oldStart;
    };

    class SetClipSourceStartCommand : public Command {
    public:
        SetClipSourceStartCommand(std::shared_ptr<AudioClip> clip, uint64_t newStart, uint64_t oldStart)
            : clip(clip), newStart(newStart), oldStart(oldStart) {}

        void execute() override { clip->setSourceStartFrame(newStart); }
        void undo() override { clip->setSourceStartFrame(oldStart); }
        std::string getName() const override { return "Set Clip Source Start"; }

    private:
        std::shared_ptr<AudioClip> clip;
        uint64_t newStart, oldStart;
    };

    class SplitClipCommand : public Command {
    public:
        SplitClipCommand(std::shared_ptr<AudioClip> originalClip, std::shared_ptr<AudioClip> newClip, std::shared_ptr<AudioTrack> track)
            : originalClip(originalClip), newClip(newClip), track(track)
        {
            oldLength = originalClip->getLengthFrames();
            // Assuming newClip starts where originalClip ends after split logic
            // The logic usually is: original length shortened, new clip added.
            // We just record the state change here.
            // The actual logic of "split()" was:
            // 1. Calculate split point
            // 2. originalClip->length = split point - start
            // 3. newClip->start = split point
            // 4. newClip->sourceStart = ...
            // 5. return newClip

            // So if we are GIVEN the new clip which is already created by `clip->split()`,
            // we assume `originalClip` has already been modified in memory?
            // Wait, if we use a Command, `execute` should do the work.
            // But `clip->split()` returns the new clip object.

            // Strategy: The caller calls `split()`. That modifies the object.
            // That is BAD for Undo if we don't capture the old state before modification.
            // BUT `AudioClip::split` modifies `this`.

            // So `applyGlitchBrush` calling `clip->split()` is indeed destructive immediately.

            // To fix this, we should NOT call `clip->split()` directly if we want fully encapsulated Undo.
            // OR we call it, record the old length, and then push a command that sets the length.
            // But if `split` does logic we can't easily replicate...

            // `AudioClip::split` logic:
            // uint64_t offset = splitPoint - currentStart;
            // this->lengthFrames = offset;

            // So the only thing changing on originalClip is lengthFrames.

            // We can capture `oldLength` BEFORE calling split?
            // No, the caller called `split`.

            // Correct approach:
            // Caller does:
            // uint64_t oldLen = clip->getLengthFrames();
            // auto newClip = clip->split(splitPoint);
            // macro->addCommand(new SetClipLengthCommand(clip, clip->getLengthFrames(), oldLen));
            // macro->addCommand(new AddClipCommand(newClip, track));

            // This works! `SetClipLengthCommand` execute() sets to NEW length.
            // Undo() sets to OLD length.
            // But wait, `clip->split()` ALREADY set the new length.
            // So `execute()` is redundant but harmless.
            // `undo()` is vital.

            // However, `AudioClip::split` does not return the old length.
            // So the caller must capture it before.
        }

        // This command is not needed if we compose it from primitives.
        // I'll stick to primitives in `TimelineComponent.cpp`.
        void execute() override {}
        void undo() override {}
        std::string getName() const override { return "Split Clip"; }

    private:
        std::shared_ptr<AudioClip> originalClip;
        std::shared_ptr<AudioClip> newClip;
        std::shared_ptr<AudioTrack> track;
        uint64_t oldLength;
    };
}
