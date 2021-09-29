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

    mActiveConfig->TorqueMap.TorqueMultMap = config.TorqueMap.TorqueMultMap;
}

void CTorqueScript::updateTorque() {
    if (!ENTITY::DOES_ENTITY_EXIST(mVehicle))
        return;

    auto handlingPtr = VExt::GetHandlingPtr(mVehicle);
    float baseDriveForce = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fInitialDriveForce);

    float rpm = VExt::GetCurrentRPM(mVehicle);

    auto& baseMapIt = BaseTorqueMultMap.lower_bound(rpm);
    if (baseMapIt != BaseTorqueMultMap.begin()) {
        baseMapIt--;
    }

    float minVal = std::min(baseMapIt->second, std::next(baseMapIt)->second);
    float maxVal = std::max(baseMapIt->second, std::next(baseMapIt)->second);

    float baseMultiplier = map(
        rpm,
        baseMapIt->first,
        std::next(baseMapIt)->first,
        minVal,
        maxVal);

    baseMultiplier = std::clamp(baseMultiplier, minVal, maxVal);

    auto gear = VExt::GetGearCurr(mVehicle);

    if (gear < 2) {
        baseMultiplier = 1.0f;
    }

    auto finalForce = baseDriveForce * baseMultiplier;

    VExt::SetDriveForce(mVehicle, finalForce);
}
