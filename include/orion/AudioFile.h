#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace Orion {

    class AudioFile {
    public:
        AudioFile();
        ~AudioFile();



        bool load(const std::string& filePath, uint32_t targetSampleRate = 0);

        const std::string& getFilePath() const { return filePath; }
        const std::string& getFileName() const { return fileName; }

        uint32_t getChannels() const { return channels; }
        uint32_t getSampleRate() const { return sampleRate; }
        uint64_t getLengthFrames() const { return lengthFrames; }

        float getBpm() const { return bpm; }


        float getSample(uint32_t channel, uint64_t frame) const;


        const float* getChannelData(uint32_t channel) const;


        const std::vector<std::vector<float>>& getData() const { return audioData; }

    private:
        std::string filePath;
        std::string fileName;

        uint32_t channels;
        uint32_t sampleRate;
        uint64_t lengthFrames;

        float bpm = 0.0f;

        std::vector<std::vector<float>> audioData;
    };

}
