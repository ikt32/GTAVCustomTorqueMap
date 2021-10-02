#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "TorqueScript.hpp"
#include "Constants.hpp"
#include "TorqueUtil.hpp"
#include "PerformanceLog.hpp"

#include "ScriptMenuUtils.hpp"

#include "Util/UI.hpp"
#include "Util/Math.hpp"

#include "Memory/VehicleExtensions.hpp"

#include <fmt/format.h>
#include <optional>

using VExt = VehicleExtensions;

namespace CustomTorque {
    std::vector<std::string> FormatTorqueConfig(CTorqueScript& context, const CConfig& config);

    std::vector<std::string> FormatTorqueLive(const STorqueData& torqueData);
    void ShowCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle);

    bool PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);
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
            mbCtx.MenuOption("Developer options", "developermenu");
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
                ShowCurve(context, *activeConfig, playerVehicle);
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
            "   - Speed less than idle RPM speed"
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

        mbCtx.MenuOption("Developer options", "developermenu");
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
                mbCtx.OptionPlusPlus(FormatTorqueConfig(context, config));
                ShowCurve(context, config, 0);
            }

            if (triggered) {
                context.ApplyConfig(config);
                UI::Notify(fmt::format("Applied config {}.", config.Name), true);
            }
        }
    });

    /* mainmenu -> developermenu */
    submenus.emplace_back("developermenu", [](NativeMenu::Menu& mbCtx, CTorqueScript& context) {
        mbCtx.Title("Developer options");
        mbCtx.Subtitle("");

        // For reference/editing coords.
        if (CustomTorque::GetConfigs().size() > 0)
            ShowCurve(context, CustomTorque::GetConfigs()[0], 0);

        const std::vector<std::string> graphPosInfo {
            "Move graph position.",
            "Use arrow keys or select to enter a value."
        };

        mbCtx.FloatOptionCb("UI.TorqueGraphX", CustomTorque::GetSettings().UI.TorqueGraphX, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphY", CustomTorque::GetSettings().UI.TorqueGraphY, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphW", CustomTorque::GetSettings().UI.TorqueGraphW, 0.0f, 1.0f, 0.05f,
            MenuUtils::GetKbFloat, graphPosInfo);
        mbCtx.FloatOptionCb("UI.TorqueGraphH", CustomTorque::GetSettings().UI.TorqueGraphH, 0.0f, 1.0f, 0.05f,
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
        extras.push_back(fmt::format("Idle: {} RPM", config.Data.IdleRPM));
        extras.push_back(fmt::format("Rev limit: {} RPM", config.Data.RevLimitRPM));
    }

    return extras;
}

std::vector<std::string> CustomTorque::FormatTorqueLive(const STorqueData& torqueData) {
    std::vector<std::string> extras;
    
    if (torqueData.RPMData != std::nullopt) {
        extras = {
            fmt::format("RPM: {:3.0f} ({:.2f})", torqueData.RPMData->RealRPM, torqueData.NormalizedRPM),
            fmt::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
            fmt::format("Mapped: {:3.0f} Nm", torqueData.RawMapForceNm),
            fmt::format("Output: {:3.0f} HP / {:3.0f} lb-ft", torqueData.RPMData->PowerHP, torqueData.TotalForceLbFt),
            fmt::format("Output: {:3.0f} kW / {:3.0f} Nm", torqueData.RPMData->PowerkW, torqueData.TotalForceNm),
        };
    }
    else {
        extras = {
            fmt::format("RPM: {:.2f}", torqueData.NormalizedRPM),
            fmt::format("Torque mult: {:.2f}x", torqueData.TorqueMult),
            fmt::format("Mapped: {:3.0f} Nm", torqueData.RawMapForceNm),
            fmt::format("Output: {:3.0f} lb-ft", torqueData.TotalForceLbFt),
            fmt::format("Output: {:3.0f} Nm", torqueData.TotalForceNm),
            "For RPM-derived data, add IdleRPM and RevLimitRPM in [Data] in the engine map file."
        };
    }

    extras.push_back("Performance upgrades and scripts may affect final output.");

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

void CustomTorque::ShowCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle) {
    struct Point {
        float x;
        float y;
        bool solid;
    };

    const int maxSamples = 100;
    const int idleRange = 20; // 0.2 of maxSamples

    std::vector<Point> points;
    for (int i = 0; i < maxSamples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(maxSamples);
        float y = CustomTorque::GetScaledValue(config.Data.TorqueMultMap, x);
        points.push_back({ x, y, i >= idleRange });
    }

    float rectX = CustomTorque::GetSettings().UI.TorqueGraphX;
    float rectY = CustomTorque::GetSettings().UI.TorqueGraphY;
    float rectW = CustomTorque::GetSettings().UI.TorqueGraphW / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float rectH = CustomTorque::GetSettings().UI.TorqueGraphH;
    float blockW = rectW / maxSamples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::_GET_ASPECT_RATIO(FALSE);

    float vertAxisLabelX = rectX - 0.5f * rectW - 0.05f;
    float vertAxisLabelY = rectY;
    UI::ShowText(vertAxisLabelX, vertAxisLabelY, 0.25f, "Torque\nMult");

    float vertAxisValueX = rectX - 0.5f * rectW - 0.025f;
    float vertAxis0Y = rectY + 0.5f * rectH;
    UI::ShowText(vertAxisValueX, vertAxis0Y, 0.25f, "0.00x");

    float vertAxis1Y = rectY - 0.5f * rectH;
    UI::ShowText(vertAxisValueX, vertAxis1Y, 0.25f, "1.00x");

    float horAxisLabelX = rectX;
    float horAxisLabelY = rectY + 0.5f * rectH + 0.05f;
    UI::ShowText(horAxisLabelX, horAxisLabelY, 0.25f, "Normalized RPM");

    float horAxis0_0X = rectX - 0.5f * rectW;
    float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
    UI::ShowText(horAxis0_0X, horAxisValueY, 0.25f, "0.00");

    float horAxis0_2X = rectX - 0.5f * rectW + 0.2f * rectW;
    UI::ShowText(horAxis0_2X, horAxisValueY, 0.25f, "0.20\n(Idle)");

    float horAxis1_0X = rectX + 0.5f * rectW;
    UI::ShowText(horAxis1_0X, horAxisValueY, 0.25f, "1.00");

    // Left
    GRAPHICS::DRAW_RECT(
        rectX - rectW * 0.5f - blockW * 0.5f,
        rectY,
        blockW,
        rectH + 2.0f * blockH,
        255, 255, 255, 255, 0);

    // Right
    GRAPHICS::DRAW_RECT(
        rectX + rectW * 0.5f + blockW * 0.5f,
        rectY,
        blockW,
        rectH + 2.0f * blockH,
        255, 255, 255, 255, 0);

    // Top
    GRAPHICS::DRAW_RECT(
        rectX,
        rectY - rectH * 0.5f - blockH * 0.5f,
        rectW + 2.0f * blockW,
        blockH,
        255, 255, 255, 255, 0);

    // Bottom
    GRAPHICS::DRAW_RECT(
        rectX,
        rectY + rectH * 0.5f + blockH * 0.5f,
        rectW + 2.0f * blockW,
        blockH,
        255, 255, 255, 255, 0);

    // Background
    GRAPHICS::DRAW_RECT(rectX, rectY,
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 191, 0);

    for (auto point : points) {
        float pointX = rectX - 0.5f * rectW + point.x * rectW;
        float pointY = rectY + 0.5f * rectH - point.y * rectH;
        GRAPHICS::DRAW_RECT(pointX, pointY,
            blockW, blockH,
            255,
            255,
            255,
            point.solid ? 255 : 91,
            0);
    }

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        float input = VExt::GetCurrentRPM(vehicle);
        std::pair<float, float> currentPoint = {
            input,
            CustomTorque::GetScaledValue(config.Data.TorqueMultMap, input)
        };

        float pointX = rectX - 0.5f * rectW + currentPoint.first * rectW;
        float pointY = rectY + 0.5f * rectH - currentPoint.second * rectH;

        // RPM
        float horAxisValueX = pointX;
        float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
        UI::ShowText(horAxisValueX, horAxisValueY, 0.25f, fmt::format("{:1.2f}", input));

        // Mult
        float vertAxisValueX = rectX - 0.5f * rectW - 0.025f;
        float vertAxisValueY = pointY;
        UI::ShowText(vertAxisValueX, vertAxisValueY, 0.25f, fmt::format("{:1.2f}x", currentPoint.second));

        GRAPHICS::DRAW_RECT(pointX, pointY,
            1.5f * blockW, 1.5f * blockH,
            255, 0, 0, 255, 0);
    }
}
