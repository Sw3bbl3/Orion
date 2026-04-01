#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <memory>

namespace Orion {

    enum class AutomationParameter {
        Volume = 0,
        Pan = 1
    };

    struct AutomationParameterSpec {
        float minValue;
        float maxValue;
        float defaultValue;
        bool drawCenterLine;
    };

    AutomationParameterSpec getAutomationParameterSpec(AutomationParameter parameter);
    float clampAutomationValue(AutomationParameter parameter, float value);
    float automationValueToNormalized(AutomationParameter parameter, float value);
    float normalizedToAutomationValue(AutomationParameter parameter, float normalized);

    struct AutomationPoint {
        uint64_t frame;
        float value;
        float curvature = 0.0f;

        bool operator<(const AutomationPoint& other) const {
            return frame < other.frame;
        }
    };

    class AutomationLane {
    public:
        AutomationLane() = default;
        ~AutomationLane() = default;

        void addPoint(uint64_t frame, float value, float curvature = 0.0f);
        void removePoint(int index);
        bool removePointAt(uint64_t frame, double tolerancePixels, double samplesPerPixel);
        int updatePoint(int index, uint64_t frame, float value, float curvature = 0.0f);



        float getValueAt(uint64_t frame) const;

        const std::vector<AutomationPoint>& getPoints() const { return points; }


        std::shared_ptr<AutomationLane> clone() const;

    private:
        std::vector<AutomationPoint> points;
    };

}
