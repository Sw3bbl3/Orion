#include "orion/WavWriter.h"
#include <algorithm>
#include <iostream>

namespace Orion {

    WavWriter::WavWriter(const std::string& filename, int sampleRate, int channels)
        : filename(filename), sampleRate(sampleRate), channels(channels), dataSize(0)
    {
        file.open(filename, std::ios::binary);
        if (file.is_open()) {
            writeHeader();
        } else {
            std::cerr << "Failed to open file for writing: " << filename << std::endl;
        }
    }

    WavWriter::~WavWriter() {
        if (file.is_open()) {
            updateHeader();
            file.close();
        }
    }

    void WavWriter::write(const float* buffer, size_t numFrames) {
        if (!file.is_open()) return;

        std::vector<int16_t> intBuffer(numFrames * channels);
        for (size_t i = 0; i < numFrames * channels; ++i) {
            float sample = buffer[i];

            if (sample > 1.0f) sample = 1.0f;
            if (sample < -1.0f) sample = -1.0f;
            intBuffer[i] = static_cast<int16_t>(sample * 32767.0f);
        }

        file.write(reinterpret_cast<const char*>(intBuffer.data()), intBuffer.size() * sizeof(int16_t));
        dataSize += static_cast<uint32_t>(intBuffer.size() * sizeof(int16_t));
    }

    void WavWriter::writeHeader() {


        char header[44] = {0};
        file.write(header, 44);
    }

    void WavWriter::updateHeader() {
        file.seekp(0, std::ios::beg);

        uint32_t fileSize = 36 + dataSize;
        uint32_t byteRate = sampleRate * channels * 2;
        uint16_t blockAlign = static_cast<uint16_t>(channels * 2);
        uint16_t bitsPerSample = 16;


        file.write("RIFF", 4);
        file.write(reinterpret_cast<const char*>(&fileSize), 4);
        file.write("WAVE", 4);


        file.write("fmt ", 4);
        uint32_t fmtChunkSize = 16;
        file.write(reinterpret_cast<const char*>(&fmtChunkSize), 4);
        uint16_t audioFormat = 1;
        file.write(reinterpret_cast<const char*>(&audioFormat), 2);
        uint16_t numChannels = static_cast<uint16_t>(channels);
        file.write(reinterpret_cast<const char*>(&numChannels), 2);
        uint32_t sRate = static_cast<uint32_t>(sampleRate);
        file.write(reinterpret_cast<const char*>(&sRate), 4);
        file.write(reinterpret_cast<const char*>(&byteRate), 4);
        file.write(reinterpret_cast<const char*>(&blockAlign), 2);
        file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);


        file.write("data", 4);
        file.write(reinterpret_cast<const char*>(&dataSize), 4);
    }

}
