#include "TorqueScript.hpp"

#include "Constants.hpp"
#include "VehicleMods.hpp"

#include "Util/Math.hpp"
#include "Util/Paths.hpp"
#include "Util/Game.hpp"
#include "Util/UI.hpp"
#include "Util/String.hpp"
#include "Util/Logger.hpp"

#include "Memory/Offsets.hpp"
#include "Memory/VehicleExtensions.hpp"

#include <inc/enums.h>
#include <inc/natives.h>
#include <fmt/format.h>
#include <filesystem>
#include <algorithm>

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
    if (!ENTITY::DOES_ENTITY_EXIST(mVehicle) ||
        !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(mVehicle) ||
        mActiveConfig == &mDefaultConfig)
        return;

    auto handlingPtr = VExt::GetHandlingPtr(mVehicle);
    float baseDriveForce = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fInitialDriveForce);

    int tuningLevel = VEHICLE::GET_VEHICLE_MOD(
        mVehicle, eVehicleModType::VMT_ENGINE);

    float tuningMultiplier = 1.0f;
    if (tuningLevel > -1) {
        // Value of 100 -> 1.2 mult.
        int modValue = VEHICLE::GET_VEHICLE_MOD_MODIFIER_VALUE(
            mVehicle, eVehicleModType::VMT_ENGINE, tuningLevel);

        tuningMultiplier = map((float)modValue, 0.0f, 100.0f, 1.0f, 1.2f);
    }

    float rpm = VExt::GetCurrentRPM(mVehicle);

    float baseMultiplier = GetScaledValue(BaseTorqueMultMap, rpm);

    auto gear = VExt::GetGearCurr(mVehicle);

    if (gear < 2) {
        baseMultiplier = 1.0f;
    }

    float mapMultiplier = 1.0f;

    if (mActiveConfig->Data.TorqueMultMap.size() >= 3) {
        mapMultiplier = GetScaledValue(mActiveConfig->Data.TorqueMultMap, rpm);
    }

    auto finalForce = baseDriveForce * tuningMultiplier * baseMultiplier * mapMultiplier;

    if (mSettings.Debug.DisplayInfo) {
        float displaySize = 5.0f;
        if (mIsNPC) {
            displaySize = 8.0f;
        }

        Vector3 loc = ENTITY::GET_ENTITY_COORDS(mVehicle, true);
        Vector3 cameraPos = CAM::GET_GAMEPLAY_CAM_COORD();
        float distance = Distance(cameraPos, loc);

        if (distance < 50.0f) {
            loc.z += 1.0f;
            UI::ShowText3DScaled(loc, displaySize, {
                { fmt::format("BaseForce: {:.3f}", baseDriveForce) },
                { fmt::format("Tuning: {}/{:.3f}", tuningLevel, tuningMultiplier) },
                { fmt::format("RPM: {:.3f}",       rpm) },
                { fmt::format("BaseMult: {:.3f}",  baseMultiplier) },
                { fmt::format("MapMult: {:.3f}",   mapMultiplier) },
                { fmt::format("Final: {:.3f}",     finalForce) },
                });
        }
    }

    VExt::SetDriveForce(mVehicle, finalForce);
}

float CTorqueScript::GetScaledValue(const std::map<float, float>& map, float key) {
    auto mapIt = map.lower_bound(key);

    // Go back twice to get the last two elements, since the value is scaled anyway.
    if (mapIt == map.end()) {
        mapIt--;
    }

    if (mapIt != map.begin()) {
        mapIt--;
    }

    float scaledValue = ::map(
        key,
        mapIt->first,
        std::next(mapIt)->first,
        mapIt->second,
        std::next(mapIt)->second);

    if (scaledValue < 0.0f)
        scaledValue = 0.0f;

    return scaledValue;
}
