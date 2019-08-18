#include "script.h"

#include <filesystem>
#include <fmt/core.h>
#include <inc/main.h>
#include <inc/natives.h>

#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/Paths.h"
#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Util/UIUtils.h"
#include "../../GTAVManualTransmission/Gears/Util/MathExt.h"

#include "scriptSettings.h"
#include "scriptMenu.h"
#include "gearInfo.h"
#include "StringUtils.h"
#include "../../GTAVManualTransmission/Gears/Memory/Offsets.hpp"
#include "Timer.h"

std::string absoluteModPath;
std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsStickFile;
std::string settingsMenuFile;

NativeMenu::Menu menu;

ScriptSettings settings;
VehicleExtensions ext;

Vehicle previousVehicle;
Vehicle currentVehicle;

std::string gearConfigDir;
std::vector<GearInfo> gearConfigs;

// Only used to restore changes the game applies, like tuning gearbox etc
std::vector<std::pair<Vehicle, GearInfo>> currentConfigs;

float cvtMaxRpm = 0.9f;
float cvtMinRpm = 0.3f;

Timer auxTimer(1000);

void applyConfig(const GearInfo& config, Vehicle vehicle, bool notify);

void parseConfigs() {
    gearConfigs.clear();
    for (auto & p : std::filesystem::directory_iterator(gearConfigDir)) {
        if (p.path().extension() == ".xml") {
            GearInfo info = GearInfo::ParseConfig(p.path().string());
            if (!info.mParseError) {
                info.mPath = p.path().string();
                gearConfigs.push_back(info);
            }
            else {
                logger.Write(ERROR, "%s skipped due to errors", p.path().stem().string().c_str());
            }
        }
    }
}

void eraseConfigs() {
    uint32_t error = 0;
    uint32_t deleted = 0;
    for (const auto& config : gearConfigs) {
        if (config.mMarkedForDeletion) {
            if (!std::filesystem::remove(config.mPath)) {
                logger.Write(ERROR, "Failed to remove file %s", config.mPath.c_str());
                error++;
            }
            else {
                logger.Write(DEBUG, "Removed file %s", config.mPath.c_str());
                deleted++;
            }
        }
    }
    if (error) {
        showNotification(fmt::format("Failed to remove {} gear config(s).", error));
    }
    if (settings.AutoNotify && deleted) {
        showNotification(fmt::format("Removed {} gear config(s).", deleted));
    }
}

void update_player() {
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    if (ENTITY::DOES_ENTITY_EXIST(currentVehicle) && currentVehicle != previousVehicle) {
        previousVehicle = currentVehicle;

        if (std::find_if(currentConfigs.begin(), currentConfigs.end(), [=](const auto& cfg) {return cfg.first == currentVehicle; }) == currentConfigs.end()) {
            currentConfigs.emplace_back(currentVehicle, GearInfo("noconfig",
                "noconfig",
                "noconfig",
                ext.GetTopGear(currentVehicle),
                ext.GetDriveMaxFlatVel(currentVehicle),
                ext.GetGearRatios(currentVehicle),
                LoadType::None)
            );
            logger.Write(DEBUG, "[Management] Appended new vehicle: 0x%X", currentVehicle);
        }

        for (const auto& config : gearConfigs) {
            bool sameModel = GAMEPLAY::GET_HASH_KEY((char*)config.mModelName.c_str()) == ENTITY::GET_ENTITY_MODEL(currentVehicle);
            bool samePlate = to_lower(config.mLicensePlate) == to_lower(VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(currentVehicle));

            switch (config.mLoadType) {
                case LoadType::Plate: {
                    if (!settings.AutoLoad)
                        break;
                    if (sameModel && samePlate) {
                        applyConfig(config, currentVehicle, settings.AutoNotify);
                        return;
                    }
                    break;
                }
                case LoadType::Model: {
                    if (!settings.AutoLoadGeneric)
                        break;
                    if (sameModel) {
                        applyConfig(config, currentVehicle, settings.AutoNotify);
                        return;
                    }
                    break;
                }
                case LoadType::None: break;
            }
        }
    }
}

void update_cvt() {    
    if (settings.EnableCVT && ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        uint8_t origNumGears = *reinterpret_cast<uint8_t *>(ext.GetHandlingPtr(currentVehicle) + hOffsets.nInitialDriveGears);
        if (origNumGears > 1 && ext.GetTopGear(currentVehicle) == 1) {
            float defaultMaxFlatVel = ext.GetDriveMaxFlatVel(currentVehicle);
            float currSpeed = avg(ext.GetTyreSpeeds(currentVehicle));
            float newRatio = map(currSpeed, 0.0f, defaultMaxFlatVel, 3.33f, 0.9f) * 0.75f * std::clamp(ext.GetThrottleP(currentVehicle), 0.1f, 1.0f);
            newRatio = std::clamp(newRatio, 0.6f, 3.33f);
            *ext.GetGearRatioPtr(currentVehicle, 1) = newRatio;
        }
    }
}

void update_reapply() {
    // remove entities that stopped existing
    currentConfigs.erase(std::remove_if(currentConfigs.begin(), currentConfigs.end(), 
        [=](const auto& cfgPair) {
            if (!ENTITY::DOES_ENTITY_EXIST(cfgPair.first)) {
                logger.Write(DEBUG, "[Management] Erased stale vehicle: 0x%X", cfgPair.first);
            }
            return !ENTITY::DOES_ENTITY_EXIST(cfgPair.first);
        }), currentConfigs.end());

    // Skip actually checking and setting ratios, but do keep updating the list.
    if (!settings.RestoreRatios)
        return;

    for (const auto& cfgPair : currentConfigs) {
        auto vehicle = cfgPair.first;
        auto config = cfgPair.second;
        bool topGearChanged = ext.GetTopGear(vehicle) != config.mTopGear;
        bool driveMaxVelChanged = ext.GetDriveMaxFlatVel(vehicle) != config.mDriveMaxVel;
        bool anyRatioChanged = false;
        if (!topGearChanged && !driveMaxVelChanged) {
            auto extRatios = ext.GetGearRatios(vehicle);
            for (uint32_t i = 0; i < config.mTopGear; ++i) {
                if (extRatios[i] != config.mRatios[i]) {
                    anyRatioChanged = true;
                    break;
                }
            }
        }

        if (topGearChanged || driveMaxVelChanged || anyRatioChanged) {
            ext.SetTopGear(vehicle, config.mTopGear);
            ext.SetDriveMaxFlatVel(vehicle, config.mDriveMaxVel);
            ext.SetInitialDriveMaxFlatVel(vehicle, config.mDriveMaxVel / 1.2f);
            ext.SetGearRatios(vehicle, config.mRatios);
            if (settings.AutoNotify) {
                showNotification(fmt::format("Restored {}: \n"
                    "Top gear = {}\n"
                    "Top speed = {:.0f} kph", vehicle, config.mTopGear,
                    3.6f * config.mDriveMaxVel / config.mRatios[config.mTopGear]));
            }
        }
    }
}

void main() {
    logger.Write(INFO, "Script started");
    absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + MOD_DIRECTORY;
    settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    gearConfigDir = absoluteModPath + "\\Configs";
    
    settings.SetFiles(settingsGeneralFile);
    menu.SetFiles(settingsMenuFile);

    settings.Read();
    logger.SetMinLevel(settings.Debug ? DEBUG : INFO);

    menu.ReadSettings();
    menu.Initialize();
    ext.initOffsets();
    parseConfigs();

    menu.RegisterOnMain([&] {
        menu.ReadSettings();
        settings.Read();
        logger.SetMinLevel(settings.Debug ? DEBUG : INFO);
        parseConfigs();
    });

    menu.RegisterOnExit([&] {
        settings.Save();
        eraseConfigs();
    });

    while (true) {
        update_player();
        update_menu();
        update_cvt();
        if (auxTimer.Expired()) {
            auxTimer.Reset();
            update_reapply();
        }
        WAIT(0);
    }
}

void ScriptMain() {
    srand(GetTickCount());
    main();
}