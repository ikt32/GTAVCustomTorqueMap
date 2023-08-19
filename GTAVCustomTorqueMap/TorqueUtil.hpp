#pragma once
#include "Config.hpp"
#include "TorqueScript.hpp"

#include <map>
#include <optional>

namespace CustomTorque {
    struct STorqueDataAdvanced {
        float RealRPM;
        float PowerkW;
        float PowerHP;
    };

    struct STorqueData {
        float NormalizedRPM;
        float TorqueMult;
        float TotalForce;
        float TotalForceNm;
        float TotalForceLbFt;
        float RawMapForceNm;
        std::optional<STorqueDataAdvanced> RPMData;
    };

    float GetScaledValue(const std::map<float, float>& map, float key);
    STorqueData GetTorqueData(Vehicle vehicle, const CConfig& config);

    inline float kW2hp(float val) { return val * 1.34102f; }
    inline float Nm2lbft(float val) { return val * 0.737562f; }
};
