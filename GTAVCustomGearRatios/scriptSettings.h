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
    // Custom CVT when 1 gear active
    bool EnableCVT;

    // [DEBUG]
    bool Debug;

private:
    void parseSettings();

    std::string settingsGeneralFile;
};
