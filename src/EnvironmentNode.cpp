#include "orion/EnvironmentNode.h"
#include <algorithm>
#include <cmath>

namespace Orion {

    EnvironmentNode::EnvironmentNode() {

        applyPreset(Preset::Studio);
    }

    void EnvironmentNode::setFormat(size_t numChannels, size_t numBlockSize) {
        AudioNode::setFormat(numChannels, numBlockSize);

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = (double)sampleRate.load();
        spec.maximumBlockSize = (juce::uint32)numBlockSize;
        spec.numChannels = (juce::uint32)numChannels;

        if (spec.sampleRate > 0) {

            highPassFilter.prepare(spec);
            highPassFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);

            lowPassFilter.prepare(spec);
            lowPassFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);


            waveShaper.functionToUse = [](float x) {
                return std::tanh(x);
            };

            compressor.prepare(spec);


            delayLine.setMaximumDelayInSamples((int)(spec.sampleRate * 2.0));
            delayLine.prepare(spec);

            reverb.setSampleRate(spec.sampleRate);
            reverb.reset();

            limiter.prepare(spec);

            presetHighPassFilter.prepare(spec);
            presetHighPassFilter.setType(juce::dsp::StateVariableTPTFilterType::highpass);
            presetLowPassFilter.prepare(spec);
            presetLowPassFilter.setType(juce::dsp::StateVariableTPTFilterType::lowpass);
        }

        updateDSP();
    }

    void EnvironmentNode::setPreset(Preset p) {
        currentPreset = p;
        applyPreset(p);
        updateDSP();
    }

    void EnvironmentNode::applyPreset(Preset p) {

        float s = 0.5f, c = 0.5f, a = 0.3f, i = 0.5f;
        float t = 0.5f;
        float w = 0.5f;
        float g = 0.0f;
        float pd = 0.15f;
        float o = 0.5f;

        switch (p) {
            case Preset::Car:
                s = 0.1f;
                c = 0.75f;
                a = 0.14f;
                i = 0.72f;
                t = 0.38f;
                w = 0.34f;
                g = 0.13f;
                pd = 0.09f;
                o = 0.52f;
                break;
            case Preset::Club:
                s = 0.78f;
                c = 0.82f;
                a = 0.34f;
                i = 0.88f;
                t = 0.43f;
                w = 0.9f;
                g = 0.28f;
                pd = 0.22f;
                o = 0.45f;
                break;
            case Preset::Outdoor:
                s = 0.95f;
                c = 0.15f;
                a = 0.09f;
                i = 0.22f;
                t = 0.6f;
                w = 1.0f;
                g = 0.0f;
                pd = 0.3f;
                o = 0.5f;
                break;
            case Preset::Hall:
                s = 0.8f;
                c = 0.4f;
                a = 0.47f;
                i = 0.42f;
                t = 0.55f;
                w = 0.9f;
                g = 0.05f;
                pd = 0.4f;
                o = 0.45f;
                break;
            case Preset::SmallRoom:
                s = 0.3f;
                c = 0.5f;
                a = 0.25f;
                i = 0.5f;
                t = 0.5f;
                w = 0.5f;
                g = 0.0f;
                pd = 0.2f;
                o = 0.5f;
                break;
            case Preset::Phone:
                s = 0.05f;
                c = 0.0f;
                a = 0.04f;
                i = 0.94f;
                t = 0.47f;
                w = 0.0f;
                g = 0.4f;
                pd = 0.01f;
                o = 0.58f;
                break;
            case Preset::Studio:
            default:
                s = 0.2f;
                c = 0.7f;
                a = 0.15f;
                i = 0.6f;
                t = 0.5f;
                w = 0.5f;
                g = 0.1f;
                pd = 0.18f;
                o = 0.5f;
                break;
        }

        size = s;
        crowd = c;
        ambience = a;
        intensity = i;
        tone = t;
        width = w;
        grit = g;
        preDelay = pd;
        outputTrim = o;
    }

    void EnvironmentNode::setSize(float s) { size = std::clamp(s, 0.0f, 1.0f); }
    void EnvironmentNode::setCrowd(float c) { crowd = std::clamp(c, 0.0f, 1.0f); }
    void EnvironmentNode::setAmbience(float a) { ambience = std::clamp(a, 0.0f, 1.0f); }
    void EnvironmentNode::setIntensity(float i) { intensity = std::clamp(i, 0.0f, 1.0f); }
    void EnvironmentNode::setTone(float t) { tone = std::clamp(t, 0.0f, 1.0f); }
    void EnvironmentNode::setWidth(float w) { width = std::clamp(w, 0.0f, 1.0f); }
    void EnvironmentNode::setGrit(float g) { grit = std::clamp(g, 0.0f, 1.0f); }
    void EnvironmentNode::setPreDelay(float p) { preDelay = std::clamp(p, 0.0f, 1.0f); }
    void EnvironmentNode::setOutputTrim(float t) { outputTrim = std::clamp(t, 0.0f, 1.0f); }

    void EnvironmentNode::updateDSP() {
        float s = size.load();
        float c = crowd.load();
        float a = ambience.load();
        float i = intensity.load();
        float t = tone.load();

        float minLP = 800.0f;
        float maxLP = 20000.0f;
        float minHP = 20.0f;
        float maxHP = 2000.0f;

        if (t < 0.5f) {
            float factor = t * 2.0f;
            float lpf = minLP + (maxLP - minLP) * factor;
            lowPassFilter.setCutoffFrequency(lpf);
            highPassFilter.setCutoffFrequency(minHP);
        } else {
            float factor = (t - 0.5f) * 2.0f;
            float hpf = minHP + (maxHP - minHP) * factor;
            lowPassFilter.setCutoffFrequency(maxLP);
            highPassFilter.setCutoffFrequency(hpf);
        }



        compressor.setRatio(1.0f + (c * 8.0f) + (i * 4.0f));
        compressor.setThreshold(-10.0f - (i * 30.0f));
        compressor.setAttack(10.0f + (1.0f - i) * 50.0f);
        compressor.setRelease(100.0f + c * 200.0f);

        float presetHp = 30.0f;
        float presetLp = 18000.0f;
        float presetRoomMul = 1.0f;
        float presetWetMul = 1.0f;
        float presetDryMul = 1.0f;
        float presetDampBias = 0.0f;
        float presetCf = 0.0f;
        float presetMono = 0.0f;
        float presetSat = 0.0f;

        switch (currentPreset) {
            case Preset::Car:
                presetHp = 70.0f;
                presetLp = 9000.0f;
                presetRoomMul = 0.7f;
                presetWetMul = 0.6f;
                presetDryMul = 0.92f;
                presetDampBias = 0.12f;
                presetCf = 0.11f;
                presetMono = 0.18f;
                presetSat = 0.15f;
                break;
            case Preset::Phone:
                presetHp = 280.0f;
                presetLp = 4300.0f;
                presetRoomMul = 0.4f;
                presetWetMul = 0.25f;
                presetDryMul = 0.86f;
                presetDampBias = 0.20f;
                presetCf = 0.0f;
                presetMono = 1.0f;
                presetSat = 0.28f;
                break;
            case Preset::Club:
                presetHp = 28.0f;
                presetLp = 15500.0f;
                presetRoomMul = 1.18f;
                presetWetMul = 1.05f;
                presetDryMul = 0.9f;
                presetDampBias = -0.05f;
                presetCf = 0.03f;
                presetMono = 0.05f;
                presetSat = 0.2f;
                break;
            case Preset::Outdoor:
                presetHp = 95.0f;
                presetLp = 17000.0f;
                presetRoomMul = 0.45f;
                presetWetMul = 0.35f;
                presetDryMul = 1.0f;
                presetDampBias = -0.1f;
                presetCf = 0.0f;
                presetMono = 0.0f;
                presetSat = 0.04f;
                break;
            case Preset::Hall:
                presetHp = 45.0f;
                presetLp = 14500.0f;
                presetRoomMul = 1.25f;
                presetWetMul = 1.2f;
                presetDryMul = 0.88f;
                presetDampBias = 0.06f;
                presetCf = 0.04f;
                presetMono = 0.08f;
                presetSat = 0.08f;
                break;
            case Preset::SmallRoom:
                presetHp = 65.0f;
                presetLp = 12000.0f;
                presetRoomMul = 0.75f;
                presetWetMul = 0.78f;
                presetDryMul = 0.95f;
                presetDampBias = 0.1f;
                presetCf = 0.07f;
                presetMono = 0.12f;
                presetSat = 0.1f;
                break;
            case Preset::Studio:
            default:
                presetHp = 35.0f;
                presetLp = 19000.0f;
                presetRoomMul = 0.85f;
                presetWetMul = 0.8f;
                presetDryMul = 1.0f;
                presetDampBias = 0.0f;
                presetCf = 0.03f;
                presetMono = 0.04f;
                presetSat = 0.06f;
                break;
        }

        presetHighPassFilter.setCutoffFrequency(presetHp);
        presetHighPassFilter.setResonance(0.72f);
        presetLowPassFilter.setCutoffFrequency(presetLp);
        presetLowPassFilter.setResonance(0.7f);
        presetCrossfeed = presetCf;
        presetMonoAmount = presetMono;
        presetSaturation = presetSat;


        float pred = preDelay.load();
        float delayMs = 2.0f + (pred * pred * 250.0f) + (s * s * 380.0f);
        if (getSampleRate() > 0) {
            delayLine.setDelay((float)(delayMs * getSampleRate() / 1000.0));
        }


        reverbParams.roomSize = juce::jlimit(0.0f, 1.0f, (s * 0.95f) * presetRoomMul);
        reverbParams.damping = juce::jlimit(0.0f, 1.0f, c + presetDampBias);
        reverbParams.wetLevel = juce::jlimit(0.0f, 1.0f, a * presetWetMul);
        reverbParams.dryLevel = juce::jlimit(0.0f, 1.0f, (1.0f - (a * 0.5f)) * presetDryMul);
        reverbParams.width = (currentPreset == Preset::Phone) ? 0.0f : 1.0f;
        reverb.setParameters(reverbParams);


        limiter.setThreshold(-0.5f - (i * 2.0f));
        limiter.setRelease(100.0f);
    }

    void EnvironmentNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        juce::ignoreUnused(frameStart);

        if (getSampleRate() <= 0) return;
        if (!active.load()) return;
        if (block.getChannelCount() < 2) return;


        updateDSP();



        float* channelPtrs[] = { block.getChannelPointer(0), block.getChannelPointer(1) };
        juce::dsp::AudioBlock<float> dspBlock(channelPtrs, 2, (size_t)block.getSampleCount());
        juce::dsp::ProcessContextReplacing<float> context(dspBlock);




        highPassFilter.process(context);
        lowPassFilter.process(context);




        float currentGrit = grit.load();
        if (currentGrit > 0.01f) {
            float drive = 1.0f + (currentGrit * 5.0f);


             for (size_t ch = 0; ch < context.getOutputBlock().getNumChannels(); ++ch) {
                float* data = context.getOutputBlock().getChannelPointer(ch);
                for (size_t i = 0; i < context.getOutputBlock().getNumSamples(); ++i) {
                    data[i] = std::tanh(data[i] * drive);
                }
            }
        }


        compressor.process(context);



        {
            auto& inputBlock = context.getOutputBlock();
            float currentAmbience = ambience.load();
            float feedback = 0.3f * currentAmbience;

            for (size_t ch = 0; ch < inputBlock.getNumChannels(); ++ch) {
                float* data = inputBlock.getChannelPointer(ch);
                for (size_t i = 0; i < inputBlock.getNumSamples(); ++i) {
                    float dry = data[i];






                    float delayedSample = delayLine.popSample((int)ch);
                    delayLine.pushSample((int)ch, dry + (delayedSample * feedback));



                    float delayMix = currentAmbience * 0.4f;
                    data[i] = dry + (delayedSample * delayMix);
                }
            }
        }


        reverb.processStereo(channelPtrs[0], channelPtrs[1], (int)block.getSampleCount());

        presetHighPassFilter.process(context);
        presetLowPassFilter.process(context);

        const float monoAmt = presetMonoAmount.load();
        const float crossfeed = presetCrossfeed.load();
        const float sat = presetSaturation.load();
        if (monoAmt > 0.001f || crossfeed > 0.001f || sat > 0.001f) {
            const float drive = 1.0f + sat * 3.2f;
            for (size_t i = 0; i < context.getOutputBlock().getNumSamples(); ++i) {
                float l = channelPtrs[0][i];
                float r = channelPtrs[1][i];

                const float m = (l + r) * 0.5f;
                l = l * (1.0f - monoAmt) + m * monoAmt;
                r = r * (1.0f - monoAmt) + m * monoAmt;

                const float l0 = l;
                const float r0 = r;
                l = l0 * (1.0f - crossfeed) + r0 * crossfeed;
                r = r0 * (1.0f - crossfeed) + l0 * crossfeed;

                if (sat > 0.001f) {
                    l = std::tanh(l * drive);
                    r = std::tanh(r * drive);
                }

                channelPtrs[0][i] = l;
                channelPtrs[1][i] = r;
            }
        }


        float currentWidth = width.load();









        if (std::abs(currentWidth - 0.5f) > 0.001f) {
            float widthFactor = currentWidth * 2.0f;

            for (size_t i = 0; i < context.getOutputBlock().getNumSamples(); ++i) {
                float l = channelPtrs[0][i];
                float r = channelPtrs[1][i];

                float m = (l + r) * 0.5f;
                float s = (l - r) * 0.5f;

                s *= widthFactor;

                channelPtrs[0][i] = m + s;
                channelPtrs[1][i] = m - s;
            }
        }


        float trimNorm = outputTrim.load();
        float trimDb = (trimNorm * 24.0f) - 12.0f;
        float gain = juce::Decibels::decibelsToGain(trimDb);

        if (autoGain.load()) {
            const float s = size.load();
            const float a = ambience.load();
            const float i = intensity.load();
            const float g = grit.load();
            const float matchDb = -(a * 3.5f + s * 1.2f + i * 1.3f + g * 2.2f);
            gain *= juce::Decibels::decibelsToGain(matchDb);
        }

        if (std::abs(gain - 1.0f) > 0.001f) {
            for (size_t ch = 0; ch < context.getOutputBlock().getNumChannels(); ++ch) {
                float* data = context.getOutputBlock().getChannelPointer(ch);
                for (size_t i = 0; i < context.getOutputBlock().getNumSamples(); ++i) {
                    data[i] *= gain;
                }
            }
        }

        limiter.process(context);
    }
}
