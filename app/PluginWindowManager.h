#pragma once
#include <JuceHeader.h>
#include "orion/EffectNode.h"
#include <map>
#include <memory>

namespace Orion { class AudioEngine; }

class PluginWindow;

class PluginWindowManager
{
public:
    static PluginWindowManager& getInstance();

    void showEditor(std::shared_ptr<Orion::EffectNode> node);
    void closeEditor(Orion::EffectNode* node);


    void handleWindowClosed(Orion::EffectNode* node);

    void setAudioEngine(Orion::AudioEngine* engine) { audioEngine = engine; }
    Orion::AudioEngine* getAudioEngine() const { return audioEngine; }

private:
    PluginWindowManager() = default;
    ~PluginWindowManager();

    Orion::AudioEngine* audioEngine = nullptr;


    std::map<Orion::EffectNode*, std::unique_ptr<PluginWindow>> windows;
};
