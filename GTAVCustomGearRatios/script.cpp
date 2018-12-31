#include "script.h"

#include <filesystem>

#include <inc/main.h>
#include <inc/natives.h>

#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/Paths.h"
#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Util/StringFormat.h"
#include "../../GTAVManualTransmission/Gears/Util/UIUtils.h"

#include "scriptSettings.h"
#include "scriptMenu.h"
#include "gearInfo.h"
#include "Names.h"
#include "StringUtils.h"

uint8_t g_numGears = 8;

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

void applyConfig(const GearInfo& config, Vehicle vehicle);

void parseConfigs() {
    gearConfigs.clear();
    for (auto & p : std::filesystem::directory_iterator(gearConfigDir)) {
        if (p.path().extension() == ".xml") {
            GearInfo info = GearInfo::ParseConfig(p.path().string());
            if (!info.mParseError) {
                gearConfigs.push_back(info);
            }
            else {
                logger.Write(ERROR, "%s skipped due to errors", p.path().stem().string().c_str());
            }
        }
    }
}

void update_player() {
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);

    if (ENTITY::DOES_ENTITY_EXIST(currentVehicle) && currentVehicle != previousVehicle) {
        previousVehicle = currentVehicle;
        if (settings.AutoLoad) {
            for(const auto& config : gearConfigs) {
                bool sameModel = GAMEPLAY::GET_HASH_KEY((char*)config.mModelName.c_str()) == ENTITY::GET_ENTITY_MODEL(currentVehicle);
                bool samePlate = to_lower(config.mLicensePlate) == to_lower(VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(currentVehicle));
                if (sameModel && samePlate) {
                    applyConfig(config, currentVehicle);
                    showNotification(fmt("[%s] automatically applied to current %s",
                        config.mDescription.c_str(), getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle)).c_str()));
                }
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
    ext.initOffsets();
    parseConfigs();

    menu.RegisterOnMain([=] {
        menu.ReadSettings();
        settings.Read();
        logger.SetMinLevel(settings.Debug ? DEBUG : INFO);
        parseConfigs();
    });

    menu.RegisterOnExit([=] {
        settings.Save();
    });

    while (true) {
        update_player();
        update_menu();
        WAIT(0);
    }
}

void ScriptMain() {
    srand(GetTickCount());
    main();
}