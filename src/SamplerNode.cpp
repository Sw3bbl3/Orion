#include "orion/SamplerNode.h"
#include <juce_core/juce_core.h>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace Orion {

    SamplerNode::SamplerNode() {

        for (auto& v : voices) {
            v.active = false;
        }
        attack = 0.005f;
        decay = 0.1f;
        sustain = 1.0f;
        release = 0.3f;
        rootNote = 60;
        tune = 0.0f;
        start = 0.0f;
        end = 1.0f;
        loopEnabled = false;
    }

    bool SamplerNode::loadSample(const std::string& path) {
        auto newSample = std::make_shared<AudioFile>();
        bool success = newSample->load(path, (uint32_t)getSampleRate());
        if (success) {
            std::atomic_store(&currentSample, std::shared_ptr<const AudioFile>(newSample));
            sampleName = newSample->getFileName();
            samplePath = path;
        } else {



        }
        return success;
    }

    void SamplerNode::processMidi(int status, int data1, int data2, int sampleOffset) {
        std::unique_lock<std::mutex> lock(eventMutex, std::try_to_lock);
        if (!lock.owns_lock()) return;

        int type = status & 0xF0;
        if (type == 0x90 && data2 > 0) {
            eventQueue.push_back({ Event::NoteOn, data1, (float)data2 / 127.0f, sampleOffset });
        } else if (type == 0x80 || (type == 0x90 && data2 == 0)) {
            eventQueue.push_back({ Event::NoteOff, data1, 0.0f, sampleOffset });
        }
    }

    bool SamplerNode::getChunk(std::vector<char>& data) {
        uint32_t pathLen = (uint32_t)samplePath.length();
        data.resize(sizeof(pathLen) + pathLen + sizeof(rootNote)
                    + sizeof(tune) + sizeof(start) + sizeof(end) + sizeof(loopEnabled)
                    + sizeof(attack) + sizeof(decay) + sizeof(sustain) + sizeof(release));
        char* ptr = data.data();
        std::memcpy(ptr, &pathLen, sizeof(pathLen)); ptr += sizeof(pathLen);
        if (pathLen > 0) {
            std::memcpy(ptr, samplePath.data(), pathLen); ptr += pathLen;
        }
        std::memcpy(ptr, &rootNote, sizeof(rootNote));
        ptr += sizeof(rootNote);
        std::memcpy(ptr, &tune, sizeof(tune)); ptr += sizeof(tune);
        std::memcpy(ptr, &start, sizeof(start)); ptr += sizeof(start);
        std::memcpy(ptr, &end, sizeof(end)); ptr += sizeof(end);
        std::memcpy(ptr, &loopEnabled, sizeof(loopEnabled)); ptr += sizeof(loopEnabled);
        std::memcpy(ptr, &attack, sizeof(attack)); ptr += sizeof(attack);
        std::memcpy(ptr, &decay, sizeof(decay)); ptr += sizeof(decay);
        std::memcpy(ptr, &sustain, sizeof(sustain)); ptr += sizeof(sustain);
        std::memcpy(ptr, &release, sizeof(release)); ptr += sizeof(release);
        return true;
    }

    bool SamplerNode::setChunk(const std::vector<char>& data) {
        if (data.size() < sizeof(uint32_t)) return false;
        const char* ptr = data.data();
        uint32_t pathLen = 0;
        std::memcpy(&pathLen, ptr, sizeof(pathLen)); ptr += sizeof(pathLen);
        if (data.size() < sizeof(uint32_t) + pathLen + sizeof(rootNote)) return false;
        if (pathLen > 0) {
            samplePath.assign(ptr, pathLen); ptr += pathLen;
            loadSample(samplePath);
        }
        std::memcpy(&rootNote, ptr, sizeof(rootNote));
        ptr += sizeof(rootNote);
        if ((size_t)(ptr - data.data()) + sizeof(tune) <= data.size()) {
            std::memcpy(&tune, ptr, sizeof(tune)); ptr += sizeof(tune);
            std::memcpy(&start, ptr, sizeof(start)); ptr += sizeof(start);
            std::memcpy(&end, ptr, sizeof(end)); ptr += sizeof(end);
            std::memcpy(&loopEnabled, ptr, sizeof(loopEnabled)); ptr += sizeof(loopEnabled);
            std::memcpy(&attack, ptr, sizeof(attack)); ptr += sizeof(attack);
            std::memcpy(&decay, ptr, sizeof(decay)); ptr += sizeof(decay);
            std::memcpy(&sustain, ptr, sizeof(sustain)); ptr += sizeof(sustain);
            std::memcpy(&release, ptr, sizeof(release)); ptr += sizeof(release);
        }
        return true;
    }

    void SamplerNode::setParameter(int index, float value) {
        switch (index) {
            case 0: attack = std::clamp(value, 0.001f, 5.0f); break;
            case 1: decay = std::clamp(value, 0.001f, 5.0f); break;
            case 2: sustain = std::clamp(value, 0.0f, 1.0f); break;
            case 3: release = std::clamp(value, 0.001f, 5.0f); break;
            case 4: rootNote = std::clamp((int)std::round(value), 0, 127); break;
            case 5: tune = std::clamp(value, -12.0f, 12.0f); break;
            case 6: start = std::clamp(value, 0.0f, 0.98f); break;
            case 7: end = std::clamp(value, 0.02f, 1.0f); break;
            case 8: loopEnabled = (value >= 0.5f); break;
            default: break;
        }
        if (end <= start + 0.01f) end = std::min(1.0f, start + 0.02f);
    }

    float SamplerNode::getParameter(int index) const {
        switch (index) {
            case 0: return attack;
            case 1: return decay;
            case 2: return sustain;
            case 3: return release;
            case 4: return (float)rootNote;
            case 5: return tune;
            case 6: return start;
            case 7: return end;
            case 8: return loopEnabled ? 1.0f : 0.0f;
            default: return 0.0f;
        }
    }

    std::string SamplerNode::getParameterName(int index) const {
        switch (index) {
            case 0: return "Attack";
            case 1: return "Decay";
            case 2: return "Sustain";
            case 3: return "Release";
            case 4: return "Root Note";
            case 5: return "Tune";
            case 6: return "Start";
            case 7: return "End";
            case 8: return "Loop";
            default: return "";
        }
    }

    ParameterRange SamplerNode::getParameterRange(int index) const {
        switch (index) {
            case 0: return {0.001f, 5.0f, 0.001f};
            case 1: return {0.001f, 5.0f, 0.001f};
            case 2: return {0.0f, 1.0f, 0.01f};
            case 3: return {0.001f, 5.0f, 0.001f};
            case 4: return {0.0f, 127.0f, 1.0f};
            case 5: return {-12.0f, 12.0f, 0.01f};
            case 6: return {0.0f, 0.98f, 0.001f};
            case 7: return {0.02f, 1.0f, 0.001f};
            case 8: return {0.0f, 1.0f, 1.0f};
            default: return EffectNode::getParameterRange(index);
        }
    }

    void SamplerNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        juce::ignoreUnused(frameStart);


        {
            std::unique_lock<std::mutex> lock(eventMutex, std::try_to_lock);
            if (lock.owns_lock()) {
                for (const auto& ev : eventQueue) {
                    if (ev.type == Event::NoteOn) {
                        int idx = -1;
                        for (int i = 0; i < MAX_VOICES; ++i) {
                            if (!voices[i].active) {
                                idx = i;
                                break;
                            }
                        }
                        if (idx == -1) idx = 0;

                        auto& v = voices[idx];
                        v.active = true;
                        v.note = ev.note;
                        v.velocity = ev.velocity;
                        v.samplePosition = 0.0;
                        v.releasing = false;
                        v.envTime = 0.0;
                        v.envelope = 0.0f;
                        v.attack = attack;
                        v.decay = decay;
                        v.sustain = sustain;
                        v.release = release;

                        double pitchSpeed = std::pow(2.0, (double)(ev.note - rootNote + tune) / 12.0);
                        auto sample = std::atomic_load(&currentSample);
                        double srRatio = (sample && getSampleRate() > 0) ? (double)sample->getSampleRate() / getSampleRate() : 1.0;
                        v.speed = pitchSpeed * srRatio;

                        if (sample) {
                            uint64_t sampleLength = sample->getLengthFrames();
                            double s = std::clamp((double)start, 0.0, 0.98);
                            double e = std::clamp((double)end, 0.02, 1.0);
                            if (e <= s + 0.01) e = std::min(1.0, s + 0.02);
                            v.sampleStart = s * (double)sampleLength;
                            v.sampleEnd = std::max(v.sampleStart + 1.0, e * (double)sampleLength);
                            v.samplePosition = v.sampleStart;
                        } else {
                            v.sampleStart = 0.0;
                            v.sampleEnd = 0.0;
                        }

                    } else if (ev.type == Event::NoteOff) {
                        for (auto& v : voices) {
                            if (v.active && v.note == ev.note && !v.releasing) {
                                v.releasing = true;
                                v.releaseLevel = v.envelope;
                                v.envTime = 0.0;
                            }
                        }
                    }
                }
                eventQueue.clear();
            }
        }

        auto sample = std::atomic_load(&currentSample);
        if (!sample) {
            block.zero();
            return;
        }

        block.zero();

        size_t numSamples = block.getSampleCount();
        size_t outChannels = block.getChannelCount();

        uint64_t sampleLength = sample->getLengthFrames();
        uint32_t sampleChannels = sample->getChannels();


        for (auto& v : voices) {
            if (!v.active) continue;

            for (size_t i = 0; i < numSamples; ++i) {

                if (!v.releasing) {

                    if (v.envTime < v.attack) {
                        v.envelope = (float)(v.envTime / v.attack);
                    }

                    else if (v.envTime < v.attack + v.decay) {
                        float progress = (float)((v.envTime - v.attack) / v.decay);
                        v.envelope = 1.0f - (progress * (1.0f - v.sustain));
                    }

                    else {
                        v.envelope = v.sustain;
                    }
                } else {

                    float progress = (float)(v.envTime / v.release);
                    if (progress >= 1.0f) {
                        v.active = false;
                        v.envelope = 0.0f;






                    } else {
                        v.envelope = v.releaseLevel * (1.0f - progress);
                    }
                }

                if (!v.active) {
                    v.envTime += 1.0 / getSampleRate();
                    continue;
                }

                v.envTime += 1.0 / getSampleRate();


                double pos = v.samplePosition;
                double endPos = (v.sampleEnd > 1.0) ? v.sampleEnd : (double)sampleLength;
                endPos = std::min(endPos, (double)sampleLength);
                double startPos = std::clamp(v.sampleStart, 0.0, std::max(1.0, (double)sampleLength - 1.0));

                if (pos >= endPos - 1.0) {
                    if (loopEnabled && endPos > startPos + 1.0) {
                        pos = startPos;
                        v.samplePosition = pos;
                    } else {
                        v.active = false;
                        continue;
                    }
                }


                uint64_t idx = (uint64_t)pos;
                float frac = (float)(pos - idx);


                for (size_t c = 0; c < outChannels; ++c) {
                    float s1 = 0.0f;
                    float s2 = 0.0f;

                    uint32_t sourceCh = (c < sampleChannels) ? (uint32_t)c : 0;
                    if (sourceCh >= sampleChannels) sourceCh = 0;

                    s1 = sample->getSample(sourceCh, idx);
                if (idx + 1 < sampleLength) {
                    s2 = sample->getSample(sourceCh, idx + 1);
                }

                    float val = s1 + frac * (s2 - s1);

                    block.getChannelPointer(c)[i] += val * v.envelope * v.velocity;
                }

                v.samplePosition += v.speed;
            }
        }
    }

}
