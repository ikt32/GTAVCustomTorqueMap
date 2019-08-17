#include "scriptMenu.h"

#include <filesystem>
#include <fmt/core.h>
#include <inc/natives.h>
#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Memory/Offsets.hpp"
#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/UIUtils.h"

#include "Names.h"
#include "script.h"
#include "scriptSettings.h"
#include "gearInfo.h"


extern NativeMenu::Menu menu;
extern ScriptSettings settings;

extern Vehicle currentVehicle;
extern VehicleExtensions ext;

extern std::string gearConfigDir;

extern std::vector<GearInfo> gearConfigs;
extern std::vector<std::pair<Vehicle, GearInfo>> currentConfigs;


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

void applyConfig(const GearInfo& config, Vehicle vehicle, bool notify) {
    ext.SetTopGear(vehicle, config.mTopGear);
    ext.SetDriveMaxFlatVel(vehicle, config.mDriveMaxVel);
    ext.SetInitialDriveMaxFlatVel(vehicle, config.mDriveMaxVel / 1.2f);
    ext.SetGearRatios(vehicle, config.mRatios);
    if (notify) {
        showNotification(fmt::format("[{}] applied to current {}",
            config.mDescription.c_str(), getFmtModelName(ENTITY::GET_ENTITY_MODEL(vehicle)).c_str()));
    }

    auto currCfgCombo = std::find_if(currentConfigs.begin(), currentConfigs.end(), [=](const auto& cfg) {return cfg.first == vehicle; });

    if (currCfgCombo != currentConfigs.end()) {
        auto& currentConfig = currCfgCombo->second;
        currentConfig.mTopGear = config.mTopGear;
        currentConfig.mDriveMaxVel = config.mDriveMaxVel;
        currentConfig.mRatios = config.mRatios;
    }
    else {
        logger.Write(DEBUG, "[Management] 0x%X not found?", vehicle);
    }

}

std::vector<std::string> printInfo(const GearInfo& info) {
    uint8_t topGear = info.mTopGear;
    auto ratios = info.mRatios;
    //float maxVel = (fInitialDriveMaxFlatVel * 1.2f) / 0.9f;
    float maxVel = info.mDriveMaxVel;

    std::string loadType;
    switch (info.mLoadType) {
        case LoadType::Plate: loadType = "Plate"; break;
        case LoadType::Model: loadType = "Model"; break;
        case LoadType::None: loadType = "None"; break;
    }

    std::vector<std::string> lines = {
        info.mDescription,
        fmt::format("For: {}", info.mModelName.c_str()),
        fmt::format("Plate: {}", info.mLoadType == LoadType::Plate ? info.mLicensePlate.c_str() : "Any"),
        fmt::format("Load type: {}", loadType.c_str()),
        fmt::format("Top gear: {}", topGear),
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
            prefix = fmt::format("{}th", i);
        }
        lines.push_back(fmt::format("{}: {:.2f} (rev limit: {:.0f} kph)",
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
        fmt::format("Top gear: {}", topGear),
        fmt::format("Final drive: {:.1f} kph", maxVel * 3.6f),
        fmt::format("Current gear: {}", currentGear),
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
            prefix = fmt::format("{}th", i);
        }
        lines.push_back(fmt::format("{}{}: {:.2f} (rev limit: {:.0f} kph)", i == tunedGear ? "~b~" : "",
            prefix.c_str(), ratios[i], 3.6f * maxVel / ratios[i]));
    }

    return lines;
}

void promptSave(Vehicle vehicle, LoadType loadType) {
    uint8_t topGear = ext.GetTopGear(vehicle);
    float driveMaxVel = ext.GetDriveMaxFlatVel(vehicle);
    std::vector<float> ratios = ext.GetGearRatios(vehicle);

    // TODO: Add-on spawner model name
    std::string modelName = VEHICLE::GET_DISPLAY_NAME_FROM_VEHICLE_MODEL(ENTITY::GET_ENTITY_MODEL(vehicle));

    std::string saveFileProto = fmt::format("{}_{}_{:.0f}kph", modelName.c_str(), topGear,
        3.6f * driveMaxVel / ratios[topGear]);
    std::string saveFileBase;
    std::string saveFile;

    std::string licensePlate;
    switch (loadType) {
        case LoadType::Plate:   licensePlate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(vehicle); break;
        case LoadType::Model:   licensePlate = LoadName::Model;  break;
        case LoadType::None:    licensePlate = LoadName::None; break;
    }

    showNotification("Enter description");
    WAIT(0);
    GAMEPLAY::DISPLAY_ONSCREEN_KEYBOARD(UNK::_GET_CURRENT_LANGUAGE_ID() == 0, "FMMC_KEY_TIP8", "", "", "", "", "", 64);
    while (GAMEPLAY::UPDATE_ONSCREEN_KEYBOARD() == 0) WAIT(0);
    if (!GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT()) {
        showNotification("Cancelled save");
        return;
    }

    std::string description = GAMEPLAY::GET_ONSCREEN_KEYBOARD_RESULT();
    if (description.empty()) {
        showNotification("No description entered, using default");
        std::string carName = getFmtModelName(ENTITY::GET_ENTITY_MODEL(vehicle));
        description = fmt::format("{} - {} gears - {:.0f} kph",
            carName.c_str(), topGear,
            3.6f * driveMaxVel / ratios[topGear]);
        saveFileBase = stripInvalidChars(fmt::format("{}_nameless", saveFileProto.c_str()), '_');
    }
    else {
        saveFileBase = stripInvalidChars(fmt::format("{}_{}", saveFileProto.c_str(), description.c_str()), '_');
    }

    uint32_t saveFileSuffix = 0;
    saveFile = saveFileBase;
    bool duplicate;
    do {
        duplicate = false;
        for (auto & p : std::filesystem::directory_iterator(gearConfigDir)) {
            if (p.path().stem() == saveFile) {
                duplicate = true;
                saveFile = fmt::format("{}_{:02d}", saveFileBase.c_str(), saveFileSuffix++);
            }
        }
    } while (duplicate);

    GearInfo(description, modelName, licensePlate, 
        topGear, driveMaxVel, ratios, loadType).SaveConfig(gearConfigDir + "\\" + saveFile + ".xml");
    showNotification(fmt::format("Saved as {}", saveFile));
}

void update_mainmenu() {
    menu.Title("Custom Gear Ratios");
    menu.Subtitle(std::string("~b~") + DISPLAY_VERSION);

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
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
    bool anyChanged = false;

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
        return;
    }

    uint8_t origNumGears = *reinterpret_cast<uint8_t *>(ext.GetHandlingPtr(currentVehicle) + hOffsets.nInitialDriveGears);
    uint32_t flags = *reinterpret_cast<uint32_t *>(ext.GetHandlingPtr(currentVehicle) + 0x128); // handling flags, b1604
    bool cvtFlag = flags & 0x00001000;

    std::string carName = getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle));

    // Change top gear
    if (cvtFlag) {
        menu.Option("Vehicle has CVT, can't be edited.", { "Get in a vehicle to change its gear stats." });
    }
    else {
        bool sel;
        uint8_t topGear = ext.GetTopGear(currentVehicle);
        menu.OptionPlus(fmt::format("Top gear: < {} >", topGear), {}, &sel,
            [&]() mutable { 
                incVal<uint8_t>(topGear, g_numGears - 1, 1);
                ext.SetTopGear(currentVehicle, topGear);
                anyChanged = true;
            },
            [&]() mutable {
                decVal<uint8_t>(topGear,  1, 1); 
                ext.SetTopGear(currentVehicle, topGear);
                anyChanged = true;
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
        menu.OptionPlus(fmt::format("Final drive max: < {:.1f} kph >", driveMaxVel * 3.6f), {}, &sel,
            [&]() mutable {
                incVal<float>(driveMaxVel, 500.0f, 0.36f); 
                ext.SetDriveMaxFlatVel(currentVehicle, driveMaxVel);
                ext.SetInitialDriveMaxFlatVel(currentVehicle, driveMaxVel / 1.2f);
                anyChanged = true;
            },
            [&]() mutable {
                decVal<float>(driveMaxVel, 1.0f, 0.36f); 
                ext.SetDriveMaxFlatVel(currentVehicle, driveMaxVel);
                ext.SetInitialDriveMaxFlatVel(currentVehicle, driveMaxVel / 1.2f);
                anyChanged = true;
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

        menu.OptionPlus(fmt::format("Gear {}", gear), {}, &sel,
            [&]() mutable {
                incVal(*reinterpret_cast<float*>(ext.GetGearRatioPtr(currentVehicle, gear)), max, 0.01f);
                anyChanged = true;
            },
            [&]() mutable {
                decVal(*reinterpret_cast<float*>(ext.GetGearRatioPtr(currentVehicle, gear)), min, 0.01f);
                anyChanged = true;
            },
            carName, { "Press left to decrease gear ratio, right to increase gear ratio." });
        if (sel) {
            menu.OptionPlusPlus(printGearStatus(currentVehicle, gear), carName);
        }
    }

    if (anyChanged) {
        auto currCfgCombo = std::find_if(currentConfigs.begin(), currentConfigs.end(), [=](const auto& cfg) {return cfg.first == currentVehicle; });
        
        if (currCfgCombo != currentConfigs.end()) {
            auto& currentConfig = currCfgCombo->second;
            currentConfig.mTopGear = ext.GetTopGear(currentVehicle);
            currentConfig.mDriveMaxVel = ext.GetDriveMaxFlatVel(currentVehicle);
            currentConfig.mRatios = ext.GetGearRatios(currentVehicle);
        }
        else {
            showNotification("CGR: Something messed up, check log.");
            logger.Write(ERROR, "Could not find currvehicle {} in list of vehicles?", currentVehicle);
        }
    }
}

void update_loadmenu() {
    menu.Title("Load ratios");
    menu.Subtitle("");

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
        return;
    }

    if (gearConfigs.empty()) {
        menu.Option("No saved ratios");
    }


    for (auto& config : gearConfigs) {
        bool selected;
        std::string modelName = getFmtModelName(
            GAMEPLAY::GET_HASH_KEY((char*)config.mModelName.c_str()));
        std::string optionName = fmt::format("{} - {} gears - {:.0f} kph", 
            modelName.c_str(), config.mTopGear, 
            3.6f * config.mDriveMaxVel / config.mRatios[config.mTopGear]);
        
        std::vector<std::string> extras = { "Press Enter/Accept to load." };

        if (config.mMarkedForDeletion) {
            extras.emplace_back("~r~Marked for deletion. ~s~Press Right again to restore."
                " File will be removed on menu exit!");
            optionName = fmt::format("~r~{}", optionName.c_str());
        }
        else {
            extras.emplace_back("Press Right to mark for deletion.");
        }

        if (menu.OptionPlus(optionName, std::vector<std::string>(), &selected,
                [&]() mutable { config.mMarkedForDeletion = !config.mMarkedForDeletion; },
                nullptr, modelName, extras)) {
            applyConfig(config, currentVehicle, true);
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
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
        return;
    }

    if (menu.Option("Save as autoload", 
        { "Save current gear setup for model and license plate.",
        "It will load automatically when entering a car "
            "with the same model and plate text."})) {
        promptSave(currentVehicle, LoadType::Plate);
    }

    if (menu.Option("Save as generic autoload",
        { "Save current gear setup with generic autoload."
            "Overridden by plate-specific autoload." })) {
        promptSave(currentVehicle, LoadType::Model);
    }

    if (menu.Option("Save as generic",
        { "Save current gear setup without autoload." })) {
        promptSave(currentVehicle, LoadType::None);
    }
}

void update_optionsmenu() {
    menu.Title("Options");
    menu.Subtitle("");

    menu.BoolOption("Autoload ratios (License plate)", settings.AutoLoad,
        { "Load gear ratio mapping automatically when getting into a vehicle"
            " that matches model and license plate." });
    menu.BoolOption("Autoload ratios (Generic)", settings.AutoLoadGeneric,
        { "Load gear ratio mapping automatically when getting into a vehicle"
            " that matches model. Overridden by plate." });
    menu.BoolOption("Override game ratio changes", settings.RestoreRatios,
        { "Restores user-set ratios when the game changes them,"
            " for example gearbox upgrades in LSC." });
    menu.BoolOption("Autoload notifications", settings.AutoNotify,
        { "Show a notification when autoload applied a preset." });
    menu.BoolOption("Enable CVT when 1 gear", settings.EnableCVT,
        { "Enable CVT simulation when settings numGears to 1 in a non-CVT vehicle." });

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