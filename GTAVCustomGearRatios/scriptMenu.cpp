#include "scriptMenu.h"

#include <filesystem>

#include <inc/natives.h>
#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Util/StringFormat.h"

#include "Names.h"
#include "script.h"
#include "scriptSettings.h"
#include "gearInfo.h"
#include "../../GTAVManualTransmission/Gears/Util/UIUtils.h"

extern NativeMenu::Menu menu;
extern ScriptSettings settings;

extern Vehicle currentVehicle;
extern VehicleExtensions ext;

extern std::string gearConfigDir;

extern std::vector<GearInfo> gearConfigs;

std::string stripInvalidChars(std::string s, char replace) {
    std::string illegalChars = "\\/:?\"<>|";
    for (auto it = s.begin(); it < s.end(); ++it) {
        if (illegalChars.find(*it) != std::string::npos) {
            *it = replace;
        }
    }
    return s;
}

template <typename T>
void incVal(T& val, const T max, const T step) {
    if (val + step > max) return;
    val += step;
}

template <typename T>
void decVal(T& val, const T min, const T step) {
    if (val - step < min) return;
    val -= step;
}

void applyConfig(const GearInfo& config, Vehicle vehicle) {
    ext.SetTopGear(vehicle, config.mTopGear);
    ext.SetDriveMaxFlatVel(vehicle, config.mDriveMaxVel);
    ext.SetInitialDriveMaxFlatVel(vehicle, config.mDriveMaxVel / 1.2f);
    ext.SetGearRatios(vehicle, config.mRatios);
}

std::vector<std::string> printInfo(const GearInfo& info) {
    uint8_t topGear = info.mTopGear;
    auto ratios = info.mRatios;
    //float maxVel = (fInitialDriveMaxFlatVel * 1.2f) / 0.9f;
    float maxVel = info.mDriveMaxVel;

    std::vector<std::string> lines = {
        info.mDescription,
        fmt("For: %s", info.mModelName.c_str()),
        fmt("Top gear: %d", topGear),
        "",
        "Gear ratios:",
    };

    for (uint8_t i = 0; i <= topGear; ++i) {
        std::string prefix;
        if (i == 0) {
            prefix = "Reverse";
        }
        else if (i == 1) {
            prefix = "1st";
        }
        else if (i == 2) {
            prefix = "2nd";
        }
        else if (i == 3) {
            prefix = "3rd";
        }
        else {
            prefix = fmt("%dth", i);
        }
        lines.push_back(fmt("%s: %.02f (rev limit: %.0f kph)",
            prefix.c_str(), ratios[i], 3.6f * maxVel / ratios[i]));
    }

    return lines;
}

std::vector<std::string> printGearStatus(Vehicle vehicle, uint8_t tunedGear) {
    uint8_t topGear = ext.GetTopGear(vehicle);
    uint16_t currentGear = ext.GetGearCurr(vehicle);
    float maxVel = ext.GetDriveMaxFlatVel(vehicle);
    auto ratios = ext.GetGearRatios(vehicle);

    std::vector<std::string> lines = {
        fmt("Top gear: %d", topGear),
        fmt("Final drive: %.01f kph", maxVel * 3.6f),
        fmt("Current gear: %d", currentGear),
        "",
        "Gear ratios:",
    };

    for (uint8_t i = 0; i <= topGear; ++i) {
        std::string prefix;
        if (i == 0) {
            prefix = "Reverse";
        }
        else if (i == 1) {
            prefix = "1st";
        }
        else if (i == 2) {
            prefix = "2nd";
        }
        else if (i == 3) {
            prefix = "3rd";
        }
        else {
            prefix = fmt("%dth", i);
        }
        lines.push_back(fmt("%s%s: %.02f (rev limit: %.0f kph)", i == tunedGear ? "~b~" : "",
            prefix.c_str(), ratios[i], 3.6f * maxVel / ratios[i]));
    }

    return lines;
}

void promptSave(Vehicle vehicle) {
    std::string saveFile;
    std::string description;
    // TODO: Add-on spawner model name
    std::string modelName = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicle));
    std::string licensePlate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicle);
    uint8_t topGear = ext.GetTopGear(vehicle);
    float driveMaxVel = ext.GetDriveMaxFlatVel(vehicle);
    std::vector<float> ratios = ext.GetGearRatios(vehicle);

    showNotification("Enter description");
    GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(0, "FMMC_KEY_TIP8", "", "", "", "", "", 64);
    while (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 0) WAIT(0);
    if (!GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT()) {
        showNotification("Cancelled save");
        return;
    }

    description = GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT();
    if (description.empty()) {
        showNotification("No description entered, using default");
        std::string carName = getFmtModelName(ENTITY::GET_ENTITY_MODEL(vehicle));
        description = fmt("%s - %d gears - %.0f kph",
            carName.c_str(), topGear,
            3.6f * driveMaxVel / ratios[topGear]);
    }

    std::string saveFileBase = stripInvalidChars(description, '_');
    saveFile = saveFileBase;
    uint32_t saveFileSuffix = 0;
    bool duplicate;
    do {
        duplicate = false;
        for (auto & p : std::filesystem::directory_iterator(gearConfigDir)) {
            if (p.path().stem() == saveFile) {
                duplicate = true;
                saveFile = fmt("%s_%02d", saveFileBase.c_str(), saveFileSuffix++);
            }
        }
    } while (duplicate);

    GearInfo(description, modelName, licensePlate, 
        topGear, driveMaxVel, ratios).SaveConfig(gearConfigDir + "\\" + saveFile + ".xml");
    showNotification(fmt("Saved as %s", saveFile));
}

void update_mainmenu() {
    menu.Title("Custom Gear Ratios");
    menu.Subtitle(std::string("~b~") + DISPLAY_VERSION);

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        menu.MenuOption("Options", "optionsmenu", { "Change some preferences." });
        return;
    }

    auto extra = printGearStatus(currentVehicle, 255);
    menu.OptionPlus("Gearbox status", extra, nullptr, nullptr, nullptr, getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle)));

    menu.MenuOption("Edit ratios", "ratiomenu");
    if (menu.MenuOption("Load ratios", "loadmenu")) {
        parseConfigs();
    }
    menu.MenuOption("Save ratios", "savemenu");

    menu.MenuOption("Options", "optionsmenu", { "Change some preferences." });
}

void update_ratiomenu() {
    menu.Title("Edit ratios");
    menu.Subtitle("");

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        return;
    }

    std::string carName = getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle));

    // Change top gear
    {
        bool sel;
        uint8_t topGear = ext.GetTopGear(currentVehicle);
        menu.OptionPlus(fmt("Top gear: < %d >", topGear), {}, &sel,
            [=]() mutable { 
                incVal<uint8_t>(topGear, 10, 1);
                ext.SetTopGear(currentVehicle, topGear); 
            },
            [=]() mutable {
                decVal<uint8_t>(topGear,  1, 1); 
                ext.SetTopGear(currentVehicle, topGear);
            },
            carName, 
            { "Press left to decrease top gear, right to increase top gear.",
               "~r~Warning: The default gearbox can't shift down from 9th gear!" });
        if (sel) {
            menu.OptionPlusPlus(printGearStatus(currentVehicle, 255), carName);
        }
    }

    // Change final drive
    {
        bool sel;
        float driveMaxVel = ext.GetDriveMaxFlatVel(currentVehicle);
        menu.OptionPlus(fmt("Final drive max: < %.01f kph >", driveMaxVel * 3.6f), {}, &sel,
            [=]() mutable {
                incVal<float>(driveMaxVel, 500.0f, 0.36f); 
                ext.SetDriveMaxFlatVel(currentVehicle, driveMaxVel);
                ext.SetInitialDriveMaxFlatVel(currentVehicle, driveMaxVel / 1.2f);
            },
            [=]() mutable {
                decVal<float>(driveMaxVel, 1.0f, 0.36f); 
                ext.SetDriveMaxFlatVel(currentVehicle, driveMaxVel);
                ext.SetInitialDriveMaxFlatVel(currentVehicle, driveMaxVel / 1.2f);
            },
            carName, { "Press left to decrease final drive max velocity, right to increase it." });
        if (sel) {
            menu.OptionPlusPlus(printGearStatus(currentVehicle, 255), carName);
        }
    }

    uint8_t topGear = ext.GetTopGear(currentVehicle);
    for (uint8_t gear = 0; gear <= topGear; ++gear) {
        bool sel = false;
        float min = 0.01f;
        float max = 10.0f;

        if (gear == 0) {
            min = -10.0f;
            max = -0.01f;
        }

        menu.OptionPlus(fmt("Gear %d", gear), {}, &sel,
                        [=] { incVal(*reinterpret_cast<float*>(ext.GetGearRatioPtr(currentVehicle, gear)), max, 0.01f); },
                        [=] { decVal(*reinterpret_cast<float*>(ext.GetGearRatioPtr(currentVehicle, gear)), min, 0.01f); },
                        carName, { "Press left to decrease gear ratio, right to increase gear ratio." });
        if (sel) {
            menu.OptionPlusPlus(printGearStatus(currentVehicle, gear), carName);
        }
    }
}

void update_loadmenu() {
    menu.Title("Load ratios");
    menu.Subtitle("");

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        return;
    }

    for (const auto& config : gearConfigs) {
        bool selected;
        std::string modelName = getFmtModelName(
            GAMEPLAY::GET_HASH_KEY((char*)config.mModelName.c_str()));
        std::string optionName = fmt("%s - %d gears - %.0f kph", 
            modelName.c_str(), config.mTopGear, 
            3.6f * config.mDriveMaxVel / config.mRatios[config.mTopGear]);
        if (menu.OptionPlus(optionName, std::vector<std::string>(), &selected)) {
            applyConfig(config, currentVehicle);
            showNotification(fmt("[%s] applied to current %s", 
                config.mDescription.c_str(), getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle)).c_str()));
        }
        if (selected) {
            menu.OptionPlusPlus(printInfo(config), modelName);
        }
    }

}

void update_savemenu() {
    menu.Title("Save ratios");
    menu.Subtitle("");

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        return;
    }

    if (menu.Option("Save as autoload", 
        { "Save current gear setup for model and license plate.",
        "It will load automatically when entering a car "
            "with the same model and plate text."})) {
        promptSave(currentVehicle);
    }

    if (menu.Option("Save as generic",
        { "Save current gear setup without autoload." })) {
        promptSave(currentVehicle);
    }
}

void update_optionsmenu() {
    menu.Title("Options");
    menu.Subtitle("");

    menu.BoolOption("Load ratios automatically", settings.AutoLoad,
        { "Load gear ratio mapping automatically when vehicle"
            " matches model and license plate." });
    menu.BoolOption("Fix ratios automatically", settings.AutoFix,
        { "Fix ratios of the 7th gear and higher when applied by the game." });
}

void update_menu() {
    menu.CheckKeys();

    /* mainmenu */
    if (menu.CurrentMenu("mainmenu")) { update_mainmenu(); }

    if (menu.CurrentMenu("ratiomenu")) { update_ratiomenu(); }

    if (menu.CurrentMenu("loadmenu")) { update_loadmenu(); }

    if (menu.CurrentMenu("savemenu")) { update_savemenu(); }
    
    if (menu.CurrentMenu("optionsmenu")) { update_optionsmenu(); }

    menu.EndMenu();
}