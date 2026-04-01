#pragma once

#include "AudioNode.h"
#include <string>
#include <functional>
#include <juce_audio_basics/juce_audio_basics.h>

namespace Orion {

    struct ParameterRange {
        float minVal = 0.0f;
        float maxVal = 1.0f;
        float step = 0.01f;
    };

    class EffectNode : public AudioNode {
    public:
        EffectNode() = default;
        virtual ~EffectNode() = default;

        virtual std::string getName() const = 0;


        virtual void setParameter(int index, float value) { (void)index; (void)value; }
        virtual float getParameter(int index) const { (void)index; return 0.0f; }
        virtual std::string getParameterName(int index) const { (void)index; return ""; }
        virtual int getParameterCount() const { return 0; }


        virtual ParameterRange getParameterRange(int index) const {
            (void)index;
            return {0.0f, 2.0f, 0.01f};
        }


        virtual bool isExternal() const { return false; }
        virtual void openEditor(void* parentWindow) { (void)parentWindow; }
        virtual void closeEditor() {}
        virtual void idle() {}
        virtual bool getEditorSize(int& width, int& height) const { (void)width; (void)height; return false; }

        std::function<void(int, int)> onSizeChanged;

        void setBypass(bool b) { bypass = b; }
        bool isBypassed() const { return bypass; }


        virtual bool getChunk(std::vector<char>& data) { (void)data; return false; }
        virtual bool setChunk(const std::vector<char>& data) { (void)data; return false; }


        virtual bool isInstrument() const { return false; }


        void processAudio(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
            if (!bypass) process(block, cacheKey, frameStart);
        }


        void processMidi(int status, int data1, int data2, int sampleOffset = 0) override {
            if (!bypass) AudioNode::processMidi(status, data1, data2, sampleOffset);
        }

        // New overload for MIDI buffer processing (for internal nodes that handle blocks)
        virtual void processMidi(juce::MidiBuffer& midi, uint64_t frameCount) {
            (void)midi; (void)frameCount;
        }

    private:
        bool bypass = false;
    };

}
