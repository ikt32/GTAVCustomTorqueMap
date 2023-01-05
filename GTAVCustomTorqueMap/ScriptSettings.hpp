#pragma once

#include "Util/Color.h"
#include <string>

class CScriptSettings
{
public:
    CScriptSettings(std::string settingsFile);

    void Load();
    void Save();

    struct {
        bool EnableNPC = true;
    } Main;

    struct {
        struct {
            bool Enable = true;
            float X = 0.90f;
            float Y = 0.85f;
            float W = 0.20f;
            float H = 0.20f;

            SColor BackgroundColor{ .R = 0,   .G = 0,   .B = 0,   .A = 191 };

            SColor NormalColor{ .R = 191, .G = 255, .B = 211, .A = 91 };
            SColor NormalHiColor{ .R = 191, .G = 255, .B = 211, .A = 255 };

            SColor RedlineColor{ .R = 255, .G = 31, .B = 31, .A = 91 };
            SColor RedlineHiColor{ .R = 255, .G = 64, .B = 64, .A = 255 };
        } Tachometer;

        struct {
            float X = 0.80f;
            float Y = 0.25f;
            float W = 0.60f;
            float H = 0.40f;
        } TorqueGraph;
    } UI;

    struct {
        bool DisplayInfo = false;
    } Debug;

private:
    std::string mSettingsFile;
};
