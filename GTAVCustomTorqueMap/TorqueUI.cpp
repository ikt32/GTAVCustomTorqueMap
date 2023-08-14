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
#include <format>

using VExt = VehicleExtensions;

void CustomTorque::DrawCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle) {
    struct SPoint {
        float x;
        float y;
        bool solid;
    };

    const SColor torqueCol{ .R = 219, .G = 86, .B = 55 }; // red-ish
    const SColor powerCol{ .R = 68, .G = 134, .B = 244 }; // blue-ish

    // Highlights
    const SColor torqueColH{ .R = 225, .G = 86, .B = 36 }; // red
    const SColor powerColH{ .R = 64, .G = 213, .B = 255 }; // blue-ish

    const int maxSamples = 100;
    const int idleRange = 20; // 0.2 of maxSamples
    const int measurement = CustomTorque::GetSettings().UI.Measurement;

    // Need to know RPM and power figures in order to display as absolute values
    bool calcAvail =
        config.Data.IdleRPM != 0 &&
        config.Data.RevLimitRPM != 0 &&
        ENTITY::DOES_ENTITY_EXIST(vehicle);

    float maxTorqueNm = 0.0f;
    float maxPowerkW = 0.0f;

    float maxMeasurement = 1.0f;

    if (calcAvail) {
        maxTorqueNm = Util::GetHandlingTorqueNm(vehicle);

        const float peakhpRPM = map(config.Data.Peak.Power, 0.2f, 1.0f,
            (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);
        maxPowerkW = (0.10471975512f * maxTorqueNm * peakhpRPM) / 1000.0f;

        if (measurement == 0) {
            maxMeasurement = 1.0f;
        }
        if (measurement == 1) {
            maxMeasurement = std::max(maxTorqueNm, maxPowerkW);
        }
        if (measurement == 2) {
            maxMeasurement = std::max(Nm2lbft(maxTorqueNm), kW2hp(maxPowerkW));
        }
    }

    std::vector<SPoint> torquePoints(maxSamples);
    std::vector<SPoint> powerPoints(maxSamples);

    for (int i = 0; i < maxSamples; i++) {
        float x = static_cast<float>(i) / static_cast<float>(maxSamples);
        float torqueRel = CustomTorque::GetScaledValue(config.Data.TorqueMultMap, x);

        float torqueDisplay = 0;
        float powerDisplay = 0;

        // Relative scaling - Cross at (RPM = 1.0)
        if (measurement == 0 || !calcAvail) {
            torqueDisplay = torqueRel;
            powerDisplay = torqueDisplay * x;
        }

        float realRPM = map(x, 0.2f, 1.0f,
            (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

        if (x < 0.2f) {
            realRPM = map(x, 0.0f, 0.2f, 0.0f, (float)config.Data.IdleRPM);
        }

        float torqueNm = torqueRel * maxTorqueNm;

        // Metric - Cross at 9549 RPM ((60 * 1000) / (2 * pi))
        if (measurement == 1 && calcAvail) {
            const float powerkW = (0.10471975512f * torqueNm * realRPM) / 1000.0f;

            torqueDisplay = torqueNm / maxMeasurement;
            powerDisplay = powerkW / maxMeasurement;
        }

        // Imperial - Cross at 5252 RPM (33000 / ( 2 * pi))
        if (measurement == 2 && calcAvail) {
            const float powerhp = (Nm2lbft(torqueNm) * realRPM) / 5252.0f;

            torqueDisplay = Nm2lbft(torqueNm) / maxMeasurement;
            powerDisplay = powerhp / maxMeasurement;
        }

        torquePoints[i] = { x, torqueDisplay, i >= idleRange };
        powerPoints[i] = { x, powerDisplay, i >= idleRange };
    }

    float rectX = CustomTorque::GetSettings().UI.TorqueGraph.X;
    float rectY = CustomTorque::GetSettings().UI.TorqueGraph.Y;
    float rectW = CustomTorque::GetSettings().UI.TorqueGraph.W / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    float rectH = CustomTorque::GetSettings().UI.TorqueGraph.H;
    float blockW = rectW / maxSamples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    float blockH = blockW * GRAPHICS::GET_ASPECT_RATIO(FALSE);

    float vertAxisLabelX = rectX - 0.5f * rectW - 0.05f;
    float vertAxisLabelY = rectY;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(vertAxisLabelX, vertAxisLabelY, 0.25f, "~r~Torque ~n~ ~b~Power");
    if (measurement == 1 && calcAvail)
        UI::ShowText(vertAxisLabelX, vertAxisLabelY, 0.25f, "~r~N-m ~n~ ~b~kW");
    if (measurement == 2 && calcAvail)
        UI::ShowText(vertAxisLabelX, vertAxisLabelY, 0.25f, "~r~lb-ft ~n~ ~b~hp");

    float vertAxisValueX = rectX - 0.5f * rectW - 0.025f;
    float vertAxis0Y = rectY + 0.5f * rectH;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(vertAxisValueX, vertAxis0Y, 0.25f, "0.00x");
    else
        UI::ShowText(vertAxisValueX, vertAxis0Y, 0.25f, "0");

    float vertAxis1Y = rectY - 0.5f * rectH;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(vertAxisValueX, vertAxis1Y, 0.25f, "1.00x");
    else
        UI::ShowText(vertAxisValueX, vertAxis1Y, 0.25f, std::format("{:.0f}", maxMeasurement));

    float horAxisLabelX = rectX;
    float horAxisLabelY = rectY + 0.5f * rectH + 0.05f;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(horAxisLabelX, horAxisLabelY, 0.25f, "Normalized RPM");
    else
        UI::ShowText(horAxisLabelX, horAxisLabelY, 0.25f, "RPM");

    float horAxis0_0X = rectX - 0.5f * rectW;
    float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(horAxis0_0X, horAxisValueY, 0.25f, "0.00");
    else
        UI::ShowText(horAxis0_0X, horAxisValueY, 0.25f, "0");

    float horAxis0_2X = rectX - 0.5f * rectW + 0.2f * rectW;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(horAxis0_2X, horAxisValueY, 0.25f, "0.20 ~n~ (Idle)");
    else
        UI::ShowText(horAxis0_2X, horAxisValueY, 0.25f, std::format("{} ~n~ (Idle)", config.Data.IdleRPM));

    float horAxis1_0X = rectX + 0.5f * rectW;
    if (measurement == 0 || !calcAvail)
        UI::ShowText(horAxis1_0X, horAxisValueY, 0.25f, "1.00");
    else
        UI::ShowText(horAxis1_0X, horAxisValueY, 0.25f, std::format("{}", config.Data.RevLimitRPM));


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

    for (auto point : torquePoints) {
        float pointX = rectX - 0.5f * rectW + point.x * rectW;
        float pointY = rectY + 0.5f * rectH - point.y * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
            blockW, blockH,
            torqueCol.R, torqueCol.G, torqueCol.B,
            point.solid ? 255 : 91,
            0);
    }

    for (auto point : powerPoints) {
        float pointX = rectX - 0.5f * rectW + point.x * rectW;
        float pointY = rectY + 0.5f * rectH - point.y * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
            blockW, blockH,
            powerCol.R, powerCol.G, powerCol.B,
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

        // RPM
        float horAxisValueX = pointX;
        float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
        if (measurement == 0 || !calcAvail)
            UI::ShowText(horAxisValueX, horAxisValueY, 0.25f, std::format("{:1.2f}", input));
        else
            UI::ShowText(horAxisValueX, horAxisValueY, 0.25f, std::format("{:.0f}", map(input, 0.2f, 1.0f, (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM)));

        // Vertical RPM bar
        GRAPHICS::DRAW_RECT({ pointX, rectY },
            0.5f * blockW, rectH + 2.0f * blockH,
            255, 255, 255, 63, 0);

        // Torque / Power
        float vertAxisValueX = rectX - 0.5f * rectW - 0.025f;

        float torqueValue = 0.0f;
        float powerValue = 0.0f;

        float torqueScaled = 0.0f;
        float powerScaled = 0.0f;

        if (measurement == 0 || !calcAvail) {
            torqueValue = currentPoint.second;
            powerValue = currentPoint.first * currentPoint.second;

            torqueScaled = torqueValue;
            powerScaled = powerValue;
        }
        if (measurement == 1 && calcAvail ||
            measurement == 2 && calcAvail) {
            float realRPM = map(input, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

            if (input < 0.2f) {
                realRPM = map(input, 0.0f, 0.2f, 0.0f, (float)config.Data.IdleRPM);
            }

            float torqueNm = currentPoint.second * maxTorqueNm;

            // Metric - Cross at 9549 RPM ((60 * 1000) / (2 * pi))
            if (measurement == 1 && calcAvail) {
                const float powerkW = (0.10471975512f * torqueNm * realRPM) / 1000.0f;

                torqueValue = torqueNm;
                powerValue = powerkW;

                torqueScaled = torqueNm / maxMeasurement;
                powerScaled = powerkW / maxMeasurement;
            }

            // Imperial - Cross at 5252 RPM (33000 / ( 2 * pi))
            if (measurement == 2 && calcAvail) {
                const float powerhp = (Nm2lbft(torqueNm) * realRPM) / 5252.0f;

                torqueValue = Nm2lbft(torqueNm);
                powerValue = powerhp;

                torqueScaled = Nm2lbft(torqueNm) / maxMeasurement;
                powerScaled = powerhp / maxMeasurement;
            }
        }

        // Torque
        float pointYTorque = rectY + 0.5f * rectH - torqueScaled * rectH;
        if (measurement == 0 || !calcAvail) {
            // pointYTorque's currentPoint.second needs no modification
            UI::ShowText(vertAxisValueX, pointYTorque, 0.25f, std::format("~r~{:1.2f}x", torqueValue));
        }
        if (measurement == 1 && calcAvail) {
            pointYTorque = rectY + 0.5f * rectH - torqueScaled * rectH;
            UI::ShowText(vertAxisValueX, pointYTorque, 0.25f, std::format("~r~{:.0f}", torqueValue));
        }
        if (measurement == 2 && calcAvail) {
            pointYTorque = rectY + 0.5f * rectH - torqueScaled * rectH;
            UI::ShowText(vertAxisValueX, pointYTorque, 0.25f, std::format("~r~{:.0f}", torqueValue));
        }

        // Horizontal torque bar
        GRAPHICS::DRAW_RECT({ rectX, pointYTorque },
            rectW + 2.0f * blockW, 0.5f * blockH,
            255, 255, 255, 63, 0);

        // Torque dot
        GRAPHICS::DRAW_RECT({ pointX, pointYTorque },
            1.5f * blockW, 1.5f * blockH,
            torqueColH.R, torqueColH.G, torqueColH.B, 255, 0);

        // Power
        float pointYPower = rectY + 0.5f * rectH - powerScaled * rectH;
        if (measurement == 0 || !calcAvail) {
            // pointYPower's (currentPoint.second * input) needs no modification
            UI::ShowText(vertAxisValueX, pointYPower, 0.25f, std::format("~b~{:1.2f}x", powerValue));
        }

        if (measurement == 1 && calcAvail) {
            pointYPower = rectY + 0.5f * rectH - powerScaled * rectH;
            UI::ShowText(vertAxisValueX, pointYPower, 0.25f, std::format("~b~{:.0f}", powerValue));
        }
        if (measurement == 2 && calcAvail) {
            pointYPower = rectY + 0.5f * rectH - powerScaled * rectH;
            UI::ShowText(vertAxisValueX, pointYPower, 0.25f, std::format("~b~{:.0f}", powerValue));
        }

        // Horizontal power bar
        GRAPHICS::DRAW_RECT({ rectX, pointYPower },
            rectW + 2.0f * blockW, 0.5f * blockH,
            255, 255, 255, 63, 0);

        // Power dot
        GRAPHICS::DRAW_RECT({ pointX, pointYPower },
            1.5f * blockW, 1.5f * blockH,
            powerColH.R, powerColH.G, powerColH.B, 255, 0);
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

        float mapMultiplier = (torqueMultiplier * barRpmRel) / cfg.Data.Peak.Power;

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
