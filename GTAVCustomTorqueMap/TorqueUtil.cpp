#include "TorqueUtil.hpp"
#include "Util/Math.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Memory/Offsets.hpp"

#include <inc/natives.h>

using VExt = VehicleExtensions;

float CustomTorque::GetScaledValue(const std::map<float, float>& map, float key) {
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


CustomTorque::STorqueData CustomTorque::GetTorqueData(Vehicle vehicle, const CConfig& config) {

    float rpm = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle) ?
        VExt::GetCurrentRPM(vehicle) : 0.00f;

    float mapMultiplier = CustomTorque::GetScaledValue(config.Data.TorqueMultMap, rpm);

    auto forces = VExt::GetWheelPower(vehicle);
    float totalForce = 0.0f;

    for (uint32_t i = 0; i < forces.size(); ++i) {
        totalForce += forces[i];
    }

    auto handlingPtr = VExt::GetHandlingPtr(vehicle);
    float weight = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fMass);
    float baseDriveForce = weight * *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fInitialDriveForce);

    float gearRatio = VExt::GetGearRatios(vehicle)[VExt::GetGearCurr(vehicle)];
    float totalForceNm = (totalForce * weight) / gearRatio;
    float totalForceLbFt = totalForceNm / 1.356f;

    STorqueData torqueData{
        .NormalizedRPM = rpm,
        .TorqueMult = mapMultiplier,
        .TotalForce = totalForce,
        .TotalForceNm = totalForceNm,
        .TotalForceLbFt = totalForceLbFt,
        .RawMapForceNm = mapMultiplier * baseDriveForce,
        .RPMData = std::nullopt,
    };

    if (config.Data.IdleRPM != 0 && config.Data.RevLimitRPM != 0) {
        float realRPM = map(rpm, 0.2f, 1.0f,
            (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

        if (rpm < 0.2f) {
            realRPM = map(rpm, 0.0f, 0.2f, 0.0f, (float)config.Data.IdleRPM);
        }

        float power = (0.10471975512f * totalForceNm * realRPM) / 1000.0f;
        float horsepower = (totalForceLbFt * realRPM) / 5252.0f;

        torqueData.RPMData = STorqueDataAdvanced{
            .RealRPM = realRPM,
            .PowerkW = power,
            .PowerHP = horsepower
        };
    }

    return torqueData;
}
