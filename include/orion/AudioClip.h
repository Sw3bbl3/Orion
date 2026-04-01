#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <atomic>
#include "AudioFile.h"
#include "TimeStretcher.h"
#include "AudioBlock.h"

namespace Orion {

    enum class SyncMode {
        Off,
        Resample,
        Stretch
    };

    class AudioClip {
    public:
        AudioClip();
        ~AudioClip();


        bool loadFromFile(const std::string& filePath, uint32_t targetSampleRate = 0);


        void setFile(std::shared_ptr<AudioFile> file);


        const std::string& getName() const { return name; }
        const std::string getFilePath() const { return file ? file->getFilePath() : ""; }


        uint64_t getStartFrame() const { return startFrame.load(); }
        uint64_t getLengthFrames() const { return lengthFrames.load(); }


        uint64_t getSourceStartFrame() const { return sourceStartFrame.load(); }

        uint32_t getChannels() const;
        uint32_t getSampleRate() const;


        void setStartFrame(uint64_t frame) { startFrame.store(frame); }
        void setLengthFrames(uint64_t length) { lengthFrames.store(length); }
        void setSourceStartFrame(uint64_t frame) { sourceStartFrame.store(frame); }
        void setName(const std::string& newName) { name = newName; }



        float getSample(uint32_t channel, uint64_t frameIndex) const;


        float getSampleLinear(uint32_t channel, double frameIndex) const;


        const float* getChannelData(uint32_t channel) const;

        std::shared_ptr<AudioFile> getFile() const { return file; }


        const float* getColor() const { return color; }
        void setColor(float r, float g, float b) {
            color[0] = r; color[1] = g; color[2] = b;
        }


        float getGain() const { return gain.load(); }
        void setGain(float g) { gain.store(g); }

        float getPitch() const { return pitch.load(); }
        void setPitch(float p) { pitch.store(p); }

        bool isPreservePitch() const { return preservePitch.load(); }
        void setPreservePitch(bool preserve) { preservePitch.store(preserve); }


        uint64_t getFadeIn() const { return fadeIn.load(); }
        void setFadeIn(uint64_t frames) { fadeIn.store(frames); }

        uint64_t getFadeOut() const { return fadeOut.load(); }
        void setFadeOut(uint64_t frames) { fadeOut.store(frames); }

        float getFadeInCurve() const { return fadeInCurve.load(); }
        void setFadeInCurve(float curve) { fadeInCurve.store(curve); }

        float getFadeOutCurve() const { return fadeOutCurve.load(); }
        void setFadeOutCurve(float curve) { fadeOutCurve.store(curve); }

        bool isLooping() const { return loop.load(); }
        void setLooping(bool l) { loop.store(l); }


        void setSyncMode(SyncMode mode) { syncMode.store(mode); }
        SyncMode getSyncMode() const { return syncMode.load(); }

        void setOriginalBpm(float bpm) { originalBpm.store(bpm); }
        float getOriginalBpm() const { return originalBpm.load(); }

        void setReverse(bool r) { reverse.store(r); }
        bool isReverse() const { return reverse.load(); }


        void prepareToPlay(double sampleRate, size_t maxBlockSize);




        void process(AudioBlock& output, uint64_t timelineStartFrame, double projectBpm);





        std::shared_ptr<AudioClip> split(uint64_t splitPoint);


        std::shared_ptr<AudioClip> clone() const;


        void normalize();

    private:
        std::string name;
        float color[3] = { 0.0f, 0.7f, 0.7f };

        std::atomic<uint64_t> startFrame;
        std::atomic<uint64_t> lengthFrames;

        std::atomic<uint64_t> sourceStartFrame;

        std::atomic<float> gain { 1.0f };
        std::atomic<float> pitch { 0.0f };

        std::atomic<uint64_t> fadeIn { 0 };
        std::atomic<uint64_t> fadeOut { 0 };
        std::atomic<float> fadeInCurve { 0.0f };
        std::atomic<float> fadeOutCurve { 0.0f };
        std::atomic<bool> loop { false };
        std::atomic<bool> reverse { false };

        std::shared_ptr<AudioFile> file;


        std::atomic<SyncMode> syncMode { SyncMode::Off };
        std::atomic<float> originalBpm { 0.0f };
        std::atomic<bool> preservePitch { false };

        TimeStretcher timeStretcher;
        std::atomic<double> lastProcessedFrame { -1.0 };

        std::atomic<float> engineSampleRate { 44100.0f };
        std::vector<std::vector<float>> renderBuffer;
    };

}
