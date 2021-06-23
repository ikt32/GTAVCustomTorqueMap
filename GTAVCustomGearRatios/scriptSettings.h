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
    
    struct {
        float LowRatio = 3.3; // 1st gear
        float HighRatio = 0.9; // top gear
        float Factor = 0.75;
    } CVT;

    // Show autoload messages. Always shown in menu interaction.
    bool AutoNotify;
    // Enable for NPC
    bool EnableNPC;

    // [DEBUG]
    bool Debug;

private:
    void parseSettings();

    std::string settingsGeneralFile;
};
