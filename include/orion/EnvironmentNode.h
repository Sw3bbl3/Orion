#pragma once

#include "AudioNode.h"
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>

namespace Orion {

    class EnvironmentNode : public AudioNode {
    public:
        enum class Preset {
            Car,
            Club,
            Outdoor,
            Hall,
            SmallRoom,
            Studio,
            Phone
        };

        EnvironmentNode();
        ~EnvironmentNode() override = default;


        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void setFormat(size_t numChannels, size_t numBlockSize) override;


        void setPreset(Preset p);
        Preset getPreset() const { return currentPreset; }


        void setSize(float size);
        float getSize() const { return size.load(); }

        void setCrowd(float crowd);
        float getCrowd() const { return crowd.load(); }

        void setAmbience(float ambience);
        float getAmbience() const { return ambience.load(); }

        void setIntensity(float intensity);
        float getIntensity() const { return intensity.load(); }

        void setTone(float t);
        float getTone() const { return tone.load(); }

        void setWidth(float w);
        float getWidth() const { return width.load(); }

        void setGrit(float g);
        float getGrit() const { return grit.load(); }

        void setPreDelay(float p);
        float getPreDelay() const { return preDelay.load(); }

        void setOutputTrim(float t);
        float getOutputTrim() const { return outputTrim.load(); }

        void setAutoGain(bool b) { autoGain = b; }
        bool getAutoGain() const { return autoGain.load(); }

        void setDamping(float d) { setCrowd(d); }
        float getDamping() const { return getCrowd(); }
        void setMix(float m) { setAmbience(m); }
        float getMix() const { return getAmbience(); }

        void setActive(bool b) { active = b; }
        bool isActive() const { return active.load(); }

    private:

        juce::dsp::StateVariableTPTFilter<float> highPassFilter;
        juce::dsp::StateVariableTPTFilter<float> lowPassFilter;
        juce::dsp::WaveShaper<float> waveShaper;
        juce::dsp::Compressor<float> compressor;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
        juce::Reverb reverb;
        juce::dsp::Limiter<float> limiter;
        juce::dsp::StateVariableTPTFilter<float> presetHighPassFilter;
        juce::dsp::StateVariableTPTFilter<float> presetLowPassFilter;

        std::atomic<bool> active { false };
        juce::Reverb::Parameters reverbParams;

        std::atomic<float> size { 0.5f };
        std::atomic<float> crowd { 0.5f };
        std::atomic<float> ambience { 0.3f };
        std::atomic<float> intensity { 0.5f };

        std::atomic<float> tone { 0.5f };
        std::atomic<float> width { 0.5f };
        std::atomic<float> grit { 0.0f };
        std::atomic<float> preDelay { 0.15f };
        std::atomic<float> outputTrim { 0.5f };
        std::atomic<bool> autoGain { true };
        std::atomic<float> presetCrossfeed { 0.0f };
        std::atomic<float> presetMonoAmount { 0.0f };
        std::atomic<float> presetSaturation { 0.0f };

        Preset currentPreset = Preset::Studio;

        void updateDSP();
        void applyPreset(Preset p);
    };

}
