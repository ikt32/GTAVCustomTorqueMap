#include "script.h"

#include <inc/main.h>
#include <inc/natives.h>

#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/Paths.h"
#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"

#include "scriptSettings.h"
#include "scriptMenu.h"

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

void update_player() {
    currentVehicle = PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false);
}

void main() {
    logger.Write(INFO, "Script started");
    absoluteModPath = Paths::GetModuleFolder(Paths::GetOurModuleHandle()) + MOD_DIRECTORY;
    settingsGeneralFile = absoluteModPath + "\\settings_general.ini";
    settingsMenuFile = absoluteModPath + "\\settings_menu.ini";
    menu.SetFiles(settingsMenuFile);

    //settings.Read(&carControls);
    //if (settings.LogLevel > 4) settings.LogLevel = 1;
    logger.SetMinLevel(LogLevel::DEBUG);
    menu.ReadSettings();

    ext.initOffsets();

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