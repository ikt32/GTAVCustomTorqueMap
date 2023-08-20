#include "ScriptUtils.h"
#include "../Constants.hpp"
#include "../Memory/Offsets.hpp"
#include "../Memory/VehicleExtensions.hpp"
#include "../Util/Math.hpp"
#include <inc/enums.h>
#include <inc/natives.h>

bool Util::PlayerAvailable(Player player, Ped playerPed) {
    if (!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE) ||
        CUTSCENE::IS_CUTSCENE_PLAYING() ||
        !ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        ENTITY::IS_ENTITY_DEAD(playerPed, 0)) {
        return false;
    }
    return true;
}

bool Util::VehicleAvailable(Vehicle vehicle, Ped playerPed) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        PED::IS_PED_SITTING_IN_VEHICLE(playerPed, vehicle) &&
        playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1, 0);
}

bool Util::IsPedOnSeat(Vehicle vehicle, Ped ped, int seat) {
    Vehicle pedVehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    return vehicle == pedVehicle &&
        VEHICLE::GET_PED_IN_VEHICLE_SEAT(pedVehicle, seat, 0) == ped;
}

std::string Util::GetFormattedModelName(Hash modelHash) {
    const char* name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(modelHash);
    std::string displayName = HUD::GET_FILENAME_FOR_AUDIO_CONVERSATION(name);
    if (displayName == "NULL") {
        displayName = name;
    }
    return displayName;
}

std::string Util::GetFormattedVehicleModelName(Vehicle vehicle) {
    return GetFormattedModelName(ENTITY::GET_ENTITY_MODEL(vehicle));
}

bool Util::VehicleHasTurboMod(Vehicle vehicle) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        VEHICLE::IS_TOGGLE_MOD_ON(vehicle, VehicleToggleModTurbo);
}

float Util::GetEngineModTorqueMult(Vehicle vehicle) {
    if (vehicle == 0 ||
        !ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        return 1.0f;
    }

    const int modLevel = VEHICLE::GET_VEHICLE_MOD(vehicle, eVehicleMod::VehicleModEngine);
    if (modLevel < 0) {
        return 1.0f;
    }

    const int modVal = VEHICLE::GET_VEHICLE_MOD_MODIFIER_VALUE(vehicle, eVehicleMod::VehicleModEngine, modLevel);
    return map(static_cast<float>(modVal), 0.0f, 100.0f, 1.0f, 1.2f);
}

float Util::GetHandlingTorqueNm(Vehicle vehicle) {
    if (vehicle == 0 ||
        !ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        return 0.0f;
    }

    auto handlingPtr = VehicleExtensions::GetHandlingPtr(vehicle);
    float weight = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fMass);
    float torqueNm = weight * *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fInitialDriveForce);
    return torqueNm;
}
