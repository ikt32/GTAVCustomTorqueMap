#pragma once
#include "inc/types.h"
#include <string>

namespace UI {
    void Notify(int level, const std::string& message);
    void Notify(int level, const std::string& message, bool removePrevious);
}

namespace Util {
    bool PlayerAvailable(Player player, Ped playerPed);
    bool VehicleAvailable(Vehicle vehicle, Ped playerPed);
    bool IsPedOnSeat(Vehicle vehicle, Ped ped, int seat);

    std::string GetFormattedModelName(Hash modelHash);
    std::string GetFormattedVehicleModelName(Vehicle vehicle);
}
