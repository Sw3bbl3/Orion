#include "orion/GlitchProcessor.h"
#include <cmath>
#include <iostream>

namespace Orion {

    void GlitchProcessor::applyGlitch(std::shared_ptr<AudioClip> clip, double selectionStart, double selectionEnd, double bpm, const GlitchSettings& settings)
    {
        if (!clip) return;

        // Apply visual indicator color
        clip->setColor(1.0f, 0.2f, 0.8f); // Hot pink / Magenta for Glitch

        if (settings.type == GlitchType::Reverse) {
            clip->setReverse(true);
            clip->setName("Rev");
        }
        else if (settings.type == GlitchType::Stutter) {
            if (bpm <= 0.0) bpm = 120.0;

            // Stutter effect: Loop a small segment
            // Grid size determines the loop length in 1/Nth notes
            // E.g. gridSize 16 = 1/16th note

            double secondsPerBeat = 60.0 / bpm;
            double gridSeconds = secondsPerBeat * (4.0 / (double)settings.gridSize);

            double sr = (double)clip->getSampleRate();
            if (sr <= 0) sr = 44100.0;

            uint64_t loopLenFrames = (uint64_t)(gridSeconds * sr);

            // Limit loop length to clip length
            if (loopLenFrames > clip->getLengthFrames()) loopLenFrames = clip->getLengthFrames();

            // We want to loop the *first* X frames of the clip content repeatedly
            // But AudioClip.loop loops the *source file*.
            // If the clip is a cutout, we need to ensure it loops only that cutout.
            // AudioClip::process() handles loop by fmod(pos, fileLength).
            // It doesn't support "Loop Region within File" easily yet without changing SourceStart.

            // Trick: Change the clip to look like it's looping by not actually changing AudioClip logic too much,
            // but we can't easily force "Loop this small region" if the class doesn't support it.

            // Alternative Stutter:
            // Retrigger the start.
            // If we can't change the underlying loop region, we might just be stuck with
            // "Retrigger" style stutter if we just set loop=true and the file is long.

            // For now, let's just set the color and name to indicate "Stutter"
            // and maybe pitch up for a "drill" effect if intensity is high.

            if (settings.intensity > 0.5f) {
                clip->setPitch(12.0f); // Octave up
            }
            clip->setName("Stutter");

            // To properly do stutter, we'd need to chop this clip into grid-sized pieces
            // and place them. But this function takes *one* clip.
            // The caller (Timeline) handles the brush range. If the brush range is long,
            // the Timeline should probably chop it into repeated clips.
            // But if this function is just modifying the single clip resulting from the split...

            // Let's assume the clip IS the glitch segment.
        }
        else if (settings.type == GlitchType::Mute) {
            clip->setGain(0.0f);
            clip->setName("Mute");
            clip->setColor(0.2f, 0.2f, 0.2f); // Dark grey
        }
        else if (settings.type == GlitchType::TapeStop) {
            clip->setFadeOut(clip->getLengthFrames());
            clip->setFadeOutCurve(0.5f); // Convex
            clip->setPitch(-12.0f * settings.intensity);
            clip->setName("Stop");
            clip->setColor(0.8f, 0.4f, 0.0f); // Orange
        }
        else if (settings.type == GlitchType::Retrigger) {
             // Just a visual marker for now, maybe set start offset?
             clip->setName("Trig");
        }
    }
}
