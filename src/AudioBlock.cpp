#include "orion/AudioBlock.h"
#include <cstring>

namespace Orion {

    AudioBlock::AudioBlock(size_t channels, size_t samplesPerChannel) {
        resize(channels, samplesPerChannel);
    }

    void AudioBlock::resize(size_t numChannels, size_t numSamples) {
        this->channels = numChannels;
        this->samplesPerChannel = numSamples;
        data.resize(numChannels);
        for (auto& channel : data) {
            channel.resize(numSamples, 0.0f);
        }
    }

    void AudioBlock::zero() {
        for (auto& channel : data) {
            std::fill(channel.begin(), channel.end(), 0.0f);
        }
    }

    float* AudioBlock::getChannelPointer(size_t channel) {
        if (channel < data.size()) {
            return data[channel].data();
        }
        return nullptr;
    }

    const float* AudioBlock::getChannelPointer(size_t channel) const {
        if (channel < data.size()) {
            return data[channel].data();
        }
        return nullptr;
    }

    void AudioBlock::copyFrom(const AudioBlock& other) {
        if (other.channels != channels || other.samplesPerChannel != samplesPerChannel) {

            return;
        }
        for (size_t c = 0; c < channels; ++c) {
            std::copy(other.data[c].begin(), other.data[c].end(), data[c].begin());
        }
    }

    void AudioBlock::add(const AudioBlock& other) {

        size_t minChannels = std::min(channels, other.channels);
        size_t minSamples = std::min(samplesPerChannel, other.samplesPerChannel);

        for (size_t c = 0; c < minChannels; ++c) {
            const float* src = other.getChannelPointer(c);
            float* dst = getChannelPointer(c);
            for (size_t s = 0; s < minSamples; ++s) {
                dst[s] += src[s];
            }
        }
    }

    void AudioBlock::multiply(float gain) {
        for (auto& channel : data) {
            for (auto& sample : channel) {
                sample *= gain;
            }
        }
    }

}
