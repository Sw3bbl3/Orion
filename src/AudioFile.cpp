#include "orion/AudioFile.h"
#include "orion/AudioEngine.h"
#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <filesystem>
#include <iostream>
#include <regex>

namespace Orion {

    AudioFile::AudioFile()
        : channels(0), sampleRate(0), lengthFrames(0), bpm(0.0f)
    {
    }

    AudioFile::~AudioFile() {
    }

    bool AudioFile::load(const std::string& path, uint32_t targetSampleRate) {
        gDiskActivityCounter++;
        juce::ignoreUnused(targetSampleRate);
        filePath = path;
        auto p = std::filesystem::path(path);
        fileName = p.filename().string();

        juce::File file(path);
        if (!file.existsAsFile()) return false;

        juce::AudioFormatManager formatManager;
        formatManager.registerBasicFormats();

        #if JUCE_WINDOWS
        formatManager.registerFormat(new juce::WindowsMediaAudioFormat(), false);
        #endif

        #if JUCE_MAC
        formatManager.registerFormat(new juce::CoreAudioFormat(), false);
        #endif

        std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
        if (reader == nullptr) return false;

        channels = reader->numChannels;
        sampleRate = (uint32_t)reader->sampleRate;
        lengthFrames = (uint64_t)reader->lengthInSamples;


        bpm = 0.0f;
        auto metadata = reader->metadataValues;


        if (metadata.containsKey("Tempo")) {
            bpm = metadata["Tempo"].getFloatValue();
        } else if (metadata.containsKey("BPM")) {
            bpm = metadata["BPM"].getFloatValue();
        } else if (metadata.containsKey("Acid Chunk: Tempo")) {
            bpm = metadata["Acid Chunk: Tempo"].getFloatValue();
        } else if (metadata.containsKey("id3:TBPM")) {
            bpm = metadata["id3:TBPM"].getFloatValue();
        }


        if (bpm <= 0.0f) {
            try {
                std::string name = p.stem().string();



                std::regex bpmSuffixRegex(R"((\d+(?:\.\d+)?)\s*bpm)", std::regex_constants::icase);
                std::smatch match;
                if (std::regex_search(name, match, bpmSuffixRegex)) {
                    if (match.size() > 1) {
                        bpm = std::stof(match[1].str());
                    }
                }


                if (bpm <= 0.0f) {
                    std::regex bpmPrefixRegex(R"(bpm\s*(\d+(?:\.\d+)?))", std::regex_constants::icase);
                    if (std::regex_search(name, match, bpmPrefixRegex)) {
                        if (match.size() > 1) {
                            bpm = std::stof(match[1].str());
                        }
                    }
                }
            } catch (...) {

            }
        }


        if (bpm < 20.0f || bpm > 999.0f) bpm = 0.0f;




        audioData.resize(channels);
        for (auto& ch : audioData) {
            ch.resize(lengthFrames);
        }


        juce::AudioBuffer<float> buffer((int)channels, (int)lengthFrames);
        reader->read(&buffer, 0, (int)lengthFrames, 0, true, true);


        for (int ch = 0; ch < (int)channels; ++ch) {
            auto* readPtr = buffer.getReadPointer(ch);
            std::copy(readPtr, readPtr + lengthFrames, audioData[ch].begin());
        }

        return true;
    }

    float AudioFile::getSample(uint32_t channel, uint64_t frame) const {
        if (channel >= channels || frame >= lengthFrames) return 0.0f;
        return audioData[channel][frame];
    }

    const float* AudioFile::getChannelData(uint32_t channel) const {
        if (channel >= channels) return nullptr;
        return audioData[channel].data();
    }

}
