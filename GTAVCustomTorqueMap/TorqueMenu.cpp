#include "ScriptMenu.hpp"
#include "Script.hpp"
#include "TorqueScript.hpp"
#include "Constants.hpp"

#include "ScriptMenuUtils.hpp"

#include "Util/UI.hpp"
#include "Util/Math.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Memory/Offsets.hpp"

#include <fmt/format.h>

using VExt = VehicleExtensions;

namespace CustomTorque {
    std::vector<std::string> FormatTorqueConfig(CTorqueScript& context, const CConfig& config);
    std::vector<std::string> FormatTorqueLive(CTorqueScript& context, const CConfig& config);

    bool PromptSave(CTorqueScript& context, CConfig& config, Hash model, std::string plate, CConfig::ESaveType saveType);
    void ShowCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle);
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
            mbCtx.Option("No vehicle", { "Get in a vehicle to change its torque map." });
            mbCtx.MenuOption("Developer options", "developermenu");
            return;
        }

        // activeConfig can always be assumed if in any vehicle.
        CConfig* activeConfig = context.ActiveConfig();

        std::string torqueExtraTitle;
        std::vector<std::string> extra;
        std::vector<std::string> details;

        torqueExtraTitle = activeConfig->Name;
        extra = FormatTorqueLive(context, *activeConfig);
        
        details = FormatTorqueConfig(context, *activeConfig);
        details.insert(details.begin(), "Raw engine map, without boost or other effects applied.");

        bool showTorqueMap = false;
        mbCtx.OptionPlus("Torque map info", extra, &showTorqueMap, nullptr, nullptr, torqueExtraTitle, details);
        if (showTorqueMap) {
            ShowCurve(context, *activeConfig, playerVehicle);
        }

        // No config editing possible here.
        // mbCtx.MenuOption("Edit configuration", "editconfigmenu",
        //     { "Enter to edit the current configuration." });

        if (mbCtx.MenuOption("Load configuration", "loadmenu",
            { "Load another configuration into the current config." })) {
            CustomTorque::LoadConfigs();
        }

        // No editing = no saving
        // mbCtx.MenuOption("Save configuration", "savemenu",
        //     { "Save the current configuration to disk." });

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

        mbCtx.BoolOption("Debug Info", CustomTorque::GetSettings().Debug.DisplayInfo,
            { "Displays a window with status info for each affected vehicle." });

        if (mbCtx.BoolOption("Enable for NPCs", CustomTorque::GetSettings().Main.EnableNPC)) {
            if (!CustomTorque::GetSettings().Main.EnableNPC) {
                CustomTorque::ClearNPCScripts();
            }
        }

        mbCtx.Option(fmt::format("NPC instances: {}", CustomTorque::GetNPCScriptCount()),
            { "CustomTorqueMap works for all NPC vehicles.",
              "This is the number of vehicles the script is working for." });
        });

    return submenus;
}

std::vector<std::string> CustomTorque::FormatTorqueConfig(CTorqueScript& context, const CConfig& config) {
    std::vector<std::string> extras{
        fmt::format("Name: {}", config.Name),
        fmt::format("Model: {}", config.ModelName.empty() ? "None (Generic)" : config.ModelName),
        fmt::format("Plate: {}", config.Plate.empty() ? "None" : fmt::format("[{}]", config.Plate)),
    };

    return extras;
}

std::vector<std::string> CustomTorque::FormatTorqueLive(CTorqueScript& context, const CConfig& config) {
    float rpm = VExt::GetCurrentRPM(context.GetVehicle());
    float mapMultiplier = CTorqueScript::GetScaledValue(config.Data.TorqueMultMap, rpm);
    
    auto forces = VExt::GetWheelPower(context.GetVehicle());
    float totalForce = 0.0f;

    for (uint32_t i = 0; i < forces.size(); ++i) {
        totalForce += forces[i];
    }

    auto handlingPtr = VExt::GetHandlingPtr(context.GetVehicle());
    float weight = *reinterpret_cast<float*>(handlingPtr + hOffsets1604.fMass);

    float gearRatio = VExt::GetGearRatios(context.GetVehicle())[VExt::GetGearCurr(context.GetVehicle())];
    float totalForceNm = (totalForce * weight) / gearRatio;

    std::vector<std::string> extras;
    
    if (config.Data.IdleRPM != 0 && config.Data.RevLimitRPM != 0) {
        float realRPM = map(rpm, 0.2f, 1.0f,
            (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

        float horsepower = (totalForceNm * realRPM) / 5252.0f;

        extras = {
            fmt::format("RPM: {:.0f} ({:.2f})", realRPM, rpm),
            fmt::format("Torque mult: {:.2f}x", mapMultiplier),
            fmt::format("Wheel output: {:.3f} ({:.0f} HP / {:.2f} Nm)", totalForce, horsepower, totalForceNm),
        };
    }
    else {
        extras = {
            fmt::format("RPM: {:.2f}", rpm),
            fmt::format("Torque mult: {:.2f}x", mapMultiplier),
            fmt::format("Wheel output: {:.3f} ({:.2f} Nm)", totalForce, totalForceNm),
        };
    }

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
        float y = CTorqueScript::GetScaledValue(config.Data.TorqueMultMap, x);
        points.push_back({ x, y, i >= idleRange });
    }

    float rectX = 0.5f;
    float rectY = 0.5f;
    float rectW = 0.66f / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float rectH = 0.40f;
    float blockW = rectW / maxSamples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::_GET_ASPECT_RATIO(FALSE);

    float vertAxisLabelX = rectX - 0.5f * rectW - 0.05f;
    float vertAxisLabelY = rectY;
    UI::ShowText(vertAxisLabelX, vertAxisLabelY, 0.25f, "Torque\nMult");

    float vertAxisValueX = rectX - 0.5f * rectW - 0.025f;
    float vertAxis0Y = rectY + 0.5f * rectH;
    UI::ShowText(vertAxisValueX, vertAxis0Y, 0.25f, "0.0x");

    float vertAxis1Y = rectY - 0.5f * rectH;
    UI::ShowText(vertAxisValueX, vertAxis1Y, 0.25f, "1.0x");

    float horAxisLabelX = rectX;
    float horAxisLabelY = rectY + 0.5f * rectH + 0.05f;
    UI::ShowText(horAxisLabelX, horAxisLabelY, 0.25f, "Normalized RPM");

    float horAxis0_0X = rectX - 0.5f * rectW;
    float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
    UI::ShowText(horAxis0_0X, horAxisValueY, 0.25f, "0.0");

    float horAxis0_2X = rectX - 0.5f * rectW + 0.2f * rectW;
    UI::ShowText(horAxis0_2X, horAxisValueY, 0.25f, "0.2\nIdle");

    float horAxis1_0X = rectX + 0.5f * rectW;
    UI::ShowText(horAxis1_0X, horAxisValueY, 0.25f, "1.0");

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

    GRAPHICS::DRAW_RECT(rectX, rectY,
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 239, 0);

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
            CTorqueScript::GetScaledValue(config.Data.TorqueMultMap, input)
        };

        float pointX = rectX - 0.5f * rectW + currentPoint.first * rectW;
        float pointY = rectY + 0.5f * rectH - currentPoint.second * rectH;
        GRAPHICS::DRAW_RECT(pointX, pointY,
            3.0f * blockW, 3.0f * blockH,
            255, 0, 0, 255, 0);
    }
}
