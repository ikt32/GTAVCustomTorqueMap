#include "PerformanceLog.hpp"
#include "Constants.hpp"
#include "TorqueScript.hpp"
#include "TorqueUtil.hpp"
#include "Script.hpp"

#include "Util/AddonSpawnerCache.hpp"
#include "Util/Logger.hpp"
#include "Util/Math.hpp"
#include "Util/UI.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <inc/natives.h>
#include <chrono>
#include <format>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <vector>

using namespace PerformanceLog;
using VExt = VehicleExtensions;

struct LogEntry {
    float NormalizedRPM;
    float RealRPM;
    float PowerkW;
    float PowerHP;
    float TorqueNm;
    float TorqueLbFt;
    float TorqueMapNm;
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
    UI::Notify("Cancelled recording.", true);
}

void PerformanceLog::Start() {
    Entries.clear();

    State = LogState::Waiting;
    UI::Notify("Starting recording.", true);
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
        .RealRPM = rpmAvailable ? torqueData.RPMData->RealRPM : 0.0f,
        .PowerkW = rpmAvailable ? torqueData.RPMData->PowerkW : 0.0f,
        .PowerHP = rpmAvailable ? torqueData.RPMData->PowerHP : 0.0f,
        .TorqueNm = torqueData.TotalForceNm,
        .TorqueLbFt = torqueData.TotalForceLbFt,
        .TorqueMapNm = torqueData.RawMapForceNm,
    };

    Entries.push_back(entry);

    if (rpm >= 1.0f || throttle <= 0.0f) {
        State = LogState::Finished;
    }
}

void PerformanceLog::Finish(Vehicle playerVehicle) {

    const auto t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::tm local_time;
    
    if (auto result = localtime_s(&local_time, &t); result != 0) {
        UI::Notify(std::format("~r~Internal error, could not create recording"), true);
        logger.Write(ERROR, "localtime_s failed: %d", result);
        return;
    }

    std::stringstream ss;
    ss << std::put_time(&local_time, "%Y%m%d-%H%M%S");
    const std::string creationTime = ss.str();

    Hash model = ENTITY::GET_ENTITY_MODEL(playerVehicle);
    auto& asCache = ASCache::Get();
    auto it = asCache.find(model);
    std::string modelName = it == asCache.end() ? VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(model) : it->second;
    std::string logFileName = std::format("{}-{}.csv", creationTime, modelName);

    std::ofstream logFile(std::format("CustomTorqueMap/{}", logFileName),
        std::ofstream::out | std::ofstream::trunc);

    logFile << "NormalizedRPM,RealRPM,PowerkW,PowerHP,TorqueNm,TorqueLbFt,TorqueMapNm" << std::endl;

    for (const auto& entry : Entries) {
        logFile << std::format("{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f},{:.3f}",
            entry.NormalizedRPM, entry.RealRPM, entry.PowerkW, entry.PowerHP, entry.TorqueNm, entry.TorqueLbFt, entry.TorqueMapNm)
            << std::endl;
    }

    State = LogState::Idle;
    UI::Notify(std::format("Saved recording {}", logFileName), true);
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
            Finish(playerVehicle);
            break;
        default:
            break;
    }
}
