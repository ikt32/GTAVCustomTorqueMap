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

struct SPoint {
    float x;
    float y;
    bool solid;
};

namespace {
    // Normal colours
    const SColor sColorTorque{ .R = 219, .G = 86, .B = 55 }; // red-ish
    const SColor sColorPower{ .R = 68, .G = 134, .B = 244 }; // blue-ish

    // Highlights
    const SColor sColorTorqueHigh{ .R = 225, .G = 86, .B = 36 }; // red
    const SColor sColorPowerHigh{ .R = 64, .G = 213, .B = 255 }; // blue-ish

    constexpr int sMaxDisplaySamples = 100;
    constexpr int sIdleRangeSamples = static_cast<int>(static_cast<float>(sMaxDisplaySamples) * 0.2f);

    const float sBaseTurboTorqueMult = 1.1f;

    bool CalcAvail(const CConfig& config, Vehicle vehicle) {
        return
            config.Data.IdleRPM != 0 &&
            config.Data.RevLimitRPM != 0 &&
            ENTITY::DOES_ENTITY_EXIST(vehicle);
    }
}

CustomTorque::SDrawPerformanceValues CustomTorque::GenerateCurveData(const CConfig& config, Vehicle vehicle) {
    const int measurement = CustomTorque::GetSettings().UI.Measurement;

    // Need to know RPM and power figures in order to display as absolute values
    const bool calcAvail = CalcAvail(config, vehicle);

    // To show the default torque map as not-a-flat-line
    const bool defaultMap = Util::strcmpwi(config.Name, "Default");

    const float peakTorqueValueNm = Util::GetHandlingTorqueNm(vehicle);
    const float modTorqueMult = Util::GetEngineModTorqueMult(vehicle);

    std::vector<SDrawPerformanceSample> perfSamples(sMaxDisplaySamples);

    // For scaling
    float maxValue = 0.0f;

    // For informative purposes
    float maxTorque = 0.0f;
    float maxPower = 0.0f;
    std::optional<float> maxTorqueBoost;
    std::optional<float> maxPowerBoost;

    for (int i = 0; i < sMaxDisplaySamples; i++) {
        const float x = static_cast<float>(i) / static_cast<float>(sMaxDisplaySamples);

        float torqueMultiplier = 1.0f;
        if (defaultMap) {
            if (x > 0.8f) {
                torqueMultiplier = map(x, 0.8f, 1.0f, 1.0f, 0.6f);
            }
            else {
                torqueMultiplier = 1.0f;
            }
        }
        float torqueRel = CustomTorque::GetScaledValue(config.Data.TorqueMultMap, x) * torqueMultiplier;

        float torque = 0.0f;
        float power = 0.0f;

        // Relative scaling - Cross at (RPM = 1.0)
        if (measurement == 0 || !calcAvail) {
            torque = torqueRel;
            power = torqueRel * x;
        }
        else if (calcAvail) {
            float realRPM = map(x, 0.2f, 1.0f,
                (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);

            if (x < 0.2f) {
                realRPM = map(x, 0.0f, 0.2f, 0.0f, (float)config.Data.IdleRPM);
            }

            float torqueNm = torqueRel * peakTorqueValueNm * modTorqueMult;

            // Metric - Cross at 9549 RPM ((60 * 1000) / (2 * pi))
            if (measurement == 1) {
                torque = torqueNm;
                power = (0.10471975512f * torqueNm * realRPM) / 1000.0f;
            }

            // Imperial - Cross at 5252 RPM (33000 / ( 2 * pi))
            if (measurement == 2) {
                torque = Nm2lbft(torqueNm);
                power = (Nm2lbft(torqueNm) * realRPM) / 5252.0f;
            }
        }

        // Boost is applied on top of base power, so this is separate
        std::optional<float> torqueBoost;
        std::optional<float> powerBoost;
        if (Util::VehicleHasTurboMod(vehicle)) {
            if (TurboFix::Active()) {
                // Boost is both RPM and spool-up linked, also has boost drop off capability
                float boost = TurboFix::GetAbsoluteBoostConfig(TurboFix::GetActiveConfigName(), x);
                float torqueMult = std::max(1.0f, 1.0f + (boost * 0.1f));
                torqueBoost = torque * torqueMult;
                powerBoost = power * torqueMult;
            }
            else {
                // Boost is not linked to RPM, just spool-up time, and bleeds off slowly.
                torqueBoost = torque * sBaseTurboTorqueMult;
                powerBoost = power * sBaseTurboTorqueMult;
            }
        }

        if (maxValue < torque)                      maxValue = torque;
        if (maxValue < power)                       maxValue = power;
        if (torqueBoost && maxValue < torqueBoost)  maxValue = torqueBoost.value();
        if (powerBoost && maxValue < powerBoost)    maxValue = powerBoost.value();

        perfSamples[i] = {
            .RPM = x,
            .Absolute = {
                .Torque = torque,
                .Power = power,
                .TorqueBoost = torqueBoost,
                .PowerBoost = powerBoost,
            }
        };
    }

    for (auto& sample : perfSamples) {
        sample.Normalized = {
            .Torque = sample.Absolute.Torque / maxValue,
            .Power = sample.Absolute.Power / maxValue,
            .TorqueBoost = sample.Absolute.TorqueBoost ?
                std::optional<float>(sample.Absolute.TorqueBoost.value() / maxValue) :
                std::nullopt,
            .PowerBoost = sample.Absolute.PowerBoost ?
                std::optional<float>(sample.Absolute.PowerBoost.value() / maxValue) :
                std::nullopt,
        };
    }

    return {
        .Samples = perfSamples,
        .MaxValue = maxValue,
    };
}

void CustomTorque::DrawCurve(const CConfig& config, Vehicle vehicle, const SDrawPerformanceValues& perfValues) {
    const int measurement = CustomTorque::GetSettings().UI.Measurement;
    const bool calcAvail = CalcAvail(config, vehicle);

    const float rectX = CustomTorque::GetSettings().UI.TorqueGraph.X;
    const float rectY = CustomTorque::GetSettings().UI.TorqueGraph.Y;
    const float rectW = CustomTorque::GetSettings().UI.TorqueGraph.W / GRAPHICS::GET_ASPECT_RATIO(FALSE);
    const float rectH = CustomTorque::GetSettings().UI.TorqueGraph.H;
    const float blockW = rectW / sMaxDisplaySamples;//0.001f * (16.0f / 9.0f) / GRAPHICS::_GET_ASPECT_RATIO(FALSE);
    const float blockH = blockW * GRAPHICS::GET_ASPECT_RATIO(FALSE);

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
        UI::ShowText(vertAxisValueX, vertAxis1Y, 0.25f, std::format("{:.0f}", perfValues.MaxValue));

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

    auto drawPoint = [&](const SPoint& point, const SColor& color) {
        float pointX = rectX - 0.5f * rectW + point.x * rectW;
        float pointY = rectY + 0.5f * rectH - point.y * rectH;
        GRAPHICS::DRAW_RECT({ pointX, pointY },
            blockW, blockH,
            color.R, color.G, color.B,
            point.solid ? 255 : 91,
            0);
    };

    for (const auto& sample : perfValues.Samples) {
        bool rpmValid = sample.RPM >= 0.2f;
        bool boostPresent = sample.Normalized.TorqueBoost && sample.Normalized.PowerBoost;

        drawPoint({ sample.RPM, sample.Normalized.Torque, rpmValid && !boostPresent }, sColorTorque);
        drawPoint({ sample.RPM, sample.Normalized.Power, rpmValid && !boostPresent }, sColorPower);
        if (boostPresent) {
            drawPoint({ sample.RPM, sample.Normalized.TorqueBoost.value(), rpmValid }, sColorTorque);
            drawPoint({ sample.RPM, sample.Normalized.PowerBoost.value(), rpmValid }, sColorPower);
        }
    }

    if (ENTITY::DOES_ENTITY_EXIST(vehicle)) {
        DrawLiveIndicators(config, vehicle, perfValues,
            {
                .RectX = rectX, .RectY = rectY,
                .RectW = rectW, .RectH = rectH,
                .BlockW = blockW, .BlockH = blockH
            }
        );
    }
}

// This may draw the dots not on the expected line, because external factors may affect things.
void CustomTorque::DrawLiveIndicators(const CConfig& config, Vehicle vehicle, const SDrawPerformanceValues& perfValues, const SGraphDims& graphDims) {
    const float rectX = graphDims.RectX;
    const float rectY = graphDims.RectY;
    const float rectW = graphDims.RectW;
    const float rectH = graphDims.RectH;
    const float blockW = graphDims.BlockW;
    const float blockH = graphDims.BlockH;

    const int measurement = CustomTorque::GetSettings().UI.Measurement;
    const bool calcAvail = CalcAvail(config, vehicle);

    // There is no purpose for this additional indent except to not mess up git history too much with this refactor.
    {
        auto torqueData = GetTorqueData(vehicle, config);
        float rpm = torqueData.NormalizedRPM;

        float pointX = rectX - 0.5f * rectW + rpm * rectW;

        // RPM tag
        float horAxisValueX = pointX;
        float horAxisValueY = rectY + 0.5f * rectH + 0.025f;
        if (measurement == 0 || !calcAvail) {
            UI::ShowText(horAxisValueX, horAxisValueY, 0.25f, std::format("{:1.2f}", rpm));
        }
        else {
            float realRPM = map(rpm, 0.2f, 1.0f, (float)config.Data.IdleRPM, (float)config.Data.RevLimitRPM);
            UI::ShowText(horAxisValueX, horAxisValueY, 0.25f, std::format("{:.0f}", realRPM));
        }

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
            // Mapping live values as relative is a bit annoying, need to normalize things.
            const float peakTorqueValueNm = Util::GetHandlingTorqueNm(vehicle);
            const float modTorqueMult = Util::GetEngineModTorqueMult(vehicle);
            float turboTorqueMult = 1.0f;
            if (Util::VehicleHasTurboMod(vehicle)) {
                float boost = VExt::GetTurbo(vehicle);
                turboTorqueMult = std::max(1.0f, 1.0f + (boost * 0.1f));
            }

            torqueValue = torqueData.TotalForceNm / (peakTorqueValueNm * modTorqueMult * turboTorqueMult);
            powerValue = rpm * torqueValue;

            torqueScaled = torqueValue;
            powerScaled = powerValue;
        }
        else if (torqueData.RPMData) {
            float realRPM = torqueData.RPMData->RealRPM;
            float torqueNm = torqueData.TotalForceNm;

            // Metric - Cross at 9549 RPM ((60 * 1000) / (2 * pi))
            if (measurement == 1 && calcAvail) {
                const float powerkW = (0.10471975512f * torqueNm * realRPM) / 1000.0f;

                torqueValue = torqueNm;
                powerValue = powerkW;

                torqueScaled = torqueNm / perfValues.MaxValue;
                powerScaled = powerkW / perfValues.MaxValue;
            }

            // Imperial - Cross at 5252 RPM (33000 / ( 2 * pi))
            if (measurement == 2 && calcAvail) {
                const float powerhp = (Nm2lbft(torqueNm) * realRPM) / 5252.0f;

                torqueValue = Nm2lbft(torqueNm);
                powerValue = powerhp;

                torqueScaled = Nm2lbft(torqueNm) / perfValues.MaxValue;
                powerScaled = powerhp / perfValues.MaxValue;
            }
        }

        // Torque
        float pointYTorque = rectY + 0.5f * rectH - torqueScaled * rectH;
        if (measurement == 0 || !calcAvail) {
            // pointYTorque's currentPoint.second needs no modification
            UI::ShowText(vertAxisValueX, pointYTorque, 0.25f, std::format("~r~{:1.2f}x", torqueValue));
        }
        else {
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
            sColorTorqueHigh.R, sColorTorqueHigh.G, sColorTorqueHigh.B, 255, 0);

        // Power
        float pointYPower = rectY + 0.5f * rectH - powerScaled * rectH;
        if (measurement == 0 || !calcAvail) {
            // pointYPower's (currentPoint.second * rpm) needs no modification
            UI::ShowText(vertAxisValueX, pointYPower, 0.25f, std::format("~b~{:1.2f}x", powerValue));
        }
        else {
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
            sColorPowerHigh.R, sColorPowerHigh.G, sColorPowerHigh.B, 255, 0);
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
