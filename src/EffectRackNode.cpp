#include "orion/EffectRackNode.h"
#include <algorithm>

namespace Orion {

    EffectRackNode::EffectRackNode() {
        effects = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>();
    }

    void EffectRackNode::setParameter(int index, float value) {
        if (index >= 0 && index < 8) {
            macros[index] = std::clamp(value, 0.0f, 1.0f);

        }
    }

    float EffectRackNode::getParameter(int index) const {
        if (index >= 0 && index < 8) return macros[index];
        return 0.0f;
    }

    std::string EffectRackNode::getParameterName(int index) const {
        if (index >= 0 && index < 8) return "Macro " + std::to_string(index + 1);
        return "";
    }

    void EffectRackNode::addEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(chainMutex);

        effect->setSampleRate(getSampleRate());
        effect->setFormat(channels.load(), blockSize.load());

        auto current = std::atomic_load(&effects);
        auto next = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*current);
        next->push_back(effect);
        std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(next));
    }

    void EffectRackNode::removeEffect(std::shared_ptr<EffectNode> effect) {
        std::lock_guard<std::mutex> lock(chainMutex);
        auto current = std::atomic_load(&effects);
        auto next = std::make_shared<std::vector<std::shared_ptr<EffectNode>>>(*current);

        auto it = std::remove(next->begin(), next->end(), effect);
        if (it != next->end()) {
            next->erase(it, next->end());
            std::atomic_store(&effects, std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>>(next));
        }
    }

    std::shared_ptr<const std::vector<std::shared_ptr<EffectNode>>> EffectRackNode::getEffects() const {
        return std::atomic_load(&effects);
    }

    void EffectRackNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t frameStart) {
        auto current = std::atomic_load(&effects);
        for (const auto& effect : *current) {
            effect->processAudio(block, cacheKey, frameStart);
        }
    }

    void EffectRackNode::setFormat(size_t ch, size_t bs) {
        EffectNode::setFormat(ch, bs);
        auto current = std::atomic_load(&effects);
        for (const auto& effect : *current) {
            effect->setFormat(ch, bs);
        }
    }

    void EffectRackNode::setSampleRate(double rate) {
        EffectNode::setSampleRate(rate);
        auto current = std::atomic_load(&effects);
        for (const auto& effect : *current) {
            effect->setSampleRate(rate);
        }
    }

    int EffectRackNode::getLatency() const {
        int total = 0;
        auto current = std::atomic_load(&effects);
        for (const auto& effect : *current) {
            total += effect->getLatency();
        }
        return total;
    }

    void EffectRackNode::moveEffect(int , int ) {

    }

}
