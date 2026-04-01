#include "orion/Automation.h"
#include <algorithm>
#include <cmath>

namespace {
    constexpr Orion::AutomationParameterSpec kVolumeSpec { 0.0f, 1.0f, 1.0f, false };
    constexpr Orion::AutomationParameterSpec kPanSpec { -1.0f, 1.0f, 0.0f, true };
}

namespace Orion {

    AutomationParameterSpec getAutomationParameterSpec(AutomationParameter parameter) {
        switch (parameter) {
            case AutomationParameter::Pan: return kPanSpec;
            case AutomationParameter::Volume:
            default: return kVolumeSpec;
        }
    }

    float clampAutomationValue(AutomationParameter parameter, float value) {
        const auto spec = getAutomationParameterSpec(parameter);
        return std::clamp(value, spec.minValue, spec.maxValue);
    }

    float automationValueToNormalized(AutomationParameter parameter, float value) {
        const auto spec = getAutomationParameterSpec(parameter);
        const float clamped = clampAutomationValue(parameter, value);
        if (spec.maxValue <= spec.minValue) return 0.0f;
        return (clamped - spec.minValue) / (spec.maxValue - spec.minValue);
    }

    float normalizedToAutomationValue(AutomationParameter parameter, float normalized) {
        const auto spec = getAutomationParameterSpec(parameter);
        const float n = std::clamp(normalized, 0.0f, 1.0f);
        return spec.minValue + n * (spec.maxValue - spec.minValue);
    }

    void AutomationLane::addPoint(uint64_t frame, float value, float curvature) {



        auto it = std::find_if(points.begin(), points.end(), [frame](const AutomationPoint& p){
            return p.frame == frame;
        });

        if (it != points.end()) {
            it->value = value;
            it->curvature = curvature;
        } else {
            points.push_back({frame, value, curvature});
            std::sort(points.begin(), points.end());
        }
    }

    void AutomationLane::removePoint(int index) {
        if (index >= 0 && index < (int)points.size()) {
            points.erase(points.begin() + index);
        }
    }

    bool AutomationLane::removePointAt(uint64_t frame, double tolerancePixels, double samplesPerPixel) {

        int bestIdx = -1;
        uint64_t toleranceFrames = (uint64_t)(tolerancePixels * samplesPerPixel);

        for (int i = 0; i < (int)points.size(); ++i) {
            int64_t diff = (int64_t)points[i].frame - (int64_t)frame;
            if (std::abs(diff) <= (int64_t)toleranceFrames) {
                bestIdx = i;
                break;
            }
        }

        if (bestIdx != -1) {
            points.erase(points.begin() + bestIdx);
            return true;
        }
        return false;
    }

    int AutomationLane::updatePoint(int index, uint64_t frame, float value, float curvature) {
        if (index >= 0 && index < (int)points.size()) {
            points[index].frame = frame;
            points[index].value = value;
            points[index].curvature = curvature;

            std::sort(points.begin(), points.end());


            for(int i=0; i<(int)points.size(); ++i) {
                if (points[i].frame == frame && points[i].value == value) {
                    return i;
                }
            }
        }
        return -1;
    }

    float AutomationLane::getValueAt(uint64_t frame) const {
        if (points.empty()) return 0.0f;


        if (frame <= points.front().frame) return points.front().value;


        if (frame >= points.back().frame) return points.back().value;


        auto it = std::upper_bound(points.begin(), points.end(), AutomationPoint{frame, 0.0f});


        const auto& p2 = *it;
        const auto& p1 = *(it - 1);

        double t = (double)(frame - p1.frame) / (double)(p2.frame - p1.frame);




        if (std::abs(p1.curvature) > 0.001f) {

            double exponent = std::exp(p1.curvature * 2.0);
            t = std::pow(t, exponent);
        }

        return p1.value + (float)(t * (p2.value - p1.value));
    }

    std::shared_ptr<AutomationLane> AutomationLane::clone() const {
        auto newLane = std::make_shared<AutomationLane>();
        newLane->points = this->points;
        return newLane;
    }

}
