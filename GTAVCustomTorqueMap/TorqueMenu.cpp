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
#include "Util/String.hpp"

#include <format>
#include <optional>

namespace CustomTorque {
    std::vector<std::string> FormatTorqueConfig(CTorqueScript& context, const CConfig& config);
    std::vector<std::string> FormatTorqueLive(const STorqueData& torqueData);

    bool PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);

    const std::vector<std::string> sMeasurementTypes { "Relative", "Metric", "Imperial" };

    std::optional<SDrawPerformanceValues> cachedPerfValues;
}

void CustomTorque::InvalidateCachedTorqueGraphData() {
    cachedPerfValues.reset();
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
            cachedPerfValues.reset();
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

            auto torqueData = GetTorqueData(context.GetVehicle(), *activeConfig);

            extra = FormatTorqueLive(torqueData);

            details = FormatTorqueConfig(context, *activeConfig);
            details.insert(details.begin(), "Raw engine map, without boost or other effects applied.");
            if (Util::strcmpwi(activeConfig->Name, "Default")) {
                details.push_back("Default map: Due to the game, the torque drop only exists in 2nd gear and up.");
            }

            bool showTorqueMap = false;
            bool triggered =
                mbCtx.OptionPlus("Torque map info", extra, &showTorqueMap, nullptr, nullptr, torqueExtraTitle, details);
            if (showTorqueMap && cachedPerfValues) {
                DrawCurve(*activeConfig, playerVehicle, cachedPerfValues.value());
            }
            if (!cachedPerfValues || triggered) {
                cachedPerfValues = GenerateCurveData(*activeConfig, playerVehicle);
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
            std::format("Datalogging: {}", stateText),
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
        mbCtx.Subtitle(std::format("Current: ", config ? config->Name : "None"));

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
                DrawCurve(config, 0, GenerateCurveData(config, 0));
            }

            if (triggered) {
                context.ApplyConfig(config);
                UI::Notify(std::format("Applied config {}.", config.Name), true);
            }
        }
    });

    /* mainmenu -> uimenu */
    submenus.emplace_back("uimenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("UI settings");
        mbCtx.Subtitle("");

        bool triggered = mbCtx.StringArray("Measurement", sMeasurementTypes, CustomTorque::GetSettings().UI.Measurement,
            { "Change visualization and summary units.",
              "Relative: Values as ratio of the max output.",
              "Metric: Use kW and N-m.",
              "Imperial: Use hp and lb-ft." });
        if (triggered)
            InvalidateCachedTorqueGraphData();

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
            DrawCurve(CustomTorque::GetConfigs()[0], 0, GenerateCurveData(CustomTorque::GetConfigs()[0], 0));

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

        mbCtx.Option(std::format("NPC instances: {}", CustomTorque::GetNPCScriptCount()),
            { "Number of vehicles the script is active on." });
    });

    return submenus;
}

std::vector<std::string> CustomTorque::FormatTorqueConfig(CTorqueScript& context, const CConfig& config) {
    std::vector<std::string> extras{
        std::format("Name: {}", config.Name),
        std::format("Model: {}", config.ModelName.empty() ? "None (Generic)" : config.ModelName),
        std::format("Plate: {}", config.Plate.empty() ? "None" : std::format("[{}]", config.Plate)),
    };

    if (config.Data.IdleRPM != 0 && config.Data.RevLimitRPM != 0) {
        extras.push_back(std::format("Idle @ {} RPM, Limit: {} RPM", config.Data.IdleRPM, config.Data.RevLimitRPM));

        auto vehicle = context.GetVehicle();
        if (ENTITY::DOES_ENTITY_EXIST(vehicle) && ENTITY::GET_ENTITY_MODEL(vehicle) == config.ModelHash) {
            float maxTorqueNm = Util::GetHandlingTorqueNm(vehicle);
            float maxTorqueRPM = map(config.Data.Peak.TorqueRPM, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

            float maxPowerRPM = map(config.Data.Peak.PowerRPM, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);
            float maxPowerkW = (0.10471975512f * config.Data.Peak.PowerTorque * maxTorqueNm * maxPowerRPM) / 1000.0f;

            if (CustomTorque::GetSettings().UI.Measurement != 2) {
                extras.push_back(std::format("Peak power: {:.0f} kW @ {:.0f} RPM",
                    maxPowerkW, maxPowerRPM));
                extras.push_back(std::format("Peak torque: {:.0f} N-m @ {:.0f} RPM",
                    maxTorqueNm, maxTorqueRPM));
            }
            else {
                extras.push_back(std::format("Peak power: {:.0f} hp @ {:.0f} RPM",
                    kW2hp(maxPowerkW), maxPowerRPM));
                extras.push_back(std::format("Peak torque: {:.0f} lb-ft @ {:.0f} RPM",
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
        rawMapTorque = std::format("Mapped: {:3.0f} N-m", torqueData.RawMapForceNm);
        if (torqueData.RPMData != std::nullopt)
            actualOutput = std::format("Output: {:3.0f} kW / {:3.0f} N-m", torqueData.RPMData->PowerkW, torqueData.TotalForceNm);
        else
            actualOutput = std::format("Output: {:3.0f} N-m", torqueData.TotalForceNm);
    }
    else {
        rawMapTorque = std::format("Mapped: {:3.0f} lb-ft", torqueData.RawMapForceNm * 0.737562f);
        if (torqueData.RPMData != std::nullopt)
            actualOutput = std::format("Output: {:3.0f} hp / {:3.0f} lb-ft", torqueData.RPMData->PowerHP, torqueData.TotalForceLbFt);
        else
            actualOutput = std::format("Output: {:3.0f} lb-ft", torqueData.TotalForceLbFt);
    }

    std::vector<std::string> extras;
    if (torqueData.RPMData != std::nullopt) {
        extras = {
            std::format("RPM: {:3.0f} ({:.2f})", torqueData.RPMData->RealRPM, torqueData.NormalizedRPM),
            std::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
            rawMapTorque,
            actualOutput,
        };
    }
    else {
        extras = {
            std::format("RPM: {:.2f}", torqueData.NormalizedRPM),
            std::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
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
