#include "script.h"

#include <filesystem>

#include <inc/main.h>
#include <inc/natives.h>

#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/Paths.h"
#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Util/StringFormat.h"

#include "scriptSettings.h"
#include "scriptMenu.h"
#include "gearInfo.h"

uint8_t g_numGears;

std::string absoluteModPath;
std::string settingsGeneralFile;
std::string settingsWheelFile;
std::string settingsStickFile;
std::string settingsMenuFile;

NativeMenu::Menu menu;

ScriptSettings settings;
VehicleExtensions ext;

Vehicle currentVehicle;

std::string gearConfigDir;
std::vector<GearInfo> gearConfigs;

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
}

void main() {
    logger.SetMinLevel(DEBUG);
    ext.initOffsets();

    logger.Write(INFO, "Script started");
    absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + MOD_DIRECTORY;
    settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    gearConfigDir = absoluteModPath + "\\Configs";
    menu.SetFiles(settingsMenuFile);

    settings.Read();
    menu.ReadSettings();

    menu.RegisterOnMain([=] {
        menu.ReadSettings();
        settings.Read();
        parseConfigs();
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