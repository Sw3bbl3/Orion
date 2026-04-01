#pragma once
#include <string>
#include <fstream>
#include <vector>
#include <cstdint>

namespace Orion {

    class WavWriter {
    public:
        WavWriter(const std::string& filename, int sampleRate, int channels);
        ~WavWriter();


        void write(const float* buffer, size_t numFrames);

    private:
        void writeHeader();
        void updateHeader();

        std::string filename;
        std::ofstream file;
        int sampleRate;
        int channels;
        uint32_t dataSize;
    };

}
