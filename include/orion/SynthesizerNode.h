#pragma once

#include "EffectNode.h"
#include <vector>
#include <array>
#include <string>
#include <mutex>

namespace Orion {

    enum class Waveform {
        Sine,
        Triangle,
        Saw,
        Square
    };

    struct SynthVoice {
        bool active = false;
        int note = 0;
        float velocity = 0.0f;
        double phase = 0.0;
        float envelope = 0.0f;
        bool releasing = false;


        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.2f;

        double envTime = 0.0;


        double filterX1 = 0, filterX2 = 0, filterY1 = 0, filterY2 = 0;
    };

    class SynthesizerNode : public EffectNode {
    public:
        SynthesizerNode();
        ~SynthesizerNode() = default;

        bool isInstrument() const override { return true; }

        std::string getName() const override { return "Pulsar"; }

        void noteOn(int note, float velocity, int sampleOffset);
        void noteOff(int note, int sampleOffset);


        void allNotesOff();

        void processAudio(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
            process(block, cacheKey, frameStart);
        }


        void setWaveform(Waveform w) { waveform = w; }
        Waveform getWaveform() const { return waveform; }

        void setFilterCutoff(float hz) { filterCutoff = hz; }
        float getFilterCutoff() const { return filterCutoff; }

        void setFilterResonance(float res) { filterResonance = res; }
        float getFilterResonance() const { return filterResonance; }

        void setAttack(float v) { attack = v; }
        void setDecay(float v) { decay = v; }
        void setSustain(float v) { sustain = v; }
        void setRelease(float v) { release = v; }

        float getAttack() const { return attack; }
        float getDecay() const { return decay; }
        float getSustain() const { return sustain; }
        float getRelease() const { return release; }

        void setLfoRate(float v) { lfoRate = v; }
        void setLfoDepth(float v) { lfoDepth = v; }
        void setLfoTarget(int t) { lfoTarget = t; }

        float getLfoRate() const { return lfoRate; }
        float getLfoDepth() const { return lfoDepth; }
        int getLfoTarget() const { return lfoTarget; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 10; }
        ParameterRange getParameterRange(int index) const override;

        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void processMidi(int status, int data1, int data2, int sampleOffset) override;

    private:
        static const int MAX_VOICES = 16;
        std::array<SynthVoice, MAX_VOICES> voices;

        struct Event {
            enum Type { NoteOn, NoteOff } type;
            int note;
            float velocity;
            int offset;
        };
        std::vector<Event> eventQueue;
        std::mutex eventMutex;

        float mToF(int note);


        Waveform waveform = Waveform::Saw;
        float filterCutoff = 20000.0f;
        float filterResonance = 0.0f;

        float attack = 0.01f;
        float decay = 0.1f;
        float sustain = 0.7f;
        float release = 0.2f;

        float lfoRate = 3.0f;
        float lfoDepth = 0.0f;
        int lfoTarget = 0; // 0=Off, 1=Pitch, 2=Filter
        double lfoPhase = 0.0;
    };

}
