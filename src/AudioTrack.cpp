#include "orion/AudioTrack.h"
#include "orion/HostContext.h"
#include "orion/GainNode.h"
#include <algorithm>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Orion {

    class PreFaderNode : public AudioNode {
    public:
        PreFaderNode(AudioTrack* t) : track(t) {}
        std::string getName() const override { return track->getName() + " (Pre)"; }

        const AudioBlock& getOutput(uint64_t cacheKey, uint64_t projectTimeSamples) override {

            track->getOutput(cacheKey, projectTimeSamples);
            return track->preFaderBlock;
        }

    private:
        AudioTrack* track;
    };

    void AudioTrack::setFormat(size_t numChannels, size_t numSamples) {
        AudioNode::setFormat(numChannels, numSamples);
        localBlock.resize(numChannels, numSamples);
        preFaderBlock.resize(numChannels, numSamples);

        if (preFaderNode) preFaderNode->setFormat(numChannels, numSamples);

        auto localEffects = std::atomic_load(&effects);
        if (localEffects) {
             for (const auto& effect : *localEffects) {
                 effect->setFormat(numChannels, numSamples);
             }
        }


        auto localClips = std::atomic_load(&clips);
        if (localClips) {
            for (const auto& clip : *localClips) {
                clip->prepareToPlay(sampleRate, numSamples);
            }
        }
    }

    AudioTrack::AudioTrack() {

        clips = std::make_shared<std::vector<std::shared_ptr<AudioClip>>>();
        effects = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>();


        preFaderNode = std::make_shared<PreFaderNode>(this);


        recordingChunks.emplace_back();
        recordingChunks.back().resize(RECORDING_CHUNK_SIZE);


        settings.color[0] = 0.5f + (rand() % 50) / 100.0f;
        settings.color[1] = 0.5f + (rand() % 50) / 100.0f;
        settings.color[2] = 0.5f + (rand() % 50) / 100.0f;
    }

    void AudioTrack::addClip(std::shared_ptr<AudioClip> clip) {
        std::lock_guard<std::mutex> lock(modificationMutex);


        if (getSampleRate() > 0 && blockSize > 0) {
            clip->prepareToPlay(getSampleRate(), blockSize);
        }

        auto currentClips = std::atomic_load(&clips);
        auto newClips = std::make_shared<std::vector<std::shared_ptr<AudioClip>>>(*currentClips);
        newClips->push_back(clip);
        std::atomic_store(&clips, std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>>(newClips));
    }

    void AudioTrack::removeClip(std::shared_ptr<AudioClip> clip) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        auto currentClips = std::atomic_load(&clips);
        auto newClips = std::make_shared<std::vector<std::shared_ptr<AudioClip>>>(*currentClips);

        auto it = std::remove(newClips->begin(), newClips->end(), clip);
        if (it != newClips->end()) {
            newClips->erase(it, newClips->end());
            std::atomic_store(&clips, std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>>(newClips));
        }
    }

    std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>> AudioTrack::getClips() const {
        return std::atomic_load(&clips);
    }

    void AudioTrack::resolveOverlaps() {
        std::lock_guard<std::mutex> lock(modificationMutex);

        auto currentClips = std::atomic_load(&clips);
        if (!currentClips || currentClips->empty()) return;


        auto newClips = std::make_shared<std::vector<std::shared_ptr<AudioClip>>>(*currentClips);


        std::sort(newClips->begin(), newClips->end(), [](const std::shared_ptr<AudioClip>& a, const std::shared_ptr<AudioClip>& b) {
            return a->getStartFrame() < b->getStartFrame();
        });


        for (size_t i = 0; i < newClips->size() - 1; ++i) {
            auto& a = (*newClips)[i];
            auto& b = (*newClips)[i+1];

            uint64_t aEnd = a->getStartFrame() + a->getLengthFrames();
            uint64_t bStart = b->getStartFrame();

            if (aEnd > bStart) {

                uint64_t overlap = aEnd - bStart;


                if (overlap > a->getLengthFrames()) overlap = a->getLengthFrames();
                if (overlap > b->getLengthFrames()) overlap = b->getLengthFrames();


                a->setFadeOut(overlap);
                b->setFadeIn(overlap);

                a->setFadeOutCurve(-0.35f);
                b->setFadeInCurve(-0.35f);
            }
        }

        std::atomic_store(&clips, std::shared_ptr<const std::vector<std::shared_ptr<AudioClip>>>(newClips));
    }

    void AudioTrack::addEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        if (!effect) return;


        effect->setSampleRate(getSampleRate());
        effect->setFormat(channels, blockSize);

        auto currentEffects = std::atomic_load(&effects);
        auto newEffects = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*currentEffects);
        newEffects->push_back(effect);
        std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(newEffects));
    }

    void AudioTrack::removeEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        auto currentEffects = std::atomic_load(&effects);
        auto newEffects = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*currentEffects);

        auto it = std::remove(newEffects->begin(), newEffects->end(), effect);
        if (it != newEffects->end()) {
            newEffects->erase(it, newEffects->end());
            std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(newEffects));
        }
    }

    std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> AudioTrack::getEffects() const {
        return std::atomic_load(&effects);
    }

    void AudioTrack::addSend(std::shared_ptr<AudioTrack> target, bool preFader) {
        if (!target || wouldCreateCycle(target)) return;
        std::lock_guard<std::mutex> lock(modificationMutex);

        auto gainNode = std::make_shared<GainNode>();
        gainNode->setGain(0.0f);

        if (preFader && preFaderNode) {
            gainNode->connectInput(preFaderNode.get());
        } else {
            gainNode->connectInput(this);
        }

        target->connectInput(gainNode.get());

        sends.push_back({target, gainNode, preFader});
    }

    void AudioTrack::removeSend(int index) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        if (index >= 0 && index < (int)sends.size()) {
            auto& send = sends[index];
            if (send.target && send.gainNode) {
                send.target->disconnectInput(send.gainNode.get());
                if (send.preFader && preFaderNode) {
                    send.gainNode->disconnectInput(preFaderNode.get());
                } else {
                    send.gainNode->disconnectInput(this);
                }
            }
            sends.erase(sends.begin() + index);
        }
    }

    std::vector<AudioTrack::Send> AudioTrack::getSends() const {
        std::lock_guard<std::mutex> lock(modificationMutex);
        return sends;
    }

    void AudioTrack::setOutputTarget(std::shared_ptr<AudioTrack> target) {
        if (target && wouldCreateCycle(target)) return;
        std::lock_guard<std::mutex> lock(modificationMutex);
        outputTarget = target;
    }

    bool AudioTrack::wouldCreateCycle(std::shared_ptr<AudioTrack> potentialTarget) const {
        if (!potentialTarget) return false;
        if (potentialTarget.get() == this) return true;

        std::vector<AudioTrack*> stack;
        stack.push_back(potentialTarget.get());

        std::vector<AudioTrack*> visited;
        visited.push_back(const_cast<AudioTrack*>(this));

        while (!stack.empty()) {
            AudioTrack* current = stack.back();
            stack.pop_back();

            if (current == this) return true;

            if (std::find(visited.begin(), visited.end(), current) != visited.end())
                continue;

            visited.push_back(current);

            auto target = current->getOutputTarget();
            if (target) stack.push_back(target.get());

            auto trackSends = current->getSends();
            for (const auto& send : trackSends) {
                if (send.target) stack.push_back(send.target.get());
            }
        }

        return false;
    }

    std::shared_ptr<AudioTrack> AudioTrack::getOutputTarget() const {
        std::lock_guard<std::mutex> lock(modificationMutex);
        return outputTarget;
    }

    void AudioTrack::setVolume(float vol) {
        settings.volume = vol;
        volumeAtomic.store(vol);
    }

    void AudioTrack::setPan(float pan) {
        settings.pan = pan;
        panAtomic.store(pan);
    }

    void AudioTrack::setMute(bool mute) {
        settings.mute = mute;
        muteAtomic.store(mute);
    }

    void AudioTrack::setSolo(bool solo) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        settings.solo = solo;
    }

    void AudioTrack::setSoloSafe(bool safe) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        settings.soloSafe = safe;
    }

    bool AudioTrack::isSoloSafe() const {
        std::lock_guard<std::mutex> lock(modificationMutex);
        return settings.soloSafe;
    }

    void AudioTrack::setPhaseInverted(bool inverted) {
        phaseInverted.store(inverted);
    }

    bool AudioTrack::isPhaseInverted() const {
        return phaseInverted.load();
    }

    void AudioTrack::setForceMono(bool mono) {
        forceMono.store(mono);
    }

    bool AudioTrack::isForceMono() const {
        return forceMono.load();
    }

    void AudioTrack::setName(const std::string& name) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        settings.name = name;


        std::string lowerName = name;
        std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                       [](unsigned char c){ return (char)std::tolower(c); });

        auto setCol = [&](float r, float g, float b) {
            settings.color[0] = r;
            settings.color[1] = g;
            settings.color[2] = b;

            auto currentClips = std::atomic_load(&clips);
            if (currentClips) {
                 for (const auto& clip : *currentClips) {
                     clip->setColor(r, g, b);
                 }
            }
        };

        if (lowerName.find("kick") != std::string::npos || lowerName.find("bd") != std::string::npos) {
            setCol(0.8f, 0.2f, 0.2f);
        } else if (lowerName.find("snare") != std::string::npos || lowerName.find("clap") != std::string::npos) {
            setCol(0.9f, 0.8f, 0.2f);
        } else if (lowerName.find("hat") != std::string::npos || lowerName.find("cymbal") != std::string::npos || lowerName.find("oh") != std::string::npos) {
            setCol(0.9f, 0.5f, 0.1f);
        } else if (lowerName.find("bass") != std::string::npos || lowerName.find("808") != std::string::npos || lowerName.find("sub") != std::string::npos) {
            setCol(0.2f, 0.4f, 0.9f);
        } else if (lowerName.find("vocal") != std::string::npos || lowerName.find("vox") != std::string::npos) {
            setCol(0.9f, 0.4f, 0.7f);
        } else if (lowerName.find("guitar") != std::string::npos || lowerName.find("gtr") != std::string::npos) {
            setCol(0.2f, 0.7f, 0.2f);
        } else if (lowerName.find("piano") != std::string::npos || lowerName.find("keys") != std::string::npos) {
            setCol(0.8f, 0.8f, 0.9f);
        } else if (lowerName.find("synth") != std::string::npos || lowerName.find("lead") != std::string::npos) {
            setCol(0.6f, 0.2f, 0.8f);
        } else if (lowerName.find("pad") != std::string::npos || lowerName.find("atm") != std::string::npos) {
            setCol(0.2f, 0.7f, 0.7f);
        } else if (lowerName.find("fx") != std::string::npos || lowerName.find("sfx") != std::string::npos) {
            setCol(0.5f, 0.5f, 0.5f);
        }
    }

    std::string AudioTrack::getName() const {
        std::lock_guard<std::mutex> lock(modificationMutex);
        return settings.name;
    }

    const float* AudioTrack::getColor() const {
        return settings.color;
    }

    void AudioTrack::setColor(float r, float g, float b) {
        std::lock_guard<std::mutex> lock(modificationMutex);
        settings.color[0] = r;
        settings.color[1] = g;
        settings.color[2] = b;

        auto currentClips = std::atomic_load(&clips);
        if (currentClips) {
             for (const auto& clip : *currentClips) {
                 clip->setColor(r, g, b);
             }
        }
    }

    void AudioTrack::setArmed(bool _armed) {
        std::lock_guard<std::mutex> lock(recordingMutex);
        if (_armed && !armed) {

            if (!recordingRingBuffer) {
                recordingRingBuffer = std::make_unique<RingBuffer<float>>(44100 * 2 * 10); // 10 seconds of stereo
            }
        }
        armed = _armed;
    }

    bool AudioTrack::isArmed() const {
        std::lock_guard<std::mutex> lock(recordingMutex);
        return armed;
    }

    void AudioTrack::setInputChannel(int channel) {
        std::lock_guard<std::mutex> lock(recordingMutex);
        inputChannel = channel;
    }

    int AudioTrack::getInputChannel() const {
        std::lock_guard<std::mutex> lock(recordingMutex);
        return inputChannel;
    }

    void AudioTrack::appendRecordingData(const float* input, size_t numFrames, size_t numChannels) {
        if (!armed || !recordingRingBuffer) return;

        for (size_t i = 0; i < numFrames; ++i) {
            float l = 0.0f;
            float r = 0.0f;
            if (inputChannel == -1) {
                l = (numChannels >= 1) ? input[i * numChannels + 0] : 0.0f;
                r = (numChannels >= 2) ? input[i * numChannels + 1] : l;
            } else {
                if ((size_t)inputChannel < numChannels) {
                    l = input[i * numChannels + inputChannel];
                    r = l;
                }
            }
            recordingRingBuffer->push(l);
            recordingRingBuffer->push(r);
        }
    }

    std::vector<float> AudioTrack::takeRecordingBuffer() {
        std::lock_guard<std::mutex> lock(recordingMutex);

        size_t totalCaptured = 0;
        for (size_t i = 0; i <= currentChunkIndex && i < recordingChunks.size(); ++i) {
            totalCaptured += (i == currentChunkIndex) ? currentChunkPos : recordingChunks[i].size();
        }

        size_t inRing = recordingRingBuffer ? recordingRingBuffer->availableRead() : 0;
        std::vector<float> result(totalCaptured + inRing);

        size_t offset = 0;
        for (size_t i = 0; i <= currentChunkIndex && i < recordingChunks.size(); ++i) {
            size_t count = (i == currentChunkIndex) ? currentChunkPos : recordingChunks[i].size();
            if (count > 0) {
                std::copy(recordingChunks[i].begin(), recordingChunks[i].begin() + count, result.begin() + offset);
                offset += count;
            }
        }

        if (inRing > 0) {
            recordingRingBuffer->read(result.data() + offset, inRing);
            offset += inRing;
        }


        recordingChunks.clear();
        recordingChunks.emplace_back();
        recordingChunks.back().resize(RECORDING_CHUNK_SIZE);
        currentChunkIndex = 0;
        currentChunkPos = 0;

        return result;
    }

    bool AudioTrack::hasRecordingData() const {
         std::lock_guard<std::mutex> lock(recordingMutex);
         if (currentChunkIndex > 0) return true;
         if (currentChunkPos > 0) return true;
         if (recordingRingBuffer && recordingRingBuffer->availableRead() > 0) return true;
         return false;
    }

    int AudioTrack::getLatency() const {
        int total = 0;
        auto localEffects = std::atomic_load(&effects);
        if (localEffects) {
             for (const auto& effect : *localEffects) {
                 total += effect->getLatency();
             }
        }
        return total;
    }

    void AudioTrack::setOutputLatency(int samples) {
        if (samples < 0) samples = 0;
        if (outputLatencySamples == samples) return;

        outputLatencySamples = samples;


        size_t required = samples + 8192;


        if (delayLines.empty()) delayLines.resize(2);

        for(auto& line : delayLines) {
            if (line.size() < required) {
                line.assign(required, 0.0f);
                delayWriteIndex = 0;
            }
        }
    }

    std::shared_ptr<const AutomationLane> AudioTrack::getVolumeAutomation() const {
        return std::atomic_load(&volumeAutomation);
    }

    void AudioTrack::setVolumeAutomation(std::shared_ptr<AutomationLane> lane) {
        std::atomic_store(&volumeAutomation, std::shared_ptr<const AutomationLane>(lane));
    }

    std::shared_ptr<const AutomationLane> AudioTrack::getPanAutomation() const {
        return std::atomic_load(&panAutomation);
    }

    void AudioTrack::setPanAutomation(std::shared_ptr<AutomationLane> lane) {
        std::atomic_store(&panAutomation, std::shared_ptr<const AutomationLane>(lane));
    }

    void AudioTrack::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        bool isMuted = muteAtomic.load() || soloMuted.load();

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


        auto localClips = std::atomic_load(&clips);

        if (gHostContext.playing) {

            for (const auto& clip : *localClips) {
                clip->process(localBlock, frameStart, gHostContext.bpm);
            }
        }


        auto localEffects = std::atomic_load(&effects);
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


        bool inverted = phaseInverted.load();
        bool mono = forceMono.load();

        if (inverted || mono) {
            for (size_t c = 0; c < internalChannels; ++c) {
                if (c >= localBlock.getChannelCount()) break;
                float* data = localBlock.getChannelPointer(c);

                if (inverted) {
                    for (size_t i = 0; i < numSamples; ++i) {
                        data[i] *= -1.0f;
                    }
                }
            }

            if (mono && internalChannels >= 2 && localBlock.getChannelCount() >= 2) {
                float* left = localBlock.getChannelPointer(0);
                float* right = localBlock.getChannelPointer(1);

                for (size_t i = 0; i < numSamples; ++i) {
                    float avg = (left[i] + right[i]) * 0.5f;
                    left[i] = avg;
                    right[i] = avg;
                }
            }
        }


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
            currentPan = Orion::clampAutomationValue(Orion::AutomationParameter::Pan, currentPan);


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


        if (outputLatencySamples > 0) {
            size_t nCh = block.getChannelCount();
            if (delayLines.size() < nCh) {

                delayLines.resize(nCh, std::vector<float>(outputLatencySamples + 8192, 0.0f));
            }

            for (size_t c = 0; c < nCh; ++c) {
                float* data = block.getChannelPointer(c);
                auto& line = delayLines[c];


                if (line.size() < (size_t)(outputLatencySamples + numSamples)) {
                    line.resize(outputLatencySamples + numSamples + 8192, 0.0f);
                }

                size_t bufSize = line.size();
                size_t writePos = delayWriteIndex;
                size_t readPos = (delayWriteIndex + bufSize - outputLatencySamples) % bufSize;

                for (size_t i = 0; i < numSamples; ++i) {
                    float in = data[i];
                    line[writePos] = in;
                    data[i] = line[readPos];

                    writePos++;
                    if (writePos >= bufSize) writePos = 0;

                    readPos++;
                    if (readPos >= bufSize) readPos = 0;
                }
            }

            if (!delayLines.empty()) {
                 delayWriteIndex = (delayWriteIndex + numSamples) % delayLines[0].size();
            }
        }
    }

}
