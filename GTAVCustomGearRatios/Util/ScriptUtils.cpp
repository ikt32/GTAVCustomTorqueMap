#include "ScriptUtils.h"

#include "../Constants.h"
#include "MathExt.h"
#include "UIUtils.h"
#include "inc/natives.h"
#include "fmt/format.h"

namespace {
    int notificationHandle = 0;
}

void UI::Notify(int level, const std::string& message) {
    Notify(level, message, true);
}

void UI::Notify(int, const std::string& message, bool removePrevious) {
    int* notifHandleAddr = nullptr;
    if (removePrevious) {
        notifHandleAddr = &notificationHandle;
    }
    showNotification(fmt::format("{}\n{}", Constants::NotificationPrefix, message), notifHandleAddr);
}

bool Util::PlayerAvailable(Player player, Ped playerPed) {
    if (!PLAYER::IS_PLAYER_CONTROL_ON(player) ||
        PLAYER::IS_PLAYER_BEING_ARRESTED(player, TRUE) ||
        CUTSCENE::IS_CUTSCENE_PLAYING() ||
        !ENTITY::DOES_ENTITY_EXIST(playerPed) ||
        ENTITY::IS_ENTITY_DEAD(playerPed)) {
        return false;
    }
    return true;
}

bool Util::VehicleAvailable(Vehicle vehicle, Ped playerPed) {
    return vehicle != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle) &&
        PED::IS_PED_SITTING_IN_VEHICLE(playerPed, vehicle) &&
        playerPed == VEHICLE::GET_PED_IN_VEHICLE_SEAT(vehicle, -1);
}

bool Util::IsPedOnSeat(Vehicle vehicle, Ped ped, int seat) {
    Vehicle pedVehicle = PED::GET_VEHICLE_PED_IS_IN(ped, false);
    return vehicle == pedVehicle &&
        VEHICLE::GET_PED_IN_VEHICLE_SEAT(pedVehicle, seat) == ped;
}

std::string Util::GetFormattedModelName(Hash modelHash) {
    const char* name = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(modelHash);
    std::string displayName = UI::_GET_LABEL_TEXT(name);
    if (displayName == "NULL") {
        displayName = name;
    }
    return displayName;
}

std::string Util::GetFormattedVehicleModelName(Vehicle vehicle) {
    return GetFormattedModelName(ENTITY::GET_ENTITY_MODEL(vehicle));
}
