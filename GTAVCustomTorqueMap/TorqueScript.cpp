#include "TorqueScript.hpp"

#include "Constants.hpp"
#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Game.hpp"
#include "Util/UI.hpp"
#include "Util/String.hpp"

#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>
#include <filesystem>
#include <algorithm>
#include "Util/Logger.hpp"

using VExt = VehicleExtensions;

// There is a non-linear power decrease
// Torque @ wheels slightly increase around 0.9 RPM, but is negligible imo
namespace {
    const std::map<float, float> BaseTorqueMultMap {
        { 0.0f, 1.0f },
        { 0.8f, 1.0f },
        { 1.0f, 1.0f/0.6f },
    };
}

CTorqueScript::CTorqueScript(CScriptSettings& settings, std::vector<CConfig>& configs)
    : mSettings(settings)
    , mConfigs(configs)
    , mVehicle(0)
    , mActiveConfig(nullptr)
    , mIsNPC(false) {
}

CTorqueScript::~CTorqueScript() = default;

void CTorqueScript::Tick() {
    Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    // Update active vehicle and config
    if (playerVehicle != mVehicle) {
        mVehicle = playerVehicle;

        UpdateActiveConfig(true);
    }

    if (mActiveConfig && Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false)) {
        updateTorque();
    }
}

void CTorqueScript::UpdateActiveConfig(bool playerCheck) {
    if (playerCheck) {
        if (!Util::VehicleAvailable(mVehicle, PLAYER::PLAYER_PED_ID(), false)) {
            mActiveConfig = nullptr;
            return;
        }
    }

    Hash model = ENTITY::GET_ENTITY_MODEL(mVehicle);
    std::string plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(mVehicle);

    // First pass - match model and plate
    auto foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const CConfig& config) {
        bool modelMatch = config.ModelHash == model;
        bool plateMatch = Util::strcmpwi(config.Plate, plate);
        return modelMatch && plateMatch;
        });

    // second pass - match model with any plate
    if (foundConfig == mConfigs.end()) {
        foundConfig = std::find_if(mConfigs.begin(), mConfigs.end(), [&](const CConfig& config) {
            bool modelMatch = config.ModelHash == model;
            bool plateMatch = config.Plate.empty();
            return modelMatch && plateMatch;
            });
    }

    // third pass - use default
    if (foundConfig == mConfigs.end()) {
        mActiveConfig = &mDefaultConfig;
    }
    else {
        mActiveConfig = &*foundConfig;
    }
}

void CTorqueScript::ApplyConfig(const CConfig& config) {
    if (!mActiveConfig)
        return;

    mActiveConfig->Data.TorqueMultMap = config.Data.TorqueMultMap;
}

void CTorqueScript::updateTorque() {
    if (!ENTITY::DOES_ENTITY_EXIST(mVehicle))
        return;

    auto handlingPtr = VExt::GetHandlingPtr(mVehicle);
    float baseDriveForce = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fInitialDriveForce);

    float rpm = VExt::GetCurrentRPM(mVehicle);

    float baseMultiplier = getScaledValue(BaseTorqueMultMap, rpm);

    auto gear = VExt::GetGearCurr(mVehicle);

    if (gear < 2) {
        baseMultiplier = 1.0f;
    }

    float mapMultiplier = 1.0f;

    if (mActiveConfig->Data.TorqueMultMap.size() >= 3) {
        mapMultiplier = getScaledValue(mActiveConfig->Data.TorqueMultMap, rpm);
    }

    auto finalForce = baseDriveForce * baseMultiplier * mapMultiplier;

    UI::ShowText(0.25f, 0.25f, 0.25f,
        fmt::format(
            "BaseForce: {:.2f}\n"
            "RPM: {:.2f}\n"
            "BaseMult: {:.2f}\n"
            "MapMult: {:.2f}\n"
            "Final: {:.2f}",
            baseDriveForce, rpm, baseMultiplier, mapMultiplier, finalForce));

    VExt::SetDriveForce(mVehicle, finalForce);
}

float CTorqueScript::getScaledValue(const std::map<float, float>& map, float key) {
    auto& mapIt = map.lower_bound(key);
    if (mapIt == map.end()) {
        return 1.0f;
    }
    if (mapIt != map.begin()) {
        mapIt--;
    }

    float minVal = std::min(mapIt->second, std::next(mapIt)->second);
    float maxVal = std::max(mapIt->second, std::next(mapIt)->second);

    float scaledValue = ::map(
        key,
        mapIt->first,
        std::next(mapIt)->first,
        minVal,
        maxVal);

    return std::clamp(scaledValue, minVal, maxVal);
}
