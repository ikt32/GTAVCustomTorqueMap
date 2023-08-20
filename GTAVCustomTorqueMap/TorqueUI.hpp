#pragma once

#include "Config.hpp"
#include "TorqueScript.hpp"
#include <inc/types.h>
#include <optional>

namespace CustomTorque {
    struct SDrawPerformanceSample {
        // Relative
        float RPM;

        // Can be either:
        // Relative ( Torque 0.0-1.0, Power as just RPM * Torque )
        // Metric ( N-m, kW )
        // Burger ( lb-ft, hp )
        struct {
            float Torque;
            float Power;
            std::optional<float> TorqueBoost;
            std::optional<float> PowerBoost;
        } Absolute;

        // Based on max of all Absolute members.
        struct {
            float Torque;
            float Power;
            std::optional<float> TorqueBoost;
            std::optional<float> PowerBoost;
        } Normalized;
    };

    struct SDrawPerformanceValues {
        std::vector<SDrawPerformanceSample> Samples;
        float MaxValue; // for graph display

        // For informative purposes
        float MaxTorqueNm = 0.0f;
        float MaxPowerkW = 0.0f;
        std::optional<float> maxTorqueNmBoost;
        std::optional<float> maxPowerkWBoost;
    };

    struct SGraphDims {
        float RectX;
        float RectY;
        float RectW;
        float RectH;
        float BlockW;
        float BlockH;
    };

    SDrawPerformanceValues GenerateCurveData(const CConfig& config, Vehicle vehicle);
    void DrawCurve(const CConfig& config, Vehicle vehicle, const SDrawPerformanceValues& perfValues);
    void DrawLiveIndicators(const CConfig& config, Vehicle vehicle, const SDrawPerformanceValues& perfValues, const SGraphDims& graphDims);

    void DrawTachometer(CTorqueScript& context, Vehicle vehicle);
}
