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
        bool DisplayInfo = false;
    } Debug;

private:
    std::string mSettingsFile;
};
