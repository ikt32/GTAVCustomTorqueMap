#include "script.h"
#include "scriptSettings.h"
#include "scriptMenu.h"
#include "gearInfo.h"

#include "Memory/VehicleExtensions.hpp"

#include "Util/Timer.h"
#include "Util/Logger.hpp"
#include "Util/ScriptUtils.h"

#include <menu.h>

#include <inc/main.h>
#include <inc/natives.h>

#include <fmt/core.h>

#include <filesystem>

#include "Constants.h"
#include "Memory/Offsets.hpp"
#include "Util/MathExt.h"
#include "Util/Paths.h"
#include "Util/Strings.h"
#include "Util/UIUtils.h"

std::string absoluteModPath;
std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsStickFile;
std::string settingsMenuFile;

NativeMenu::Menu menu;

ScriptSettings settings;

Vehicle previousVehicle;
Vehicle currentVehicle;

std::string gearConfigDir;
std::vector<GearInfo> gearConfigs;

VehicleExtensions ext;

// Only used to restore changes the game applies, like tuning gearbox etc
std::vector<std::pair<Vehicle, GearInfo>> currentConfigs;

float cvtMaxRpm = 0.9f;
float cvtMinRpm = 0.3f;

Timer auxTimer(1000);

void applyConfig(const GearInfo& config, Vehicle vehicle, bool notify);

void parseConfigs() {
    namespace fs = std::filesystem;
    gearConfigs.clear();

    if (!(fs::exists(fs::path(gearConfigDir)) && fs::is_directory(fs::path(gearConfigDir)))) {
        logger.Write(ERROR, "Directory [%s] not found, creating an empty one.", gearConfigDir.c_str());

        try {
            fs::create_directory(fs::path(gearConfigDir));
        }
        catch (std::exception& ex) {
            logger.Write(FATAL, "Directory [%s] couldn't be created, exception: %s.",
                gearConfigDir.c_str(),
                ex.what());
            logger.Write(FATAL, "Directory [%s] couldn't be created, skipping config loading.", gearConfigDir.c_str());
            return;
        }
    }

    for (const auto& p : fs::directory_iterator(gearConfigDir)) {
        if (p.path().extension() == ".xml") {
            GearInfo info = GearInfo::ParseConfig(p.path().string());
            if (!info.ParseError) {
                info.Path = p.path().string();
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
        if (config.MarkedForDeletion) {
            if (!std::filesystem::remove(config.Path)) {
                logger.Write(ERROR, "Failed to remove file %s", config.Path.c_str());
                error++;
            }
            else {
                logger.Write(DEBUG, "Removed file %s", config.Path.c_str());
                deleted++;
            }
        }
    }
    if (error) {
        UI::Notify(INFO, fmt::format("Failed to remove {} gear config(s).", error));
    }
    if (settings.AutoNotify && deleted) {
        UI::Notify(INFO, fmt::format("Removed {} gear config(s).", deleted));
    }
}

void update_player() {
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    if (ENTITY::DOES_ENTITY_EXIST(currentVehicle) && currentVehicle != previousVehicle) {
        previousVehicle = currentVehicle;

        if (std::find_if(currentConfigs.begin(), currentConfigs.end(), [=](const auto& cfg) {return cfg.first == currentVehicle; }) == currentConfigs.end()) {
            currentConfigs.emplace_back(currentVehicle, GearInfo("noconfig",
                "noconfig",
                0,
                "noconfig",
                ext.GetTopGear(currentVehicle),
                ext.GetDriveMaxFlatVel(currentVehicle),
                ext.GetGearRatios(currentVehicle),
                LoadType::None)
            );
            logger.Write(DEBUG, "[Management] Appended new vehicle: 0x%X", currentVehicle);
        }

        for (const auto& config : gearConfigs) {
            bool sameModel = GAMEPLAY::GET_HASH_KEY(config.ModelName.c_str()) == ENTITY::GET_ENTITY_MODEL(currentVehicle);
            if (!sameModel)
                sameModel = config.ModelHash == ENTITY::GET_ENTITY_MODEL(currentVehicle);
            bool samePlate = StrUtil::to_lower(config.LicensePlate) == StrUtil::to_lower(VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(currentVehicle));

            switch (config.LoadType) {
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
        bool topGearChanged = ext.GetTopGear(vehicle) != config.TopGear;
        bool driveMaxVelChanged = ext.GetDriveMaxFlatVel(vehicle) != config.DriveMaxVel;
        bool anyRatioChanged = false;
        if (!topGearChanged && !driveMaxVelChanged) {
            auto extRatios = ext.GetGearRatios(vehicle);
            for (uint32_t i = 0; i < config.TopGear; ++i) {
                if (extRatios[i] != config.Ratios[i]) {
                    anyRatioChanged = true;
                    break;
                }
            }
        }

        if (topGearChanged || driveMaxVelChanged || anyRatioChanged) {
            ext.SetTopGear(vehicle, config.TopGear);
            ext.SetDriveMaxFlatVel(vehicle, config.DriveMaxVel);
            ext.SetInitialDriveMaxFlatVel(vehicle, config.DriveMaxVel / 1.2f);
            ext.SetGearRatios(vehicle, config.Ratios);
            if (settings.AutoNotify) {
                UI::Notify(INFO, fmt::format("Restored {}: \n"
                    "Top gear = {}\n"
                    "Top speed = {:.0f} kph", vehicle, config.TopGear,
                    3.6f * config.DriveMaxVel / config.Ratios[config.TopGear]));
            }
        }
    }
}

void main() {
    logger.Write(INFO, "Script started");
    absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + Constants::ModDir;
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