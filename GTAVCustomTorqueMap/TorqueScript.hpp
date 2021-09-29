#pragma once
#include "ScriptSettings.hpp"
#include "Config.hpp"

#include <vector>
#include <string>

class CTorqueScript {
public:
    CTorqueScript(
        CScriptSettings& settings,
        std::vector<CConfig>& configs);
    virtual ~CTorqueScript();
    virtual void Tick();

    CConfig* ActiveConfig() {
        return mActiveConfig;
    }

    void UpdateActiveConfig(bool playerCheck);

    // Applies the passed config onto the current active config.
    void ApplyConfig(const CConfig& config);

    Vehicle GetVehicle() {
        return mVehicle;
    }

protected:
    void updateTorque();
    float getScaledValue(const std::map<float, float>& map, float key);

    const CScriptSettings& mSettings;
    std::vector<CConfig>& mConfigs;
    CConfig mDefaultConfig;

    Vehicle mVehicle;
    CConfig* mActiveConfig;

    bool mIsNPC;
};
