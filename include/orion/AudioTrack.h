#pragma once

#include "AudioNode.h"
#include "AudioClip.h"
#include "EffectNode.h"
#include "GainNode.h"
#include "Automation.h"
#include "RingBuffer.h"
#include <vector>
#include <memory>
#include <string>
#include <atomic>
#include <mutex>

namespace Orion {

    class PreFaderNode;

    struct TrackSettings {
        float volume = 1.0f;
        float pan = 0.0f;
        bool mute = false;
        bool solo = false;
        bool soloSafe = false;
        std::string name = "Track";
        float color[3] = {0.5f, 0.5f, 0.5f};
    };

    class AudioTrack : public AudioNode {
    public:
        AudioTrack();
        ~AudioTrack() = default;


        void addClip(std::shared_ptr<AudioClip> clip);
        void removeClip(std::shared_ptr<AudioClip> clip);

        std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>> getClips() const;


        void resolveOverlaps();


        void addEffect(std::shared_ptr<EffectNode> effect);
        void removeEffect(std::shared_ptr<EffectNode> effect);

        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> getEffects() const;


        void setOutputTarget(std::shared_ptr<AudioTrack> target);
        std::shared_ptr<AudioTrack> getOutputTarget() const;

        bool wouldCreateCycle(std::shared_ptr<AudioTrack> potentialTarget) const;


        struct Send {
            std::shared_ptr<AudioTrack> target;
            std::shared_ptr<GainNode> gainNode;
            bool preFader = false;
        };
        void addSend(std::shared_ptr<AudioTrack> target, bool preFader = false);
        void removeSend(int index);
        std::vector<Send> getSends() const;


        void setVolume(float vol);
        float getVolume() const { return volumeAtomic.load(); }

        void setFormat(size_t channels, size_t blockSize) override;

        void setPan(float pan);
        float getPan() const { return panAtomic.load(); }

        void setMute(bool mute);
        bool isMuted() const { return muteAtomic.load(); }

        void setSolo(bool solo);
        bool isSolo() const { return settings.solo; }


        std::shared_ptr<const AutomationLane> getVolumeAutomation() const;
        void setVolumeAutomation(std::shared_ptr<AutomationLane> lane);

        std::shared_ptr<const AutomationLane> getPanAutomation() const;
        void setPanAutomation(std::shared_ptr<AutomationLane> lane);

        void setName(const std::string& name);
        std::string getName() const override;

        const float* getColor() const;
        virtual void setColor(float r, float g, float b);

        float getLevelL() const { return levelL.load(); }
        float getLevelR() const { return levelR.load(); }


        void setSoloMuted(bool muted) { soloMuted = muted; }
        bool isSoloMuted() const { return soloMuted; }

        void setSoloSafe(bool safe);
        bool isSoloSafe() const;


        void setPhaseInverted(bool inverted);
        bool isPhaseInverted() const;


        void setForceMono(bool mono);
        bool isForceMono() const;


        void setArmed(bool armed);
        bool isArmed() const;


        void setInputChannel(int channel);
        int getInputChannel() const;

        void appendRecordingData(const float* input, size_t numFrames, size_t numChannels);
        std::vector<float> takeRecordingBuffer();
        bool hasRecordingData() const;


        int getLatency() const override;
        void setOutputLatency(int samples);


        void setShowAutomation(bool show) { showAutomation = show; }
        bool isAutomationVisible() const { return showAutomation; }

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;

    protected:
        mutable std::mutex modificationMutex;
        mutable std::mutex recordingMutex;


        std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> effects;
        std::vector<Send> sends;

        std::shared_ptr<AudioTrack> outputTarget;

        TrackSettings settings;


        std::atomic<float> volumeAtomic { 1.0f };
        std::atomic<float> panAtomic { 0.0f };
        std::atomic<bool> muteAtomic { false };

        std::atomic<float> levelL { 0.0f };
        std::atomic<float> levelR { 0.0f };
        std::atomic<bool> soloMuted { false };

        std::atomic<bool> phaseInverted { false };
        std::atomic<bool> forceMono { false };

        bool armed = false;
        int inputChannel = -1;
        bool showAutomation = false;


        static const size_t RECORDING_CHUNK_SIZE = 1024 * 1024;
        std::vector<std::vector<float>> recordingChunks;
        size_t currentChunkIndex = 0;
        size_t currentChunkPos = 0;
        std::unique_ptr<RingBuffer<float>> recordingRingBuffer;


        AudioBlock localBlock;


        std::vector<std::vector<float>> delayLines;
        size_t delayWriteIndex = 0;
        int outputLatencySamples = 0;

    protected:
        friend class PreFaderNode;
        std::shared_ptr<PreFaderNode> preFaderNode;
        AudioBlock preFaderBlock;


        std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>> clips;


        std::shared_ptr<const AutomationLane> volumeAutomation;
        std::shared_ptr<const AutomationLane> panAutomation;
    };

}
