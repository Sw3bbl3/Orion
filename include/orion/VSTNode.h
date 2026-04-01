#pragma once

#include "EffectNode.h"
#include "VST2.h"
#include "RingBuffer.h"
#include <string>
#include <vector>

namespace Orion {

    class VSTNode : public EffectNode {
    public:
        VSTNode(VST2::AEffect* effect, const std::string& name, const std::string& path = "");
        ~VSTNode() override;

        std::string getName() const override;
        std::string getPath() const { return path; }

        void setParameter(int index, float value) override;
        float getParameter(int index) const override;
        std::string getParameterName(int index) const override;
        int getParameterCount() const override;

        bool isExternal() const override;
        void openEditor(void* parentWindow) override;
        void closeEditor() override;
        void idle() override;
        bool getEditorSize(int& width, int& height) const override;

        void setFormat(size_t numChannels, size_t newBlockSize) override;


        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

        bool isInstrument() const override;

    protected:
        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void processMidi(int status, int data1, int data2, int sampleOffset) override;

    private:
        VST2::AEffect* effect;
        std::string name;
        std::string path;


        std::vector<float*> inputPointers;
        std::vector<float*> outputPointers;


        std::vector<float> silenceBuffer;
        std::vector<float> trashBuffer;


        RingBuffer<VST2::VstMidiEvent> midiQueue;


        struct VstEventsContainer {
            VST2::VstInt32 numEvents;
            VST2::VstIntPtr reserved;
            VST2::VstEvent* events[128];
        } vstEventsBlock;

        double lastSampleRate = 0.0;
        size_t lastBlockSize = 0;
        bool loggedProcessDetails = false;
    };

}
