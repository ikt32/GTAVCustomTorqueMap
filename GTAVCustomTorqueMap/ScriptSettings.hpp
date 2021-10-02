#pragma once
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
        float TorqueGraphX = 0.80f;
        float TorqueGraphY = 0.25f;
        float TorqueGraphW = 0.60f;
        float TorqueGraphH = 0.40f;
    } UI;

    struct {
        bool DisplayInfo = false;
    } Debug;

private:
    std::string mSettingsFile;
};
