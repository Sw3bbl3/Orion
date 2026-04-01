#pragma once

#include <vector>
#include <cstddef>
#include <algorithm>

namespace Orion {


    class AudioBlock {
    public:
        static const size_t DefaultBlockSize = 128;
        static const size_t DefaultChannels = 2;

        AudioBlock(size_t channels = DefaultChannels, size_t samplesPerChannel = DefaultBlockSize);


        void resize(size_t channels, size_t samplesPerChannel);


        void zero();


        float* getChannelPointer(size_t channel);
        const float* getChannelPointer(size_t channel) const;

        size_t getChannelCount() const { return channels; }
        size_t getSampleCount() const { return samplesPerChannel; }


        void copyFrom(const AudioBlock& other);
        void add(const AudioBlock& other);
        void multiply(float gain);

    private:
        size_t channels;
        size_t samplesPerChannel;


        std::vector<std::vector<float>> data;
    };

}
