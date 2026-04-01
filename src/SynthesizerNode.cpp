#include "orion/SynthesizerNode.h"
#include <cmath>
#include <algorithm>
#include <cstring>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Orion {

    SynthesizerNode::SynthesizerNode() {
        for (auto& v : voices) v.active = false;
        waveform = Waveform::Saw;
        filterCutoff = 20000.0f;
        filterResonance = 0.0f;
        attack = 0.01f;
        decay = 0.1f;
        sustain = 0.7f;
        release = 0.2f;
        lfoRate = 3.0f;
        lfoDepth = 0.0f;
        lfoTarget = 0;
        lfoPhase = 0.0;
    }

    float SynthesizerNode::mToF(int note) {
        return 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
    }

    void SynthesizerNode::processMidi(int status, int data1, int data2, int sampleOffset) {
        std::unique_lock<std::mutex> lock(eventMutex, std::try_to_lock);
        if (!lock.owns_lock()) return;

        if ((status & 0xF0) == 0x90) {
            if (data2 > 0)
                eventQueue.push_back({Event::NoteOn, data1, data2 / 127.0f, sampleOffset});
            else
                eventQueue.push_back({Event::NoteOff, data1, 0.0f, sampleOffset});
        } else if ((status & 0xF0) == 0x80) {
            eventQueue.push_back({Event::NoteOff, data1, 0.0f, sampleOffset});
        } else if ((status & 0xF0) == 0xB0) {
            if (data1 == 123 || data1 == 120) {
                for (auto& v : voices) v.active = false;
                eventQueue.clear();
            }
        }
    }

    void SynthesizerNode::noteOn(int note, float velocity, int sampleOffset) {
        processMidi(0x90, note, (int)(velocity * 127.0f), sampleOffset);
    }

    void SynthesizerNode::noteOff(int note, int sampleOffset) {
        processMidi(0x80, note, 0, sampleOffset);
    }

    void SynthesizerNode::allNotesOff() {
        std::unique_lock<std::mutex> lock(eventMutex, std::try_to_lock);
        if (!lock.owns_lock()) {

            for (auto& v : voices) v.active = false;
            return;
        }
        for (auto& v : voices) v.active = false;
        eventQueue.clear();
    }

    bool SynthesizerNode::getChunk(std::vector<char>& data) {
        data.resize(sizeof(waveform) + sizeof(filterCutoff) + sizeof(filterResonance)
                    + sizeof(attack) + sizeof(decay) + sizeof(sustain) + sizeof(release)
                    + sizeof(lfoRate) + sizeof(lfoDepth) + sizeof(lfoTarget));
        char* ptr = data.data();
        std::memcpy(ptr, &waveform, sizeof(waveform)); ptr += sizeof(waveform);
        std::memcpy(ptr, &filterCutoff, sizeof(filterCutoff)); ptr += sizeof(filterCutoff);
        std::memcpy(ptr, &filterResonance, sizeof(filterResonance)); ptr += sizeof(filterResonance);
        std::memcpy(ptr, &attack, sizeof(attack)); ptr += sizeof(attack);
        std::memcpy(ptr, &decay, sizeof(decay)); ptr += sizeof(decay);
        std::memcpy(ptr, &sustain, sizeof(sustain)); ptr += sizeof(sustain);
        std::memcpy(ptr, &release, sizeof(release)); ptr += sizeof(release);
        std::memcpy(ptr, &lfoRate, sizeof(lfoRate)); ptr += sizeof(lfoRate);
        std::memcpy(ptr, &lfoDepth, sizeof(lfoDepth)); ptr += sizeof(lfoDepth);
        std::memcpy(ptr, &lfoTarget, sizeof(lfoTarget)); ptr += sizeof(lfoTarget);
        return true;
    }

    bool SynthesizerNode::setChunk(const std::vector<char>& data) {
        if (data.size() < sizeof(waveform) + sizeof(filterCutoff) + sizeof(filterResonance)) return false;
        const char* ptr = data.data();
        std::memcpy(&waveform, ptr, sizeof(waveform)); ptr += sizeof(waveform);
        std::memcpy(&filterCutoff, ptr, sizeof(filterCutoff)); ptr += sizeof(filterCutoff);
        std::memcpy(&filterResonance, ptr, sizeof(filterResonance)); ptr += sizeof(filterResonance);
        if ((size_t)(ptr - data.data()) + sizeof(attack) <= data.size()) {
            std::memcpy(&attack, ptr, sizeof(attack)); ptr += sizeof(attack);
            std::memcpy(&decay, ptr, sizeof(decay)); ptr += sizeof(decay);
            std::memcpy(&sustain, ptr, sizeof(sustain)); ptr += sizeof(sustain);
            std::memcpy(&release, ptr, sizeof(release)); ptr += sizeof(release);
        }
        if ((size_t)(ptr - data.data()) + sizeof(lfoRate) <= data.size()) {
            std::memcpy(&lfoRate, ptr, sizeof(lfoRate)); ptr += sizeof(lfoRate);
            std::memcpy(&lfoDepth, ptr, sizeof(lfoDepth)); ptr += sizeof(lfoDepth);
            std::memcpy(&lfoTarget, ptr, sizeof(lfoTarget)); ptr += sizeof(lfoTarget);
        }
        return true;
    }

    void SynthesizerNode::setParameter(int index, float value) {
        switch (index) {
            case 0: waveform = (Waveform)std::clamp((int)std::round(value), 0, 3); break;
            case 1: filterCutoff = std::clamp(value, 20.0f, 20000.0f); break;
            case 2: filterResonance = std::clamp(value, 0.0f, 1.0f); break;
            case 3: attack = std::clamp(value, 0.001f, 5.0f); break;
            case 4: decay = std::clamp(value, 0.001f, 5.0f); break;
            case 5: sustain = std::clamp(value, 0.0f, 1.0f); break;
            case 6: release = std::clamp(value, 0.001f, 5.0f); break;
            case 7: lfoRate = std::clamp(value, 0.1f, 20.0f); break;
            case 8: lfoDepth = std::clamp(value, 0.0f, 1.0f); break;
            case 9: lfoTarget = std::clamp((int)std::round(value), 0, 2); break;
            default: break;
        }
    }

    float SynthesizerNode::getParameter(int index) const {
        switch (index) {
            case 0: return (float)waveform;
            case 1: return filterCutoff;
            case 2: return filterResonance;
            case 3: return attack;
            case 4: return decay;
            case 5: return sustain;
            case 6: return release;
            case 7: return lfoRate;
            case 8: return lfoDepth;
            case 9: return (float)lfoTarget;
            default: return 0.0f;
        }
    }

    std::string SynthesizerNode::getParameterName(int index) const {
        switch (index) {
            case 0: return "Waveform";
            case 1: return "Cutoff";
            case 2: return "Resonance";
            case 3: return "Attack";
            case 4: return "Decay";
            case 5: return "Sustain";
            case 6: return "Release";
            case 7: return "LFO Rate";
            case 8: return "LFO Depth";
            case 9: return "LFO Target";
            default: return "";
        }
    }

    ParameterRange SynthesizerNode::getParameterRange(int index) const {
        switch (index) {
            case 0: return {0.0f, 3.0f, 1.0f};
            case 1: return {20.0f, 20000.0f, 1.0f};
            case 2: return {0.0f, 1.0f, 0.01f};
            case 3: return {0.001f, 5.0f, 0.001f};
            case 4: return {0.001f, 5.0f, 0.001f};
            case 5: return {0.0f, 1.0f, 0.01f};
            case 6: return {0.001f, 5.0f, 0.001f};
            case 7: return {0.1f, 20.0f, 0.01f};
            case 8: return {0.0f, 1.0f, 0.01f};
            case 9: return {0.0f, 2.0f, 1.0f};
            default: return EffectNode::getParameterRange(index);
        }
    }

    void SynthesizerNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;
        size_t numSamples = block.getSampleCount();
        block.zero();

        float* outL = block.getChannelPointer(0);
        float* outR = (block.getChannelCount() > 1) ? block.getChannelPointer(1) : outL;


        double fs = getSampleRate();
        if (fs <= 0) fs = 44100.0;



        double baseFc = std::clamp((double)filterCutoff, 20.0, fs * 0.49);
        double q = 0.707 + (filterResonance * 4.0);

        auto computeCoeffs = [&](double fc, double& nb0, double& nb1, double& nb2, double& na1, double& na2) {
            double w0 = 2 * M_PI * fc / fs;
            double cosw0 = std::cos(w0);
            double sinw0 = std::sin(w0);
            double alpha = sinw0 / (2.0 * q);

            double b0 =  (1 - cosw0) / 2;
            double b1 =   1 - cosw0;
            double b2 =  (1 - cosw0) / 2;
            double a0 =   1 + alpha;
            double a1 =  -2 * cosw0;
            double a2 =   1 - alpha;

            nb0 = b0/a0; nb1 = b1/a0; nb2 = b2/a0; na1 = a1/a0; na2 = a2/a0;
        };

        double baseNb0 = 0, baseNb1 = 0, baseNb2 = 0, baseNa1 = 0, baseNa2 = 0;
        computeCoeffs(baseFc, baseNb0, baseNb1, baseNb2, baseNa1, baseNa2);

        std::vector<Event> localEvents;
        {
            std::unique_lock<std::mutex> lock(eventMutex, std::try_to_lock);
            if (lock.owns_lock()) {
                localEvents.swap(eventQueue);
            }
        }

        std::sort(localEvents.begin(), localEvents.end(), [](const Event& a, const Event& b) {
            if (a.offset != b.offset) return a.offset < b.offset;
            return a.type > b.type;
        });


        size_t currentSample = 0;
        size_t eventIdx = 0;


        auto renderChunk = [&](size_t start, size_t count) {
            double dt = 1.0 / fs;
            const double lfoInc = (lfoRate > 0.0f) ? (2.0 * M_PI * (double)lfoRate / fs) : 0.0;
            double phase = lfoPhase;

            for (size_t i = 0; i < count; ++i) {
                double lfoValue = 0.0;
                if (lfoTarget != 0 && lfoDepth > 0.0001f) {
                    lfoValue = std::sin(phase);
                    phase += lfoInc;
                    if (phase > 2.0 * M_PI) phase -= 2.0 * M_PI;
                }

                double nb0 = baseNb0, nb1 = baseNb1, nb2 = baseNb2, na1 = baseNa1, na2 = baseNa2;
                if (lfoTarget == 2 && lfoDepth > 0.0001f) {
                    double mod = 1.0 + (double)lfoDepth * 0.5 * lfoValue;
                    double fc = std::clamp(baseFc * mod, 20.0, fs * 0.49);
                    computeCoeffs(fc, nb0, nb1, nb2, na1, na2);
                }

                for (auto& v : voices) {
                    if (!v.active) continue;

                    float freq = mToF(v.note);
                    if (lfoTarget == 1 && lfoDepth > 0.0001f) {
                        const double depthSemis = (double)lfoDepth * 2.0;
                        const double semis = depthSemis * lfoValue;
                        freq = (float)(freq * std::pow(2.0, semis / 12.0));
                    }

                    float velocityScale = v.velocity * 0.2f;

                    float osc = 0.0f;


                    switch(waveform) {
                        case Waveform::Sine:
                            osc = (float)std::sin(v.phase * 2.0 * M_PI);
                            break;
                        case Waveform::Saw:
                            osc = (float)(v.phase * 2.0 - 1.0);
                            break;
                        case Waveform::Square:
                            osc = (v.phase < 0.5) ? 1.0f : -1.0f;
                            break;
                        case Waveform::Triangle:
                            osc = (float)(std::abs(v.phase * 2.0 - 1.0) * 2.0 - 1.0);
                            break;
                    }

                    v.phase += freq * dt;
                    v.phase -= std::floor(v.phase);


                    float env = 0.0f;
                    if (!v.releasing) {
                        v.envTime += dt;
                        if (v.envTime < v.attack) {
                            env = (float)(v.envTime / v.attack);
                        } else if (v.envTime < v.attack + v.decay) {
                            float rel = (float)((v.envTime - v.attack) / v.decay);
                            env = 1.0f - rel * (1.0f - v.sustain);
                        } else {
                            env = v.sustain;
                        }
                    } else {
                        v.envTime += dt;
                        if (v.envTime < v.release) {
                            float rel = (float)(v.envTime / v.release);
                            env = v.sustain * (1.0f - rel);
                        } else {
                            v.active = false;
                            env = 0.0f;
                        }
                    }

                    float voiceSample = osc * env * velocityScale;


                    double in = (double)voiceSample;
                    double out = nb0*in + nb1*v.filterX1 + nb2*v.filterX2 - na1*v.filterY1 - na2*v.filterY2;


                    if (std::abs(out) < 1e-10) out = 0.0;

                    v.filterX2 = v.filterX1;
                    v.filterX1 = in;
                    v.filterY2 = v.filterY1;
                    v.filterY1 = out;

                    outL[start + i] += (float)out;
                    if (outR != outL) outR[start + i] += (float)out;
                }
            }

            lfoPhase = phase;
        };

        while (currentSample < numSamples) {
            size_t nextEventSample = numSamples;
            if (eventIdx < localEvents.size()) {
                nextEventSample = (size_t)localEvents[eventIdx].offset;
                if (nextEventSample < currentSample) nextEventSample = currentSample;
                if (nextEventSample > numSamples) nextEventSample = numSamples;
            }

            if (nextEventSample > currentSample) {
                renderChunk(currentSample, nextEventSample - currentSample);
                currentSample = nextEventSample;
            }

            while (eventIdx < localEvents.size() && (size_t)localEvents[eventIdx].offset == currentSample) {
                const auto& evt = localEvents[eventIdx];
                if (evt.type == Event::NoteOn) {

                    int voiceIdx = -1;


                    for (int v = 0; v < MAX_VOICES; ++v) {
                        if (!voices[v].active) {
                            voiceIdx = v;
                            break;
                        }
                    }


                    if (voiceIdx == -1) {
                        for (int v = 0; v < MAX_VOICES; ++v) {
                            if (voices[v].releasing) {
                                voiceIdx = v;
                                break;
                            }
                        }
                    }


                    if (voiceIdx == -1) {
                        double maxTime = -1.0;
                        for (int v = 0; v < MAX_VOICES; ++v) {
                            if (voices[v].envTime > maxTime) {
                                maxTime = voices[v].envTime;
                                voiceIdx = v;
                            }
                        }
                    }

                    if (voiceIdx == -1) voiceIdx = 0;

                    voices[voiceIdx].active = true;
                    voices[voiceIdx].note = evt.note;
                    voices[voiceIdx].velocity = evt.velocity;
                    voices[voiceIdx].phase = 0.0;
                    voices[voiceIdx].envelope = 0.0f;
                    voices[voiceIdx].releasing = false;
                    voices[voiceIdx].envTime = 0.0;
                    voices[voiceIdx].attack = attack;
                    voices[voiceIdx].decay = decay;
                    voices[voiceIdx].sustain = sustain;
                    voices[voiceIdx].release = release;

                    voices[voiceIdx].filterX1 = 0; voices[voiceIdx].filterX2 = 0;
                    voices[voiceIdx].filterY1 = 0; voices[voiceIdx].filterY2 = 0;
                }
                else if (evt.type == Event::NoteOff) {
                    for (auto& v : voices) {
                        if (v.active && v.note == evt.note && !v.releasing) {
                            v.releasing = true;
                            v.envTime = 0.0;
                        }
                    }
                }
                eventIdx++;
            }
        }
    }

}
