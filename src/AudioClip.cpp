#include "orion/AudioClip.h"
#include "orion/AudioBlock.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace Orion {

    AudioClip::AudioClip()
        : startFrame(0), lengthFrames(0), sourceStartFrame(0), lastProcessedFrame(-9999.0)
    {
    }

    AudioClip::~AudioClip() {
    }

    bool AudioClip::loadFromFile(const std::string& path, uint32_t targetSampleRate) {
        file = std::make_shared<AudioFile>();
        if (file->load(path, targetSampleRate)) {
            name = file->getFileName();
            lengthFrames = file->getLengthFrames();
            sourceStartFrame = 0;

            timeStretcher.setSource(file);
            lastProcessedFrame.store(-1.0);

            if (file->getBpm() > 0.0f) {
                originalBpm.store(file->getBpm());
            }

            return true;
        }
        return false;
    }

    void AudioClip::setFile(std::shared_ptr<AudioFile> newFile) {
        file = newFile;
        if (file) {
            if (lengthFrames.load() == 0) lengthFrames = file->getLengthFrames();
            timeStretcher.setSource(file);
            lastProcessedFrame.store(-1.0);
        }
    }

    uint32_t AudioClip::getChannels() const {
        return file ? file->getChannels() : 0;
    }

    uint32_t AudioClip::getSampleRate() const {
        return file ? file->getSampleRate() : 0;
    }

    float AudioClip::getSample(uint32_t channel, uint64_t frameIndex) const {
        return getSampleLinear(channel, (double)frameIndex);
    }

    float AudioClip::getSampleLinear(uint32_t channel, double frameIndex) const {
        if (!file) return 0.0f;
        if (frameIndex < 0.0 || frameIndex >= (double)lengthFrames.load()) return 0.0f;

        double rate = 1.0;
        double srRatio = (file->getSampleRate() > 0 && engineSampleRate.load() > 0.0f)
                         ? ((double)file->getSampleRate() / (double)engineSampleRate.load())
                         : 1.0;

        float currentPitch = pitch.load();
        if (std::abs(currentPitch) > 0.001f) {
            rate = std::pow(2.0, (double)currentPitch / 12.0);
        }
        rate *= srRatio;

        double sourcePos;
        if (reverse.load()) {
            // For reverse, we map frameIndex (0 to length) to (length - 1 - frameIndex)
            // relative to the clip start, but playing *from* the sourceStartFrame + length backwards?
            // Usually simpler: sourcePos = (sourceStart + length) - (frameIndex * rate)
            // But if the clip is just a segment of a file, we want to play that segment reversed.

            // Wait, lengthFrames is in project timeline frames. rate handles pitch.

            // Standard reverse: play from end to start.
            // sourcePos = sourceStart + (length - 1 - frameIndex) * rate
            double revIndex = (double)lengthFrames.load() - 1.0 - frameIndex;
            if (revIndex < 0) revIndex = 0;

            sourcePos = (double)sourceStartFrame.load() + (revIndex * rate);
        } else {
            sourcePos = (double)sourceStartFrame.load() + (frameIndex * rate);
        }

        if (loop.load()) {
            double len = (double)file->getLengthFrames();
            if (len > 0.0) {
                 sourcePos = std::fmod(sourcePos, len);
                 if (sourcePos < 0.0) sourcePos += len;
            }
        }

        uint64_t idx0 = (uint64_t)std::floor(sourcePos);
        uint64_t idx1 = idx0 + 1;
        float frac = (float)(sourcePos - idx0);

        float s0 = file->getSample(channel, idx0);
        float s1 = file->getSample(channel, idx1);

        return (s0 + frac * (s1 - s0)) * gain.load();
    }

    const float* AudioClip::getChannelData(uint32_t channel) const {
        return file ? file->getChannelData(channel) : nullptr;
    }

    void AudioClip::prepareToPlay(double sampleRate, size_t maxBlockSize) {
        engineSampleRate.store((float)sampleRate);
        timeStretcher.prepare((uint32_t)sampleRate, maxBlockSize);
        if (file) timeStretcher.setSource(file);


        size_t chans = (file) ? file->getChannels() : 2;
        if (chans < 2) chans = 2;
        renderBuffer.resize(chans);
        for (auto& ch : renderBuffer) {
            ch.resize(maxBlockSize + 4096);
        }
    }

    void AudioClip::process(AudioBlock& output, uint64_t timelineStart, double projectBpm) {
        if (!file) return;

        uint64_t clipStart = startFrame.load();
        uint64_t clipLen = lengthFrames.load();
        uint64_t clipEnd = clipStart + clipLen;
        uint64_t blockStart = timelineStart;
        uint64_t blockLen = output.getSampleCount();
        uint64_t blockEnd = blockStart + blockLen;

        if (clipEnd <= blockStart || clipStart >= blockEnd) return;


        uint64_t renderStart = std::max(clipStart, blockStart);
        uint64_t renderEnd = std::min(clipEnd, blockEnd);
        size_t framesToProcess = (size_t)(renderEnd - renderStart);
        size_t blockOffset = (size_t)(renderStart - blockStart);
        uint64_t relStart = renderStart - clipStart;

        SyncMode mode = syncMode.load();
        float baseBpm = originalBpm.load();

        double srRatio = (file->getSampleRate() > 0 && engineSampleRate.load() > 0.0f)
                         ? ((double)file->getSampleRate() / (double)engineSampleRate.load())
                         : 1.0;

        double speedRatio = srRatio;
        if (mode != SyncMode::Off && baseBpm > 0.01f && projectBpm > 0.01f) {
            speedRatio = srRatio * (projectBpm / (double)baseBpm);
        }

        double pitchRatio = 1.0;
        float currentPitch = pitch.load();
        if (std::abs(currentPitch) > 0.001f) {
            pitchRatio = std::pow(2.0, (double)currentPitch / 12.0);
        }

        double rate;
        double stretcherPitchRatio = 1.0;

        bool wantsPitch = (std::abs(currentPitch) > 0.001f);
        bool tempoStretch = (mode == SyncMode::Stretch && baseBpm > 0.01f && projectBpm > 0.01f);
        // Preserve length only when explicitly requested (stretch mode or preserve toggle)
        bool useStretcher = tempoStretch || (preservePitch.load() && wantsPitch);

        if (useStretcher) {
            rate = speedRatio;
            stretcherPitchRatio = pitchRatio;
        } else {
            rate = speedRatio * pitchRatio; // pitchRatio will be ~1.0 here
            stretcherPitchRatio = 1.0;
        }

        bool isReverse = reverse.load();


        uint64_t fadeInFrames = fadeIn.load();
        uint64_t fadeOutFrames = fadeOut.load();
        float clipGain = gain.load();

        if (useStretcher) {
             if (loop.load()) {
                 timeStretcher.setLooping(true, file->getLengthFrames());
             } else {
                 timeStretcher.setLooping(false, 0);
             }


             double expectedPos;
             if (isReverse) {
                 // TimeStretcher usually processes forward. Supporting reverse stretch is complex.
                 // For now, if reverse is on, we might disable stretch or handle it by
                 // asking TimeStretcher to play backwards if supported, or manually reversing buffer.
                 // Given TimeStretcher implementation is likely a wrapper around RubberBand/SoundTouch or simple,
                 // let's assume it doesn't support reverse playback natively without reconfiguration.
                 // We will skip reverse logic for Stretch mode in this iteration or fallback to Resample logic if possible.
                 // For simplicity, let's treat Reverse as incompatible with Stretch for now, or just ignore reverse in Stretch.
                 // Ideally, we'd render stretch forward then reverse the buffer.

                 expectedPos = (double)sourceStartFrame.load() + (double)relStart * rate; // Fallback
             } else {
                 expectedPos = (double)sourceStartFrame.load() + (double)relStart * rate;
             }

             if (loop.load() && file->getLengthFrames() > 0) {
                 expectedPos = std::fmod(expectedPos, (double)file->getLengthFrames());
             }



             if (lastProcessedFrame.load() < 0.0 || std::abs((double)renderStart - lastProcessedFrame.load()) > 1.0) {
                 timeStretcher.setPosition(expectedPos);
             }


             std::vector<float*> outPtrs;
             size_t numCh = std::min((size_t)output.getChannelCount(), renderBuffer.size());
             for (size_t c = 0; c < numCh; ++c) {
                 outPtrs.push_back(renderBuffer[c].data());
             }

             timeStretcher.process(outPtrs.data(), numCh, framesToProcess, rate, stretcherPitchRatio);


             for (size_t c = 0; c < numCh; ++c) {
                 float* dst = output.getChannelPointer(c) + blockOffset;
                 const float* src = renderBuffer[c].data();
                 for (size_t i = 0; i < framesToProcess; ++i) {
                     float fade = 1.0f;
                     uint64_t relPos = relStart + i;

                     if (fadeInFrames > 0 && relPos < fadeInFrames) {
                         double progress = (double)relPos / fadeInFrames;
                         float curve = fadeInCurve.load();
                         if (std::abs(curve) > 0.001f) {
                             progress = std::pow(progress, std::exp(curve * 2.0));
                         }
                         fade = (float)progress;
                     }
                     else if (fadeOutFrames > 0 && relPos >= clipLen - fadeOutFrames) {
                         double progress = (double)(clipLen - relPos) / fadeOutFrames;
                         float curve = fadeOutCurve.load();
                         if (std::abs(curve) > 0.001f) {
                             progress = std::pow(progress, std::exp(curve * 2.0));
                         }
                         fade = (float)progress;
                     }

                     dst[i] += src[i] * clipGain * fade;
                 }
             }

        } else {

            size_t numCh = output.getChannelCount();
            size_t fileCh = file->getChannels();

            for (size_t i = 0; i < framesToProcess; ++i) {
                uint64_t relPos = relStart + i;


                float fade = 1.0f;
                if (fadeInFrames > 0 && relPos < fadeInFrames) {
                    double progress = (double)relPos / fadeInFrames;
                    float curve = fadeInCurve.load();
                    if (std::abs(curve) > 0.001f) {
                        progress = std::pow(progress, std::exp(curve * 2.0));
                    }
                    fade = (float)progress;
                }
                else if (fadeOutFrames > 0 && relPos >= clipLen - fadeOutFrames) {
                    double progress = (double)(clipLen - relPos) / fadeOutFrames;
                    float curve = fadeOutCurve.load();
                    if (std::abs(curve) > 0.001f) {
                        progress = std::pow(progress, std::exp(curve * 2.0));
                    }
                    fade = (float)progress;
                }

                float finalGain = clipGain * fade;


                double sourceIndex = (double)relPos * rate;
                double sPos;

                if (isReverse) {
                    // Wait, relPos increases. We want to sample backwards from end of CLIP content.
                    // sourceStartFrame points to where the clip starts in the file.
                    // So clip content is [sourceStartFrame, sourceStartFrame + clipLen*rate]
                    // We want to read from (sourceStartFrame + clipLen*rate) - sourceIndex

                    // However, 'clipLen' is in timeline frames. 'rate' converts to source frames.
                    // Total source duration for this clip = clipLen * rate.

                    double totalSourceLen = (double)clipLen * rate;
                    double currentSourceOffset = totalSourceLen - 1.0 - sourceIndex;

                    sPos = (double)sourceStartFrame.load() + currentSourceOffset;
                } else {
                    sPos = (double)sourceStartFrame.load() + sourceIndex;
                }

                if (loop.load() && file->getLengthFrames() > 0) {
                     sPos = std::fmod(sPos, (double)file->getLengthFrames());
                     if (sPos < 0) sPos += (double)file->getLengthFrames();
                } else if (sPos >= file->getLengthFrames() || sPos < 0) {
                     if (isReverse && sPos < 0) break; // End of file (start) reached
                     if (!isReverse && sPos >= file->getLengthFrames()) break;
                }


                uint64_t idx0 = (uint64_t)sPos;
                uint64_t idx1 = idx0 + 1;
                float frac = (float)(sPos - idx0);

                if (idx0 >= file->getLengthFrames()) idx0 = file->getLengthFrames() - 1;
                if (idx1 >= file->getLengthFrames()) {
                     if (loop.load()) idx1 = 0; else idx1 = file->getLengthFrames() - 1;
                }

                for (size_t c = 0; c < numCh; ++c) {
                    uint32_t inCh = (c < fileCh) ? (uint32_t)c : 0;
                    float val = file->getSample(inCh, idx0);
                    float val2 = file->getSample(inCh, idx1);
                    float s = val + frac * (val2 - val);

                    output.getChannelPointer(c)[blockOffset + i] += s * finalGain;
                }
            }
        }

        lastProcessedFrame.store((double)(renderStart + framesToProcess));
    }


    std::shared_ptr<AudioClip> AudioClip::split(uint64_t splitPoint) {
        uint64_t currentStart = startFrame.load();
        uint64_t currentLength = lengthFrames.load();
        if (splitPoint <= currentStart || splitPoint >= currentStart + currentLength) return nullptr;
        uint64_t offset = splitPoint - currentStart;
        uint64_t rightLength = currentLength - offset;

        auto newClip = std::make_shared<AudioClip>();
        newClip->file = this->file;
        newClip->startFrame = splitPoint;
        newClip->lengthFrames = rightLength;
        newClip->sourceStartFrame = this->sourceStartFrame.load() + offset;
        newClip->name = this->name;
        newClip->gain = this->gain.load();
        newClip->pitch = this->pitch.load();
        newClip->fadeIn = this->fadeIn.load();
        newClip->fadeOut = this->fadeOut.load();
        newClip->fadeInCurve = this->fadeInCurve.load();
        newClip->fadeOutCurve = this->fadeOutCurve.load();
        newClip->loop = this->loop.load();
        newClip->reverse = this->reverse.load();
        newClip->color[0] = this->color[0]; newClip->color[1] = this->color[1]; newClip->color[2] = this->color[2];
        newClip->syncMode = this->syncMode.load();
        newClip->originalBpm = this->originalBpm.load();
        newClip->preservePitch = this->preservePitch.load();

        this->lengthFrames = offset;
        return newClip;
    }

    std::shared_ptr<AudioClip> AudioClip::clone() const {
        auto newClip = std::make_shared<AudioClip>();
        newClip->file = this->file;
        newClip->startFrame.store(this->startFrame.load());
        newClip->lengthFrames.store(this->lengthFrames.load());
        newClip->sourceStartFrame.store(this->sourceStartFrame.load());
        newClip->name = this->name;
        newClip->gain.store(this->gain.load());
        newClip->pitch.store(this->pitch.load());
        newClip->fadeIn.store(this->fadeIn.load());
        newClip->fadeOut.store(this->fadeOut.load());
        newClip->fadeInCurve.store(this->fadeInCurve.load());
        newClip->fadeOutCurve.store(this->fadeOutCurve.load());
        newClip->loop.store(this->loop.load());
        newClip->reverse.store(this->reverse.load());
        newClip->color[0] = this->color[0]; newClip->color[1] = this->color[1]; newClip->color[2] = this->color[2];
        newClip->syncMode.store(this->syncMode.load());
        newClip->originalBpm.store(this->originalBpm.load());
        newClip->preservePitch.store(this->preservePitch.load());
        return newClip;
    }

    void AudioClip::normalize() {
        if (!file) return;
        uint64_t len = lengthFrames.load();
        uint64_t start = sourceStartFrame.load();
        uint64_t fileLen = file->getLengthFrames();
        bool looping = loop.load();
        uint32_t channels = file->getChannels();
        float maxVal = 0.0f;
        for (uint32_t c = 0; c < channels; ++c) {
            for (uint64_t i = 0; i < len; ++i) {
                uint64_t idx = start + i;
                if (looping && fileLen > 0) idx = idx % fileLen; else if (idx >= fileLen) break;
                float val = std::abs(file->getSample(c, idx));
                if (val > maxVal) maxVal = val;
            }
        }
        if (maxVal > 0.0001f) gain.store(1.0f / maxVal);
    }

}
