#pragma once
#include "EffectNode.h"
#include <vector>
#include <atomic>
#include <cmath>
#include <mutex>

namespace Orion {

    class FluxShaperNode : public EffectNode {
    public:
        FluxShaperNode();
        ~FluxShaperNode() override;

        std::string getName() const override;

        void process(AudioBlock& block, uint64_t cacheKey, uint64_t projectTimeFrames) override;
        void reset();

        // Points are stored as (x, y) where x is phase [0, 1] and y is gain [0, 1]
        // Sorted by x.
        struct Point {
            float x;
            float y;
            float curve; // 0 = linear, >0 convex, <0 concave
        };

        void setPoints(const std::vector<Point>& points);
        std::vector<Point> getPoints() const;

        void setMix(float mix);
        float getMix() const { return mix.load(); }

        bool getChunk(std::vector<char>& data) override;
        bool setChunk(const std::vector<char>& data) override;

    private:
        std::atomic<float> mix { 1.0f };
        std::vector<Point> points;
        mutable std::mutex pointMutex;

        float calculateGain(float phase);
    };
}
