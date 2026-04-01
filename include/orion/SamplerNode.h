#pragma once

#include "EffectNode.h"
#include "AudioFile.h"
#include <vector>
#include <array>
#include <string>
#include <mutex>
#include <memory>

namespace Orion {

    struct SamplerVoice {
        bool active = false;
        int note = 0;
        float velocity = 0.0f;
        double samplePosition = 0.0;
        double sampleStart = 0.0;
        double sampleEnd = 0.0;
        float envelope = 0.0f;
        bool releasing = false;


        float attack = 0.005f;
        float decay = 0.1f;
        float sustain = 1.0f;
        float release = 0.3f;
        double envTime = 0.0;
        float releaseLevel = 0.0f;

        double speed = 1.0;
    };

    class SamplerNode : public EffectNode {
    public:
        SamplerNode();
        ~SamplerNode() = default;

        bool isInstrument() const override { return true; }

        std::string getName() const override { return "Comet"; }

        bool loadSample(const std::string& path);
        std::string getSampleName() const { return sampleName; }
        std::string getSamplePath() const { return samplePath; }
        std::shared_ptr<const AudioFile> getSample() const { return std::atomic_load(&currentSample); }

        void setAttack(float v) { attack = v; }
        void setDecay(float v) { decay = v; }
        void setSustain(float v) { sustain = v; }
        void setRelease(float v) { release = v; }

        float getAttack() const { return attack; }
        float getDecay() const { return decay; }
        float getSustain() const { return sustain; }
        float getRelease() const { return release; }

        void setRootNote(int v) { rootNote = v; }
        int getRootNote() const { return rootNote; }

        void setTune(float v) { tune = v; }
        float getTune() const { return tune; }

        void setStart(float v) { start = v; }
        void setEnd(float v) { end = v; }
        float getStart() const { return start; }
        float getEnd() const { return end; }

        void setLoop(bool v) { loopEnabled = v; }
        bool isLooping() const { return loopEnabled; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override { return 9; }
        ParameterRange getParameterRange(int index) const override;

        void processAudio(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
            process(block, cacheKey, frameStart);
        }

        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void processMidi(int status, int data1, int data2, int sampleOffset) override;

    private:
        static const int MAX_VOICES = 16;
        std::array<SamplerVoice, MAX_VOICES> voices;

        struct Event {
            enum Type { NoteOn, NoteOff } type;
            int note;
            float velocity;
            int offset;
        };
        std::vector<Event> eventQueue;
        std::mutex eventMutex;

        std::shared_ptr<const AudioFile> currentSample;
        std::string sampleName;
        std::string samplePath;

        int rootNote = 60;
        float tune = 0.0f;
        float start = 0.0f;
        float end = 1.0f;
        bool loopEnabled = false;

        float attack = 0.005f;
        float decay = 0.1f;
        float sustain = 1.0f;
        float release = 0.3f;
    };

}
