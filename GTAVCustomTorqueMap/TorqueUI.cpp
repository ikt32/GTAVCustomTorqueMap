#include "TorqueUI.hpp"

#include "Compatibility.hpp"
#include "Script.hpp"
#include "TorqueUtil.hpp"

#include "Memory/VehicleExtensions.hpp"
#include "Util/Color.h"
#include "Util/Math.hpp"
#include "Util/UI.hpp"
#include "Util/ScriptUtils.h"
#include "Util/String.hpp"

#include <inc/natives.h>
#include <fmt/format.h>

using VExt = VehicleExtensions;

void CustomTorque::DrawCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle) {
    struct SPoint {
        float x;
        float y;
        bool solid;
    };

    const int maxSamples = 100;
    const int idleRange = 20; // 0.2 of maxSamples

    std::vector<SPoint> points;
    for (int i = 0; i < maxSamples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(maxSamples);
        float y = CustomTorque::GetScaledValue(config.Data.TorqueMultMap, x);
        points.push_back({ x, y, i >= idleRange });
    }

    float rectX = CustomTorque::GetSettings().UI.TorqueGraph.X;
    float rectY = CustomTorque::GetSettings().UI.TorqueGraph.Y;
    float rectW = CustomTorque::GetSettings().UI.TorqueGraph.W / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float rectH = CustomTorque::GetSettings().UI.TorqueGraph.H;
    float blockW = rectW / maxSamples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::GET_ASPECT_RATIO(FALSE);

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
    GRAPHICS::DRAW_RECT({ rectX - rectW * 0.5f - blockW * 0.5f, rectY },
        blockW,
        rectH + 2.0f * blockH,
        255, 255, 255, 255, 0);

    // Right
    GRAPHICS::DRAW_RECT({ rectX + rectW * 0.5f + blockW * 0.5f, rectY },
        blockW,
        rectH + 2.0f * blockH,
        255, 255, 255, 255, 0);

    // Top
    GRAPHICS::DRAW_RECT({ rectX, rectY - rectH * 0.5f - blockH * 0.5f },
        rectW + 2.0f * blockW,
        blockH,
        255, 255, 255, 255, 0);

    // Bottom
    GRAPHICS::DRAW_RECT({ rectX, rectY + rectH * 0.5f + blockH * 0.5f },
        rectW + 2.0f * blockW,
        blockH,
        255, 255, 255, 255, 0);

    // Background
    GRAPHICS::DRAW_RECT({ rectX, rectY },
        rectW + blockW / 2.0f, rectH + blockH / 2.0f,
        0, 0, 0, 191, 0);

    for (auto point : points) {
        float pointX = rectX - 0.5f * rectW + point.x * rectW;
        float pointY = rectY + 0.5f * rectH - point.y * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
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

        GRAPHICS::DRAW_RECT({ pointX, pointY },
            1.5f * blockW, 1.5f * blockH,
            255, 0, 0, 255, 0);
    }
}

void CustomTorque::DrawTachometer(CTorqueScript& context, Vehicle vehicle) {
    const int barsPer1kRpm = 5;

    const auto& uiCfg = CustomTorque::GetSettings().UI;
    auto* activeConfig = context.ActiveConfig();
    if (!activeConfig) {
        // config should always exist, so something's gone not right
        return;
    }
    const auto& cfg = *activeConfig;

    int lowRpm = cfg.Data.IdleRPM;
    int maxRpm = cfg.Data.RevLimitRPM;

    if (lowRpm == 0)
        lowRpm = 800;
    if (maxRpm == 0)
        maxRpm = 8000;

    int redRpm = cfg.Data.RedlineRPM > 0 ?
        cfg.Data.RedlineRPM :
        static_cast<int>(static_cast<float>(maxRpm) * 0.85f);

    const int numTachoBars = (maxRpm * barsPer1kRpm) / 1000;

    float internalRpm = VExt::GetCurrentRPM(vehicle);

    // NA: Use scale as-is
    // Turbo: Scale / maxTurboDispMod
    float maxBoost = 0.0f;
    float mapScaleFactorTurbo = 1.0f;

    if (Util::VehicleHasTurboMod(vehicle)) {
        if (TurboFix::Active()) {
            maxBoost = TurboFix::GetAbsoluteBoostMax();
        }
        else {
            maxBoost = 1.0f;
        }

        float maxPowerMult = 1.0f + (maxBoost / 10.0f);
        mapScaleFactorTurbo = 1.0f / maxPowerMult;
    }

    float turboValue = VExt::GetTurbo(vehicle);
    float turboDispMod = 1.0f + std::max(0.0f, turboValue) / 10.0f;

    int realRpm = static_cast<int>(
        map(internalRpm, 0.2f, 1.0f,
        static_cast<float>(lowRpm),
        static_cast<float>(maxRpm))
    );
    realRpm = std::max(0, realRpm);
    if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle)) {
        realRpm = 0;
    }

    GRAPHICS::DRAW_RECT({ uiCfg.Tachometer.X, uiCfg.Tachometer.Y },
        uiCfg.Tachometer.W, uiCfg.Tachometer.H,
        uiCfg.Tachometer.BackgroundColor.R,
        uiCfg.Tachometer.BackgroundColor.G,
        uiCfg.Tachometer.BackgroundColor.B,
        uiCfg.Tachometer.BackgroundColor.A,
        false);

    // Game uses for a basic map for gears 2 and up
    // This compensates for the mapping applied in the loop, because
    // the actual map the game uses, we don't have.
    float mapScaleFactor = 1.0f;
    bool defaultMap = Util::strcmpwi(cfg.Name, "Default");
    if (defaultMap) {
        mapScaleFactor = 1.0f / 0.8f;
    }

    for (int i = 0; i <= numTachoBars; ++i) {
        const int barRpmMin = map(i, 0, numTachoBars, 0, maxRpm);
        const int barRpmMax = map(i + 1, 0, numTachoBars, 0, maxRpm) - 1;
        const float barRpmRel = static_cast<float>(barRpmMin) / static_cast<float>(maxRpm);

        bool solid = false;
        if (realRpm >= barRpmMin &&
            realRpm <= barRpmMax) {
            solid = true;
        }

        float x = map(static_cast<float>(i), 0.0f, static_cast<float>(numTachoBars),
            uiCfg.Tachometer.X - 0.475f * uiCfg.Tachometer.W,
            uiCfg.Tachometer.X + 0.475f * uiCfg.Tachometer.W);

        float w = 0.66f * uiCfg.Tachometer.W / static_cast<float>(numTachoBars);

        float torqueMultiplier;
        if (defaultMap) {
            if (barRpmRel > 0.8f) {
                torqueMultiplier = map(barRpmRel, 0.8f, 1.0f, 1.0f, 0.6f);
            }
            else {
                torqueMultiplier = 1.0f;
            }
        }
        else {
            torqueMultiplier = CustomTorque::GetScaledValue(cfg.Data.TorqueMultMap,
                static_cast<float>(i) / static_cast<float>(numTachoBars));
        }

        float mapMultiplier = (torqueMultiplier * barRpmRel) / cfg.Data.MaxPower.RelativePower;

        float h = 0.90f * uiCfg.Tachometer.H * mapMultiplier * mapScaleFactor *
            mapScaleFactorTurbo * turboDispMod;
        float y = uiCfg.Tachometer.Y + (0.45f * uiCfg.Tachometer.H) - (0.5f * h);

        SColor hiCol = barRpmMin >= redRpm ?
            uiCfg.Tachometer.RedlineHiColor :
            uiCfg.Tachometer.NormalHiColor;

        SColor loCol = barRpmMin >= redRpm ?
            uiCfg.Tachometer.RedlineColor :
            uiCfg.Tachometer.NormalColor;

        GRAPHICS::DRAW_RECT({ x, y },
            w, h,
            solid ? hiCol.R : loCol.R,
            solid ? hiCol.G : loCol.G,
            solid ? hiCol.B : loCol.B,
            solid ? hiCol.A : loCol.A,
            false);
    }
}
