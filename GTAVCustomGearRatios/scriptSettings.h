#pragma once
#include <string>

class ScriptSettings
{
public:
    ScriptSettings();
    void SetFiles(const std::string &general);
    void Read();
    void Save() const;

    // [OPTIONS]
    // Load based on model + plate
    bool AutoLoad;
    // Load based on model, overridden by AutoLoad
    bool AutoLoadGeneric;
    // Restore ratios when the game changes them
    bool RestoreRatios;
    // Custom CVT when 1 gear active
    bool EnableCVT;
    // Show autoload messages. Always shown in menu interaction.
    bool AutoNotify;



    // [DEBUG]
    bool Debug;

private:
    void parseSettings();

    std::string settingsGeneralFile;
};
