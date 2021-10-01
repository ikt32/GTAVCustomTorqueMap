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
        std::optional<STorqueDataAdvanced> RPMData;
    };

    float GetScaledValue(const std::map<float, float>& map, float key);
    STorqueData GetTorqueData(CTorqueScript& context, const CConfig& config);
};

