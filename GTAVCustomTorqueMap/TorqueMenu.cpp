#include "ScriptMenu.hpp"

#include "Constants.hpp"
#include "PerformanceLog.hpp"
#include "Script.hpp"
#include "TorqueUI.hpp"
#include "TorqueUtil.hpp"
#include "TorqueScript.hpp"

#include "ScriptMenuUtils.hpp"

#include "Util/UI.hpp"
#include "Util/Math.hpp"
#include "Util/ScriptUtils.h"

#include <fmt/format.h>
#include <optional>

namespace CustomTorque {
    std::vector<std::string> FormatTorqueConfig(CTorqueScript& context, const CConfig& config);
    std::vector<std::string> FormatTorqueLive(const STorqueData& torqueData);

    bool PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);

    const std::vector<std::string> MeasurementTypes { "Relative", "Metric", "Imperial" };
}

std::vector<CScriptMenu<CTorqueScript>::CSubmenu> CustomTorque::BuildMenu() {
    std::vector<CScriptMenu<CTorqueScript>::CSubmenu> submenus;
    /* mainmenu */
    submenus.emplace_back("mainmenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Custom Torque Map");
        mbCtx.Subtitle(std::string("~b~") + Constants::DisplayVersion);

        Ped playerPed = PLAYER::PLAYER_PED_ID();
        Vehicle playerVehicle = PED::GET_VEHICLE_PED_IS_IN(playerPed, false);

        if (!playerVehicle || !ENTITY::DOES_ENTITY_EXIST(playerVehicle)) {
            mbCtx.Option("No vehicle", { "Get in a vehicle." });
            mbCtx.MenuOption("UI settings", "uimenu");
            mbCtx.MenuOption("Developer settings", "developermenu");
            return;
        }

        // activeConfig can always be assumed if in any vehicle.
        CConfig* activeConfig = context.ActiveConfig();

        if (!activeConfig) {
            mbCtx.Option("Fatal error?",
                { "This vehicle has no config at all?" });
        }
        else {
            std::string torqueExtraTitle;
            std::vector<std::string> extra;
            std::vector<std::string> details;

            torqueExtraTitle = activeConfig->Name;

            auto torqueData = GetTorqueData(context, *activeConfig);

            extra = FormatTorqueLive(torqueData);

            details = FormatTorqueConfig(context, *activeConfig);
            details.insert(details.begin(), "Raw engine map, without boost or other effects applied.");

            bool showTorqueMap = false;
            mbCtx.OptionPlus("Torque map info", extra, &showTorqueMap, nullptr, nullptr, torqueExtraTitle, details);
            if (showTorqueMap) {
                DrawCurve(context, *activeConfig, playerVehicle);
            }
        }

        if (mbCtx.MenuOption("Load configuration", "loadmenu",
            { "Load another configuration into the current config." })) {
            CustomTorque::LoadConfigs();
        }

        auto recordState = PerformanceLog::GetState();
        std::string stateText;

        switch (recordState) {
            case PerformanceLog::LogState::Idle:
                stateText = "Idle";
                break;
            case PerformanceLog::LogState::Waiting:
                stateText = "Waiting";
                break;
            case PerformanceLog::LogState::Recording:
                stateText = "Recording";
                break;
            case PerformanceLog::LogState::Finished:
                stateText = "Finished";
                break;
            default:
                stateText = "Invalid";
                break;
        }

        std::vector<std::string> datalogDetails{
            "Record performance data to a csv file to analyze.",
            "Use your favourite plotting software to visualize the data."
        };

        std::vector<std::string> datalogExplain{
            "Make sure the current status is 'Idle'",
            "1. Be at or below idle RPM (in gear)",
            "   - Speed less than idle RPM speed",
            "2. Press this option to start",
            "   - Status goes to 'Waiting'",
            "3. Hit full throttle",
            "   - Status goes go 'Recording'",
            "4. Keep on full throttle until rev limiter",
            "5. Output saves as timestamp-model.csv",
            "   - Status returns to 'Idle'",
            "",
            "Releasing throttle also saves. Make sure assists such as traction control are turned off.",
        };

        bool dataLogToggled = mbCtx.OptionPlus(
            fmt::format("Datalogging: {}", stateText),
            datalogExplain,
            nullptr,
            nullptr,
            nullptr,
            "Instructions",
            datalogDetails);

        if (dataLogToggled) {
            if (recordState == PerformanceLog::LogState::Idle) {
                PerformanceLog::Start();
            }
        }

        mbCtx.MenuOption("UI settings", "uimenu");
        mbCtx.MenuOption("Developer settings", "developermenu");
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
            mbCtx.Option("No saved configs");
        }

        for (const auto& config : CustomTorque::GetConfigs()) {
            bool selected;
            bool triggered = mbCtx.OptionPlus(config.Name, {}, &selected);

            if (selected) {
                mbCtx.OptionPlusPlus(FormatTorqueConfig(context, config), config.Name);
                DrawCurve(context, config, 0);
            }

            if (triggered) {
                context.ApplyConfig(config);
                UI::Notify(fmt::format("Applied config {}.", config.Name), true);
            }
        }
    });

    /* mainmenu -> uimenu */
    submenus.emplace_back("uimenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("UI settings");
        mbCtx.Subtitle("");

        mbCtx.StringArray("Measurement", MeasurementTypes, CustomTorque::GetSettings().UI.Measurement,
            { "Change visualization and summary units.",
              "Relative: Values as ratio of the max output.",
              "Metric: Use kW and N-m.",
              "Imperial: Use hp and lb-ft." });

        mbCtx.BoolOption("Enable tachometer", CustomTorque::GetSettings().UI.Tachometer.Enable,
            { "Display a custom tachometer that represents the actual power output." });

        const std::vector<std::string> graphPosInfo{
            "Move tachometer position.",
            "Use arrow keys or select to enter a value."
        };

        mbCtx.FloatOptionCb("Horizontal position", CustomTorque::GetSettings().UI.Tachometer.X, 0.0f, 1.0f, 0.01f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("Vertical position", CustomTorque::GetSettings().UI.Tachometer.Y, 0.0f, 1.0f, 0.01f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("Width", CustomTorque::GetSettings().UI.Tachometer.W, 0.0f, 1.0f, 0.01f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("Height", CustomTorque::GetSettings().UI.Tachometer.H, 0.0f, 1.0f, 0.01f,
            MenuUtils::GetKbFloat, graphPosInfo);
    });

    /* mainmenu -> developermenu */
    submenus.emplace_back("developermenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Developer settings");
        mbCtx.Subtitle("");

        // For reference/editing coords.
        if (CustomTorque::GetConfigs().size() > 0)
            DrawCurve(context, CustomTorque::GetConfigs()[0], 0);

        const std::vector<std::string> graphPosInfo {
            "Move graph position.",
            "Use arrow keys or select to enter a value."
        };

        mbCtx.FloatOptionCb("UI.TorqueGraphX", CustomTorque::GetSettings().UI.TorqueGraph.X, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphY", CustomTorque::GetSettings().UI.TorqueGraph.Y, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphW", CustomTorque::GetSettings().UI.TorqueGraph.W, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphH", CustomTorque::GetSettings().UI.TorqueGraph.H, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);

        mbCtx.BoolOption("Debug Info", CustomTorque::GetSettings().Debug.DisplayInfo,
            { "Displays a window with status info for each affected vehicle." });

        std::vector<std::string> npcDetails = {
            "The script can also apply engine mappings for applicable NPC vehicles.",
            "If this impacts performance, turn this off."
        };

        if (mbCtx.BoolOption("Enable for NPCs", CustomTorque::GetSettings().Main.EnableNPC, npcDetails)) {
            if (!CustomTorque::GetSettings().Main.EnableNPC) {
                CustomTorque::ClearNPCScripts();
            }
        }

        mbCtx.Option(fmt::format("NPC instances: {}", CustomTorque::GetNPCScriptCount()),
            { "Number of vehicles the script is active on." });
    });

    return submenus;
}

std::vector<std::string> CustomTorque::FormatTorqueConfig(CTorqueScript& context, const CConfig& config) {
    std::vector<std::string> extras{
        fmt::format("Name: {}", config.Name),
        fmt::format("Model: {}", config.ModelName.empty() ? "None (Generic)" : config.ModelName),
        fmt::format("Plate: {}", config.Plate.empty() ? "None" : fmt::format("[{}]", config.Plate)),
    };

    if (config.Data.IdleRPM != 0 && config.Data.RevLimitRPM != 0) {
        extras.push_back(fmt::format("Idle @ {} RPM, Limit: {} RPM", config.Data.IdleRPM, config.Data.RevLimitRPM));

        auto vehicle = context.GetVehicle();
        if (ENTITY::DOES_ENTITY_EXIST(vehicle) && ENTITY::GET_ENTITY_MODEL(vehicle) == config.ModelHash) {
            float maxTorqueNm = Util::GetHandlingTorqueNm(vehicle);
            float maxTorqueRPM = map(config.Data.Peak.TorqueRPM, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

            float maxPowerkW = config.Data.Peak.Power * maxTorqueNm;
            float maxPowerRPM = map(config.Data.Peak.PowerRPM, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

            if (CustomTorque::GetSettings().UI.Measurement != 2) {
                extras.push_back(fmt::format("Peak power: {:.0f} kW @ {:.0f} RPM",
                    maxPowerkW, maxPowerRPM));
                extras.push_back(fmt::format("Peak torque: {:.0f} N-m @ {:.0f} RPM",
                    maxTorqueNm, maxTorqueRPM));
            }
            else {
                extras.push_back(fmt::format("Peak power: {:.0f} hp @ {:.0f} RPM",
                    kW2hp(maxPowerkW), maxPowerRPM));
                extras.push_back(fmt::format("Peak torque: {:.0f} lb-ft @ {:.0f} RPM",
                    Nm2lbft(maxTorqueNm), maxTorqueRPM));
            }
        }
    }

    return extras;
}

std::vector<std::string> CustomTorque::FormatTorqueLive(const STorqueData& torqueData) {
    std::string rawMapTorque;
    std::string actualOutput;

    if (CustomTorque::GetSettings().UI.Measurement != 2) {
        rawMapTorque = fmt::format("Mapped: {:3.0f} N-m", torqueData.RawMapForceNm);
        if (torqueData.RPMData != std::nullopt)
            actualOutput = fmt::format("Output: {:3.0f} kW / {:3.0f} N-m", torqueData.RPMData->PowerkW, torqueData.TotalForceNm);
        else
            actualOutput = fmt::format("Output: {:3.0f} N-m", torqueData.TotalForceNm);
    }
    else {
        rawMapTorque = fmt::format("Mapped: {:3.0f} lb-ft", torqueData.RawMapForceNm * 0.737562f);
        if (torqueData.RPMData != std::nullopt)
            actualOutput = fmt::format("Output: {:3.0f} hp / {:3.0f} lb-ft", torqueData.RPMData->PowerHP, torqueData.TotalForceLbFt);
        else
            actualOutput = fmt::format("Output: {:3.0f} lb-ft", torqueData.TotalForceLbFt);
    }

    std::vector<std::string> extras;
    if (torqueData.RPMData != std::nullopt) {
        extras = {
            fmt::format("RPM: {:3.0f} ({:.2f})", torqueData.RPMData->RealRPM, torqueData.NormalizedRPM),
            fmt::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
            rawMapTorque,
            actualOutput,
        };
    }
    else {
        extras = {
            fmt::format("RPM: {:.2f}", torqueData.NormalizedRPM),
            fmt::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
            rawMapTorque,
            actualOutput,
            "Horsepower not calculated: Add IdleRPM and RevLimitRPM in [Data] in the engine map file for RPM-derived horsepower."
        };
    }

    extras.push_back("Performance upgrades and scripts may affect final output.");

    if (CustomTorque::GetSettings().UI.Measurement != 0)
        extras.push_back("Performance visualization is only based on handling.meta power.");

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
