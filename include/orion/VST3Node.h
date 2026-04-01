#pragma once

#include "EffectNode.h"
#include <string>

namespace Orion {

    class VST3Node : public EffectNode {
    public:
        VST3Node(const std::string& path, const std::string& cidString);
        ~VST3Node() override;

        bool isValid() const;

        std::string getName() const override;


        bool isExternal() const override;
        void openEditor(void* parentWindow) override;
        void closeEditor() override;
        bool getEditorSize(int& width, int& height) const override;

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) override;
        void processMidi(int status, int data1, int data2, int sampleOffset) override;

        int getParameterCount() const override;
        float getParameter(int index) const override;
        void setParameter(int index, float value) override;
        std::string getParameterName(int index) const override;

        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

        std::string getPath() const;
        std::string getCID() const;

        void setFormat(size_t channels, size_t blockSize) override;
        int getLatency() const override;

        bool isInstrument() const override;

    private:
        struct Impl;
        Impl* impl;
    };

}
