#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "TorqueScript.hpp"
#include "Constants.hpp"

#include "ScriptMenuUtils.hpp"

#include "Util/UI.hpp"
#include "Util/Math.hpp"

#include <fmt/format.h>

namespace CustomTorque {
    std::vector<std::string> FormatTorqueConfig(CTorqueScript& context, const CConfig& config);
    bool PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);
}

std::vector<CScriptMenu<CTorqueScript>::CSubmenu> CustomTorque::BuildMenu() {
    std::vector<CScriptMenu<CTorqueScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Torque Map");
        mbCtx.Subtitle(std::string("~b~") + Constants::DisplayVersion);

        Ped playerPed = PLAYER::PLAYER_PED_ID();
        Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

        if (!playerVehicle || !ENTITY::DOES_ENTITY_EXIST(playerVehicle)) {
            mbCtx.Option("No vehicle", { "Get in a vehicle to change its gear stats." });
            mbCtx.MenuOption("Developer options", "developermenu");
            return;
        }

        // activeConfig can always be assumed if in any vehicle.
        CConfig* activeConfig = context.ActiveConfig();

        std::string torqueExtraTitle;
        std::vector<std::string> extra;

        torqueExtraTitle = activeConfig->Name;
        extra = FormatTorqueConfig(context, *activeConfig);

        mbCtx.OptionPlus("Torque map info", extra, nullptr, nullptr, nullptr, torqueExtraTitle);

        mbCtx.MenuOption("Edit configuration", "editconfigmenu",
            { "Enter to edit the current configuration." });

        if (mbCtx.MenuOption("Load configuration", "loadmenu",
            { "Load another configuration into the current config." })) {
            CustomTorque::LoadConfigs();
        }

        mbCtx.MenuOption("Save configuration", "savemenu",
            { "Save the current configuration to disk." });

        mbCtx.MenuOption("Developer options", "developermenu");
        });

    /* mainmenu -> editconfigmenu */
    submenus.emplace_back("editconfigmenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Config edit");
        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(config ? config->Name : "None");

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        // TODO: Config editing

        });

    /* mainmenu -> loadmenu */
    submenus.emplace_back("loadmenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Load configurations");

        CConfig* config = context.ActiveConfig();
        mbCtx.Subtitle(fmt::format("Current: ", config ? config->Name : "None"));

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        if (CustomTorque::GetConfigs().empty()) {
            mbCtx.Option("No saved ratios");
        }

        for (const auto& config : CustomTorque::GetConfigs()) {
            bool selected;
            bool triggered = mbCtx.OptionPlus(config.Name, {}, &selected);

            if (selected) {
                mbCtx.OptionPlusPlus(FormatTorqueConfig(context, config));
            }

            if (triggered) {
                context.ApplyConfig(config);
                UI::Notify(fmt::format("Applied config {}.", config.Name), true);
            }
        }
        });

    /* mainmenu -> savemenu */
    submenus.emplace_back("savemenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Save configuration");
        mbCtx.Subtitle("");
        auto* config = context.ActiveConfig();

        if (config == nullptr) {
            mbCtx.Option("No active configuration");
            return;
        }

        Hash model = ENTITY::GET_ENTITY_MODEL(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false));
        const char* plate = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false));

        if (mbCtx.Option("Save",
            { "Save the current configuration to the current active configuration.",
              fmt::format("Current active configuration: {}.", context.ActiveConfig()->Name) })) {
            config->Write(CConfig::ESaveType::GenericNone); // Don't write model/plate
            CustomTorque::LoadConfigs();
            UI::Notify("Saved changes", true);
        }

        if (mbCtx.Option("Save as specific vehicle",
            { "Save current configuration for the current vehicle model and license plate.",
               "Automatically loads for vehicles of this model with this license plate." })) {
            if (PromptSave(context, *config, model, plate, CConfig::ESaveType::Specific))
                CustomTorque::LoadConfigs();
        }

        if (mbCtx.Option("Save as generic vehicle",
            { "Save current configuration for the current vehicle model."
                "Automatically loads for any vehicle of this model.",
                "Overridden by license plate config, if present." })) {
            if (PromptSave(context, *config, model, std::string(), CConfig::ESaveType::GenericModel))
                CustomTorque::LoadConfigs();
        }

        if (mbCtx.Option("Save as generic",
            { "Save current configuration, but don't make it automatically load for any vehicle." })) {
            if (PromptSave(context, *config, 0, std::string(), CConfig::ESaveType::GenericNone))
                CustomTorque::LoadConfigs();
        }
        });

    /* mainmenu -> developermenu */
    submenus.emplace_back("developermenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Developer options");
        mbCtx.Subtitle("");

        mbCtx.Option(fmt::format("NPC instances: {}", CustomTorque::GetNPCScriptCount()),
            { "CustomTorqueMap works for all NPC vehicles.",
              "This is the number of vehicles the script is working for." });
        mbCtx.BoolOption("NPC Details", CustomTorque::GetSettings().Debug.NPCDetails);
        });

    return submenus;
}

std::vector<std::string> CustomTorque::FormatTorqueConfig(CTorqueScript& context, const CConfig& config) {
    std::vector<std::string> extras{
        fmt::format("Name: {}", config.Name),
        fmt::format("Model: {}", config.ModelName.empty() ? "None (Generic)" : config.ModelName),
        fmt::format("Plate: {}", config.Plate.empty() ? "None" : fmt::format("[{}]", config.Plate)),
    };

    // TODO: Draw the torque map

    return extras;
}

bool CustomTorque::PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType) {
    UI::Notify("Enter new config name.", true);
    std::string newName = UI::GetKeyboardResult();

    if (newName.empty()) {
        UI::Notify("No config name entered. Not saving anything.", true);
        return false;
    }

    if (config.Write(newName, model, plate, saveType))
        UI::Notify("Saved as new configuration", true);
    else
        UI::Notify("Failed to save as new configuration", true);
    CustomTorque::LoadConfigs();

    return true;
}
