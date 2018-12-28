#include "scriptMenu.h"

#include <inc/natives.h>
#include <menu.h>

#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"
#include "../../GTAVManualTransmission/Gears/Util/StringFormat.h"

#include "script.h"
#include "Names.h"

extern NativeMenu::Menu menu;

extern Vehicle currentVehicle;
extern VehicleExtensions ext;

void incRatio(float& ratio, float max, float step) {
    if (ratio + step > max) return;
    ratio += step;
}

void decRatio(float& ratio, float min, float step) {
    if (ratio - step < min) return;
    ratio -= step;
}

std::vector<std::string> printGearStatus(Vehicle vehicle, uint8_t tunedGear) {
    uint8_t topGear = ext.GetTopGear(vehicle);
    uint16_t currentGear = ext.GetGearCurr(vehicle);
    float maxVel = ext.GetDriveMaxFlatVel(vehicle);
    auto ratios = ext.GetGearRatios(vehicle);

    std::vector<std::string> lines = {
        fmt("Top gear: %d", topGear),
        fmt("Current gear: %d", currentGear),
        "",
        "Gear ratios:",
    };

    for (uint8_t i = 1; i <= topGear; ++i) {
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
        lines.push_back(fmt("%s%s: %.02f (rev limit: %.0f kph)%s", i == tunedGear ? "~b~" : "",
            prefix.c_str(), ratios[i], 3.6f * maxVel / ratios[i], i == tunedGear ? "" : ""));
    }

    return lines;
}

void update_mainmenu() {
    menu.Title("Custom Gear Ratios");
    menu.Subtitle(std::string("~b~") + DISPLAY_VERSION);

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        menu.MenuOption("Options", "optionsmenu", { "Change some preferences." });
        return;
    }

    auto extra = printGearStatus(currentVehicle, 0);
    menu.OptionPlus("Gearbox status", extra, nullptr, nullptr, nullptr, getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle)));

    menu.MenuOption("Edit ratios", "ratiomenu");
    menu.MenuOption("Load ratios", "loadmenu");
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

    uint8_t topGear = ext.GetTopGear(currentVehicle);
    std::string carName = getFmtModelName(ENTITY::GET_ENTITY_MODEL(currentVehicle));
    for (uint8_t gear = 1; gear <= topGear; ++gear) {
        bool sel = false;
        auto extra = std::vector<std::string>();
        menu.OptionPlus(fmt("Gear %d", gear), extra, &sel, 
            [=] { incRatio(*reinterpret_cast<float*>(ext.GetGearRatiosAddress(currentVehicle) + gear * sizeof(float)), 10.0f, 0.01f); },
            [=] { decRatio(*reinterpret_cast<float*>(ext.GetGearRatiosAddress(currentVehicle) + gear * sizeof(float)), 0.01f, 0.01f); },
            carName, { "Press left to decrease gear ratio, right to increase gear ratio." });
        if (sel) {
            extra = printGearStatus(currentVehicle, gear);
            menu.OptionPlusPlus(extra, carName);
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

}

void update_savemenu() {
    menu.Title("Save ratios");
    menu.Subtitle("");

    if (!currentVehicle || !ENTITY::DOES_ENTITY_EXIST(currentVehicle)) {
        menu.Option("No vehicle", { "Get in a vehicle to change its gear stuff." });
        return;
    }
}

void update_optionsmenu() {
    menu.Title("Options");
    menu.Subtitle("");

}

void update_menu() {
    menu.CheckKeys();

    /* mainmenu */
    if (menu.CurrentMenu("mainmenu")) { update_mainmenu(); }

    if (menu.CurrentMenu("ratiomenu")) { update_ratiomenu(); }

    if (menu.CurrentMenu("loadmenu")) { update_loadmenu(); }

    if (menu.CurrentMenu("savemenu")) { update_savemenu(); }

    if (menu.CurrentMenu("savemenu")) { update_savemenu(); }
    
    if (menu.CurrentMenu("optionsmenu")) { update_optionsmenu(); }

    menu.EndMenu();
}