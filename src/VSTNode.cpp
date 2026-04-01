#include "orion/VSTNode.h"
#include "orion/PluginManager.h"
#include "orion/Logger.h"
#include <iostream>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cmath>

namespace Orion {

    VSTNode::VSTNode(VST2::AEffect* effect, const std::string& name, const std::string& path)
        : effect(effect), name(name), path(path), midiQueue(4096)
    {

        vstEventsBlock.numEvents = 0;
        vstEventsBlock.reserved = 0;
        std::memset(vstEventsBlock.events, 0, sizeof(vstEventsBlock.events));

        if (effect) {
             effect->dispatcher(effect, VST2::effMainsChanged, 0, 1, nullptr, 0.0f);
             Logger::info("VST2 Initialized: " + name + " (Inputs: " + std::to_string(effect->numInputs) + ", Outputs: " + std::to_string(effect->numOutputs) + ")");
        } else {
             Logger::error("VST2 Initialized with null effect: " + name);
        }
    }

    VSTNode::~VSTNode() {
        if (effect) {
             effect->dispatcher(effect, VST2::effMainsChanged, 0, 0, nullptr, 0.0f);
             effect->dispatcher(effect, VST2::effClose, 0, 0, nullptr, 0.0f);
             PluginManager::unloadPlugin(effect);
        }
    }

    std::string VSTNode::getName() const {
        return name;
    }

    void VSTNode::setParameter(int index, float value) {
        if (effect && index < effect->numParams) {
            effect->setParameter(effect, index, value);
        }
    }

    float VSTNode::getParameter(int index) const {
        if (effect && index < effect->numParams) {
            return effect->getParameter(effect, index);
        }
        return 0.0f;
    }

    std::string VSTNode::getParameterName(int index) const {
        if (effect && index < effect->numParams) {
            char buf[64];
            effect->dispatcher(effect, VST2::effGetParamName, index, 0, buf, 0.0f);
            return std::string(buf);
        }
        return "Param " + std::to_string(index);
    }

    int VSTNode::getParameterCount() const {
        if (effect) {
            return effect->numParams;
        }
        return 0;
    }

    bool VSTNode::isExternal() const {
        return true;
    }

    void VSTNode::openEditor(void* parentWindow) {
        if (effect) {
            effect->dispatcher(effect, VST2::effEditOpen, 0, 0, parentWindow, 0.0f);
        }
    }

    void VSTNode::closeEditor() {
        if (effect) {
            effect->dispatcher(effect, VST2::effEditClose, 0, 0, nullptr, 0.0f);
        }
    }

    void VSTNode::idle() {
        if (effect) {
            effect->dispatcher(effect, VST2::effEditIdle, 0, 0, nullptr, 0.0f);
        }
    }

    bool VSTNode::getEditorSize(int& width, int& height) const {
        if (effect) {
            VST2::ERect* rect = nullptr;
            effect->dispatcher(effect, VST2::effEditGetRect, 0, 0, &rect, 0.0f);
            if (rect) {
                width = rect->right - rect->left;
                height = rect->bottom - rect->top;
                return true;
            }
        }
        return false;
    }

    void VSTNode::setFormat(size_t numChannels, size_t newBlockSize) {
        AudioNode::setFormat(numChannels, newBlockSize);
        loggedProcessDetails = false;

        if (!effect) return;

        bool rateChanged = (std::abs(sampleRate.load() - lastSampleRate) > 0.1);
        bool sizeChanged = (newBlockSize != lastBlockSize);

        if (!rateChanged && !sizeChanged) return;

        Logger::info("VST2 " + name + " setFormat: Rate=" + std::to_string(sampleRate.load()) + ", Block=" + std::to_string(newBlockSize));


        effect->dispatcher(effect, VST2::effMainsChanged, 0, 0, nullptr, 0.0f);

        if (rateChanged) {
            effect->dispatcher(effect, VST2::effSetSampleRate, 0, 0, nullptr, sampleRate.load());
            lastSampleRate = sampleRate.load();
        }

        if (sizeChanged) {
            effect->dispatcher(effect, VST2::effSetBlockSize, 0, newBlockSize, nullptr, 0.0f);
            lastBlockSize = newBlockSize;
        }


        effect->dispatcher(effect, VST2::effMainsChanged, 0, 1, nullptr, 0.0f);


    }

    void VSTNode::processMidi(int status, int data1, int data2, int sampleOffset) {
        VST2::VstMidiEvent evt;
        std::memset(&evt, 0, sizeof(evt));
        evt.type = VST2::kVstMidiType;
        evt.byteSize = sizeof(VST2::VstMidiEvent);
        evt.deltaFrames = sampleOffset;
        evt.midiData[0] = (char)status;
        evt.midiData[1] = (char)data1;
        evt.midiData[2] = (char)data2;
        evt.midiData[3] = 0;

        midiQueue.push(evt);
    }

    void VSTNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        (void)cacheKey;
        (void)frameStart;
        if (!effect) return;


        VST2::VstMidiEvent localEventStorage[128];
        int count = 0;
        int maxEvents = 128;

        while(count < maxEvents && midiQueue.pop(localEventStorage[count])) {
             vstEventsBlock.events[count] = (VST2::VstEvent*)&localEventStorage[count];
             count++;
        }

        if (count > 0) {
            vstEventsBlock.numEvents = count;
            vstEventsBlock.reserved = 0;

            effect->dispatcher(effect, VST2::effProcessEvents, 0, 0, &vstEventsBlock, 0.0f);
        }


        size_t frames = block.getSampleCount();


        if (silenceBuffer.size() < frames) {
            silenceBuffer.assign(frames, 0.0f);
        } else {
            std::fill(silenceBuffer.begin(), silenceBuffer.begin() + frames, 0.0f);
        }

        if (trashBuffer.size() < frames) {
            trashBuffer.resize(frames);
        }


        int numIn = effect->numInputs;
        int numOut = effect->numOutputs;


        if (inputPointers.size() < (size_t)numIn) inputPointers.resize(numIn);
        if (outputPointers.size() < (size_t)numOut) outputPointers.resize(numOut);

        int blockCh = (int)block.getChannelCount();


        for(int i=0; i<numIn; ++i) {
            if (i < blockCh) {
                inputPointers[i] = block.getChannelPointer(i);
            } else {
                inputPointers[i] = silenceBuffer.data();
            }
        }


        for(int i=0; i<numOut; ++i) {
            if (i < blockCh) {
                outputPointers[i] = block.getChannelPointer(i);
            } else {
                outputPointers[i] = trashBuffer.data();
            }
        }

        if (!loggedProcessDetails) {
            Logger::info("VST2 " + name + " First Process Block:");
            Logger::info("  Host Block: Ch=" + std::to_string(blockCh) + ", Samples=" + std::to_string(frames));
            Logger::info("  Plugin I/O: In=" + std::to_string(numIn) + ", Out=" + std::to_string(numOut));
            loggedProcessDetails = true;
        }


        effect->processReplacing(effect, inputPointers.data(), outputPointers.data(), (VST2::VstInt32)frames);
    }

    bool VSTNode::isInstrument() const {
        return effect && (effect->flags & VST2::effFlagsIsSynth);
    }

    bool VSTNode::getChunk(std::vector<char>& data) {
        if (!effect) return false;

        void* chunkPtr = nullptr;



        VST2::VstIntPtr size = effect->dispatcher(effect, VST2::effGetChunk, 0, 0, &chunkPtr, 0.0f);
        int chunkType = 0;

        if (size <= 0 || !chunkPtr) {

            size = effect->dispatcher(effect, VST2::effGetChunk, 1, 0, &chunkPtr, 0.0f);
            chunkType = 1;
        }

        if (size > 0 && chunkPtr) {



            const char header[] = "OrionVST2";
            size_t headerSize = 10;

            try {
                data.resize(headerSize + 1 + size);
                char* ptr = data.data();


                std::memcpy(ptr, header, headerSize);
                ptr += headerSize;


                *ptr = (char)chunkType;
                ptr += 1;


                std::memcpy(ptr, chunkPtr, size);
                return true;
            } catch (...) {
                Logger::error("VST2 getChunk: Exception during memory copy.");
                return false;
            }
        }
        return false;
    }

    bool VSTNode::setChunk(const std::vector<char>& data) {
        if (!effect || data.empty()) return false;


        int chunkType = 0;
        const char* payload = data.data();
        size_t payloadSize = data.size();

        if (data.size() >= 11) {
            if (std::memcmp(data.data(), "OrionVST2", 10) == 0) {

                chunkType = (int)data[10];
                payload = data.data() + 11;
                payloadSize = data.size() - 11;
            }
        }

        if (payloadSize > 0) {

            if (chunkType != 0 && chunkType != 1) {
                Logger::error("VST2 setChunk: Invalid chunk type: " + std::to_string(chunkType) + ". Defaulting to Bank (0).");
                chunkType = 0;
            }



            effect->dispatcher(effect, VST2::effSetChunk, chunkType, (VST2::VstIntPtr)payloadSize, (void*)payload, 0.0f);
            return true;
        }
        return false;
    }

}
