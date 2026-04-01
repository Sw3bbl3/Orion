#include "orion/FluxShaperNode.h"
#include "orion/HostContext.h"
#include <algorithm>
#include <cmath>

namespace Orion {

    FluxShaperNode::FluxShaperNode() {
        // Default curve: ramp down (sidechain/ducking shape)
        points = {
            {0.0f, 0.0f, 0.0f},
            {0.25f, 1.0f, 0.5f},
            {1.0f, 1.0f, 0.0f}
        };
    }

    std::string FluxShaperNode::getName() const {
        return "Flux Shaper";
    }

    FluxShaperNode::~FluxShaperNode() {}

    void FluxShaperNode::reset() {}

    void FluxShaperNode::process(AudioBlock& block, uint64_t cacheKey, uint64_t projectTimeFrames) {
        (void)cacheKey;
        if (isBypassed() || std::abs(mix.load()) < 0.001f) return;

        double bpm = gHostContext.bpm;
        if (bpm <= 0.1) bpm = 120.0;

        double blockSampleRate = gHostContext.sampleRate;
        if (blockSampleRate <= 0.0) blockSampleRate = 44100.0;

        // Calculate phase based on 1/4 note (beat)
        double secondsPerBeat = 60.0 / bpm;
        double samplesPerBeat = secondsPerBeat * blockSampleRate;

        // projectTimeFrames is the global start frame of the block

        size_t numSamples = block.getSampleCount();
        size_t numCh = block.getChannelCount();
        float currentMix = mix.load();

        std::vector<Point> currentPoints = getPoints();
        if (currentPoints.empty()) return;

        for (size_t i = 0; i < numSamples; ++i) {
            uint64_t currentFrame = projectTimeFrames + i;
            double beatPos = (double)currentFrame / samplesPerBeat;
            double phase = beatPos - std::floor(beatPos); // 0.0 to 1.0

            float gain = calculateGain((float)phase);

            // Apply mix
            // If mix is 1.0, we get purely the gain. If mix is 0.0, we get unity gain (original signal).
            // Original signal is effectively multiplied by 1.0.
            // Wet signal is multiplied by 'gain'.
            // Result = Original * (1 - mix) + (Original * gain) * mix
            // Result = Original * (1 - mix + gain * mix)

            float effectiveGain = 1.0f * (1.0f - currentMix) + gain * currentMix;

            for (size_t ch = 0; ch < numCh; ++ch) {
                block.getChannelPointer(ch)[i] *= effectiveGain;
            }
        }
    }

    void FluxShaperNode::setPoints(const std::vector<Point>& pts) {
        std::lock_guard<std::mutex> lock(pointMutex);
        points = pts;
        // Ensure sorted
        std::sort(points.begin(), points.end(), [](const Point& a, const Point& b){
            return a.x < b.x;
        });
    }

    std::vector<FluxShaperNode::Point> FluxShaperNode::getPoints() const {
        std::unique_lock<std::mutex> lock(const_cast<std::mutex&>(pointMutex), std::try_to_lock);
        if (lock.owns_lock()) {
            return points;
        }
        return {};
    }

    void FluxShaperNode::setMix(float m) {
        mix.store(m);
    }

    bool FluxShaperNode::getChunk(std::vector<char>& data) {
        std::lock_guard<std::mutex> lock(pointMutex);
        size_t numPoints = points.size();
        float m = mix.load();

        size_t size = sizeof(float) + sizeof(uint32_t) + numPoints * sizeof(Point);
        data.resize(size);

        char* ptr = data.data();
        std::memcpy(ptr, &m, sizeof(float)); ptr += sizeof(float);
        uint32_t n = (uint32_t)numPoints;
        std::memcpy(ptr, &n, sizeof(uint32_t)); ptr += sizeof(uint32_t);
        if (n > 0) {
            std::memcpy(ptr, points.data(), n * sizeof(Point));
        }
        return true;
    }

    bool FluxShaperNode::setChunk(const std::vector<char>& data) {
        if (data.size() < sizeof(float) + sizeof(uint32_t)) return false;

        const char* ptr = data.data();
        float m;
        std::memcpy(&m, ptr, sizeof(float)); ptr += sizeof(float);
        mix.store(m);

        uint32_t n;
        std::memcpy(&n, ptr, sizeof(uint32_t)); ptr += sizeof(uint32_t);

        if (data.size() < sizeof(float) + sizeof(uint32_t) + n * sizeof(Point)) return false;

        std::vector<Point> newPoints(n);
        if (n > 0) {
            std::memcpy(newPoints.data(), ptr, n * sizeof(Point));
        }

        setPoints(newPoints);
        return true;
    }

    float FluxShaperNode::calculateGain(float phase) {
        // Find segment
        if (points.empty()) return 1.0f;

        if (phase <= points.front().x) return points.front().y;
        if (phase >= points.back().x) return points.back().y;

        for (size_t i = 0; i < points.size() - 1; ++i) {
            if (phase >= points[i].x && phase < points[i+1].x) {
                const auto& p1 = points[i];
                const auto& p2 = points[i+1];

                float t = (phase - p1.x) / (p2.x - p1.x);

                // Simple curve logic: pow
                // curve 0 = linear
                // curve 0.5 = convex
                // curve -0.5 = concave
                // map curve (-1, 1) to exponent

                float exponent = 1.0f;
                if (p1.curve > 0.0f) exponent = 1.0f - p1.curve * 0.9f;
                else if (p1.curve < 0.0f) exponent = 1.0f + std::abs(p1.curve) * 4.0f;

                t = std::pow(t, exponent);

                return p1.y + t * (p2.y - p1.y);
            }
        }
        return 1.0f;
    }
}
