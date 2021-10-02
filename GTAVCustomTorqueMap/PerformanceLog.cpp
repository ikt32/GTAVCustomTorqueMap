#include "PerformanceLog.hpp"
#include "TorqueScript.hpp"
#include "TorqueUtil.hpp"
#include "Script.hpp"

#include "Util/Math.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <fmt/format.h>
#include <fstream>
#include <vector>

using namespace PerformanceLog;
using VExt = VehicleExtensions;

struct LogEntry {
    float NormalizedRPM;
    float RealRPM;
    float NormalizedForce;
    float PowerkW;
    float PowerHP;
    float TorqueNm;
    float TorqueLbFt;
};

namespace {
    LogState State = LogState::Idle;
    std::vector<LogEntry> Entries;
}

LogState PerformanceLog::GetState() {
    return State;
}

void PerformanceLog::Cancel() {
    State = LogState::Idle;
}

void PerformanceLog::Start() {
    State = LogState::Waiting;
    Entries.clear();
}

void UpdateWaiting(Vehicle playerVehicle) {
    auto rpm = VExt::GetCurrentRPM(playerVehicle);
    auto throttle = VExt::GetThrottle(playerVehicle);
    auto wheelSpeeds = VExt::GetTyreSpeeds(playerVehicle);
    auto gear = VExt::GetGearCurr(playerVehicle);
    auto ratios = VExt::GetGearRatios(playerVehicle);
    auto idleSpeed = 0.2f * (VExt::GetDriveMaxFlatVel(playerVehicle) / ratios[gear]);

    if (rpm <= .21f && throttle >= 1.0f && avg(wheelSpeeds) > 0.75f * idleSpeed) {
        State = LogState::Recording;
    }
}

void UpdateRecording(Vehicle playerVehicle, CTorqueScript& context) {
    auto rpm = VExt::GetCurrentRPM(playerVehicle);
    auto throttle = VExt::GetThrottle(playerVehicle);
    auto wheelSpeeds = VExt::GetTyreSpeeds(playerVehicle);
    auto gear = VExt::GetGearCurr(playerVehicle);
    auto ratios = VExt::GetGearRatios(playerVehicle);
    auto idleSpeed = 0.2f * (VExt::GetDriveMaxFlatVel(playerVehicle) / ratios[gear]);

    const auto& config = *context.ActiveConfig();

    auto torqueData = CustomTorque::GetTorqueData(context, config);
    bool rpmAvailable = torqueData.RPMData != std::nullopt;

    LogEntry entry{
        .NormalizedRPM = rpm,
        .RealRPM = torqueData.RPMData->RealRPM,
        .NormalizedForce = torqueData.TotalForce,
        .PowerkW = torqueData.RPMData->PowerkW,
        .PowerHP = torqueData.RPMData->PowerHP,
        .TorqueNm = torqueData.TotalForceNm,
        .TorqueLbFt = torqueData.TotalForceLbFt,
    };

    Entries.push_back(entry);

    if (rpm >= 1.0f || throttle <= 0.0f) {
        State = LogState::Finished;
    }
}

void PerformanceLog::Finish() {
    State = LogState::Idle;

    std::ofstream logFile("CustomTorqueMap/LatestDataLog.csv", std::ofstream::out | std::ofstream::trunc);

    logFile << "NormalizedRPM,RealRPM,NormalizedForce,PowerkW,PowerHP,TorqueNm,TorqueLbFt" << std::endl;

    for (const auto& entry : Entries) {
        logFile << fmt::format("{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}",
            entry.NormalizedRPM, entry.RealRPM, entry.NormalizedForce, entry.PowerkW, entry.PowerHP, entry.TorqueNm, entry.TorqueLbFt)
            << std::endl;
    }
}

void PerformanceLog::Update(Vehicle playerVehicle) {
    switch (State) {
        case LogState::Idle:
            break;
        case LogState::Waiting:
            UpdateWaiting(playerVehicle);
            break;
        case LogState::Recording:
            UpdateRecording(playerVehicle, *CustomTorque::GetScript());
            break;
        case LogState::Finished:
            Finish();
            break;
        default:
            break;
    }
}
