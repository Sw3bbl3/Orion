#pragma once

#include "AudioTrack.h"
#include "MidiClip.h"
#include "SynthesizerNode.h"
#include "RingBuffer.h"
#include <vector>
#include <memory>
#include <atomic>
#include <bitset>
#include <juce_audio_basics/juce_audio_basics.h>

namespace Orion {

    class InstrumentTrack : public AudioTrack {
    public:
        InstrumentTrack();
        ~InstrumentTrack() = default;

        void addMidiClip(std::shared_ptr<MidiClip> clip);
        void removeMidiClip(std::shared_ptr<MidiClip> clip);


        std::shared_ptr<const std::vector<std::shared_ptr<MidiClip>>> getMidiClips() const {
             return std::atomic_load(&midiClips);
        }


        SynthesizerNode* getSynth();
        SynthesizerNode* getSynthesizerNode() { return getSynth(); }

        std::shared_ptr<AudioNode> getInstrument() {
            return std::atomic_load(&instrument);
        }

        void setInstrument(std::shared_ptr<AudioNode> inst);

        void setFormat(size_t numChannels, size_t numSamples) override;


        int getLatency() const override;

        void setColor(float r, float g, float b) override;


        void allNotesOff();


        void startPreviewNote(int note, float velocity);
        void stopPreviewNote(int note);


        void injectLiveMidiMessage(const juce::MidiMessage& message);

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    private:

        std::shared_ptr<const std::vector<std::shared_ptr<MidiClip>>> midiClips;


        std::shared_ptr<AudioNode> instrument;


        struct PreviewEvent {
            int note;
            float velocity;
        };
        RingBuffer<PreviewEvent> previewQueue{256};

        struct RawMidiMessage {
            uint8_t data[128];
            int size;
        };
        RingBuffer<RawMidiMessage> liveMidiQueue{256};


        std::bitset<128> activeNotes;
    };

}
