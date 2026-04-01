#include "orion/InstrumentTrack.h"
#include "orion/HostContext.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Orion {

    InstrumentTrack::InstrumentTrack() {
        midiClips = std::make_shared<std::vector<std::shared_ptr<MidiClip>>>();

        auto synth = std::make_shared<SynthesizerNode>();
        synth->setSampleRate(44100.0);
        instrument = synth;
    }

    SynthesizerNode* InstrumentTrack::getSynth() {
        auto inst = std::atomic_load(&instrument);
        return dynamic_cast<SynthesizerNode*>(inst.get());
    }

    void InstrumentTrack::setInstrument(std::shared_ptr<AudioNode> inst) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        if (inst) {
            inst->setSampleRate(getSampleRate());
            inst->setFormat(channels.load(), blockSize.load());
            std::atomic_store(&instrument, inst);
        }
    }

    void InstrumentTrack::setFormat(size_t numChannels, size_t numSamples) {
        AudioTrack::setFormat(numChannels, numSamples);

        auto localInst = std::atomic_load(&instrument);
        if (localInst) {
            localInst->setFormat(numChannels, numSamples);
        }
    }

    int InstrumentTrack::getLatency() const {
        int total = AudioTrack::getLatency();
        auto localInst = std::atomic_load(&instrument);
        if (localInst) {
            total += localInst->getLatency();
        }
        return total;
    }

    void InstrumentTrack::setColor(float r, float g, float b) {
        AudioTrack::setColor(r, g, b);

        auto currentMidiClips = std::atomic_load(&midiClips);
        if (currentMidiClips) {
             for (const auto& clip : *currentMidiClips) {
                 clip->setColor(r, g, b);
             }
        }
    }

    void InstrumentTrack::allNotesOff() {
        auto localInst = std::atomic_load(&instrument);
        if (!localInst) return;


        for (int i = 0; i < 128; ++i) {
            if (activeNotes[i]) {
                localInst->processMidi(0x80, i, 0, 0);
                activeNotes[i] = false;
            }
        }
        activeNotes.reset();


        localInst->processMidi(0xB0, 123, 0, 0);

        localInst->processMidi(0xB0, 120, 0, 0);
    }

    void InstrumentTrack::startPreviewNote(int note, float velocity) {
        previewQueue.push({note, velocity});
    }

    void InstrumentTrack::stopPreviewNote(int note) {
        previewQueue.push({note, 0.0f});
    }

    void InstrumentTrack::injectLiveMidiMessage(const juce::MidiMessage& message) {
        RawMidiMessage raw;
        const auto* d = message.getRawData();
        int s = message.getRawDataSize();
        if (s > 128) s = 128;
        std::memcpy(raw.data, d, s);
        raw.size = s;
        liveMidiQueue.push(raw);
    }

    void InstrumentTrack::addMidiClip(std::shared_ptr<MidiClip> clip) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        auto current = std::atomic_load(&midiClips);
        auto next = std::make_shared<std::vector<std::shared_ptr<MidiClip>>>(*current);
        next->push_back(clip);
        std::atomic_store(&midiClips, std::shared_ptr<const std::vector<std::shared_ptr<MidiClip>>>(next));
    }

    void InstrumentTrack::removeMidiClip(std::shared_ptr<MidiClip> clip) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        auto current = std::atomic_load(&midiClips);
        auto next = std::make_shared<std::vector<std::shared_ptr<MidiClip>>>(*current);

        auto it = std::remove(next->begin(), next->end(), clip);
        if (it != next->end()) {
            next->erase(it, next->end());
            std::atomic_store(&midiClips, std::shared_ptr<const std::vector<std::shared_ptr<MidiClip>>>(next));
        }
    }

    void InstrumentTrack::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {

        bool isMuted = muteAtomic.load() || soloMuted.load();

        auto localInst = std::atomic_load(&instrument);
        if (!localInst) return;


        if (localInst->getSampleRate() != getSampleRate()) {
            localInst->setSampleRate(getSampleRate());
        }

        size_t numSamples = block.getSampleCount();
        size_t outChannels = block.getChannelCount();
        size_t internalChannels = std::max((size_t)2, outChannels);



        if (localBlock.getSampleCount() != numSamples || localBlock.getChannelCount() != internalChannels) {
            localBlock.resize(internalChannels, numSamples);
        }
        localBlock.zero();
        for (size_t c = 0; c < block.getChannelCount() && c < internalChannels; ++c) {
            std::copy(block.getChannelPointer(c), block.getChannelPointer(c) + numSamples, localBlock.getChannelPointer(c));
        }


        auto localMidiClips = std::atomic_load(&midiClips);
        auto localEffects = std::atomic_load(&effects);


        PreviewEvent ev;
        while (previewQueue.pop(ev)) {
            if (ev.velocity > 0.0f) {
                localInst->processMidi(0x90, ev.note, (int)(ev.velocity * 127.0f), 0);
                if (ev.note >= 0 && ev.note < 128) activeNotes[ev.note] = true;

                if (localEffects) {
                    for (const auto& effect : *localEffects) effect->processMidi(0x90, ev.note, (int)(ev.velocity * 127.0f), 0);
                }
            } else {
                localInst->processMidi(0x80, ev.note, 0, 0);
                if (ev.note >= 0 && ev.note < 128) activeNotes[ev.note] = false;

                if (localEffects) {
                    for (const auto& effect : *localEffects) effect->processMidi(0x80, ev.note, 0, 0);
                }
            }
        }


        RawMidiMessage rawMsg;
        while (liveMidiQueue.pop(rawMsg)) {
            const juce::uint8* data = rawMsg.data;
            int size = rawMsg.size;
            if (size >= 3) {
                localInst->processMidi(data[0], data[1], data[2], 0);
                if (localEffects) {
                    for (const auto& effect : *localEffects)
                        effect->processMidi(data[0], data[1], data[2], 0);
                }


                int status = data[0] & 0xF0;
                int note = data[1] & 0x7F;
                int vel = data[2] & 0x7F;

                if (status == 0x90 && vel > 0) {
                     activeNotes[note] = true;
                } else if (status == 0x80 || (status == 0x90 && vel == 0)) {
                     activeNotes[note] = false;
                }
            }
        }

        if (gHostContext.playing) {
            for (const auto& clip : *localMidiClips) {
                uint64_t clipStart = clip->getStartFrame();
                uint64_t clipLen = clip->getLengthFrames();
                uint64_t clipEnd = clipStart + clipLen;

                if (clipEnd < frameStart || clipStart >= frameStart + numSamples) continue;

                const auto& notes = clip->getNotes();
                uint64_t clipOffset = clip->getOffsetFrames();
                uint64_t maxNoteLen = clip->getMaxNoteLength();







                bool useOptimization = !clip->isDirty();
                auto it = notes.begin();
                int64_t relBlockEnd = 0;

                if (useOptimization) {
                    int64_t relFrameStart = (int64_t)frameStart - (int64_t)clipStart + (int64_t)clipOffset;
                    int64_t searchStart = relFrameStart - (int64_t)maxNoteLen;
                    if (searchStart < 0) searchStart = 0;

                    it = std::lower_bound(notes.begin(), notes.end(), (uint64_t)searchStart,
                        [](const MidiNote& note, uint64_t val) {
                            return note.startFrame < val;
                        });


                    relBlockEnd = relFrameStart + (int64_t)numSamples;
                }

                for (; it != notes.end(); ++it) {
                    const auto& note = *it;


                    if (useOptimization && (int64_t)note.startFrame >= relBlockEnd) break;


                    int64_t relStart = (int64_t)note.startFrame - (int64_t)clipOffset;


                    int64_t relEnd = relStart + (int64_t)note.lengthFrames;
                    if (relEnd <= 0 || relStart >= (int64_t)clipLen) continue;

                    int64_t globalStart = (int64_t)clipStart + relStart;
                    int64_t globalEnd = globalStart + (int64_t)note.lengthFrames;

                    if (globalEnd > (int64_t)clipEnd) {
                        globalEnd = (int64_t)clipEnd;
                    }


                    if (globalStart >= (int64_t)frameStart && globalStart < (int64_t)(frameStart + numSamples)) {
                        int offset = (int)(globalStart - (int64_t)frameStart);
                        localInst->processMidi(0x90, note.noteNumber, (int)(note.velocity * 127.0f), offset);
                        if (note.noteNumber >= 0 && note.noteNumber < 128) activeNotes[note.noteNumber] = true;

                        if (localEffects) {
                            for (const auto& effect : *localEffects) effect->processMidi(0x90, note.noteNumber, (int)(note.velocity * 127.0f), offset);
                        }
                    }


                    if (globalEnd >= (int64_t)frameStart && globalEnd < (int64_t)(frameStart + numSamples)) {
                        int offset = (int)(globalEnd - (int64_t)frameStart);
                        localInst->processMidi(0x80, note.noteNumber, 0, offset);
                        if (note.noteNumber >= 0 && note.noteNumber < 128) activeNotes[note.noteNumber] = false;

                        if (localEffects) {
                            for (const auto& effect : *localEffects) effect->processMidi(0x80, note.noteNumber, 0, offset);
                        }
                    }
                }
            }
        }


        const AudioBlock& instOut = localInst->getOutput(cacheKey, frameStart);


        for(size_t c=0; c<internalChannels; ++c) {
            const float* src = nullptr;
            if (instOut.getChannelCount() > c) src = instOut.getChannelPointer(c);
            else if (instOut.getChannelCount() > 0) src = instOut.getChannelPointer(0);

            if (src) {
                float* dst = localBlock.getChannelPointer(c);
                for (size_t i = 0; i < numSamples; ++i) {
                    dst[i] += src[i];
                }
            }
        }



        for (const auto& effect : *localEffects) {
            if (effect->getSampleRate() != getSampleRate()) {
                effect->setSampleRate(getSampleRate());
            }
            effect->processAudio(localBlock, cacheKey, frameStart);
        }


        if (preFaderBlock.getSampleCount() != numSamples || preFaderBlock.getChannelCount() != internalChannels) {
            preFaderBlock.resize(internalChannels, numSamples);
        }
        preFaderBlock.copyFrom(localBlock);


        auto volAuto = std::atomic_load(&volumeAutomation);
        auto panAuto = std::atomic_load(&panAutomation);

        float baseVol = Orion::clampAutomationValue(Orion::AutomationParameter::Volume, volumeAtomic.load());
        float basePan = Orion::clampAutomationValue(Orion::AutomationParameter::Pan, panAtomic.load());

        float maxL = 0.0f;
        float maxR = 0.0f;

        for (size_t i = 0; i < numSamples; ++i) {
            uint64_t currentFrame = frameStart + i;


            float currentVol = baseVol;
            if (volAuto && !volAuto->getPoints().empty()) {
                currentVol = Orion::clampAutomationValue(Orion::AutomationParameter::Volume, volAuto->getValueAt(currentFrame));
            }


            float currentPan = basePan;
            if (panAuto && !panAuto->getPoints().empty()) {
                currentPan = Orion::clampAutomationValue(Orion::AutomationParameter::Pan, panAuto->getValueAt(currentFrame));
            }


            float gainL = currentVol;
            float gainR = currentVol;

            float pan = Orion::clampAutomationValue(Orion::AutomationParameter::Pan, currentPan);

            // Equal Power (-3dB) Pan Law
            float angle = (pan + 1.0f) * ((float)M_PI * 0.25f);
            gainL *= std::cos(angle);
            gainR *= std::sin(angle);

            float sL = (internalChannels >= 1) ? localBlock.getChannelPointer(0)[i] : 0.0f;
            float sR = (internalChannels >= 2) ? localBlock.getChannelPointer(1)[i] : sL;

            float outL = sL * gainL;
            float outR = sR * gainR;

            float absL = std::abs(outL);
            float absR = std::abs(outR);

            if (absL > maxL) maxL = absL;
            if (absR > maxR) maxR = absR;

            if (outChannels >= 1) block.getChannelPointer(0)[i] = outL;
            if (outChannels >= 2) block.getChannelPointer(1)[i] = outR;
        }

        float currentL = levelL.load();
        float currentR = levelR.load();

        if (maxL > currentL) levelL = maxL;
        else levelL = currentL * 0.95f;

        if (maxR > currentR) levelR = maxR;
        else levelR = currentR * 0.95f;

        if (isMuted) {
            block.zero();
        }
    }

}
