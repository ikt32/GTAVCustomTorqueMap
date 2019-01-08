#include "scriptSettings.h"

#include <simpleini/SimpleIni.h>

ScriptSettings::ScriptSettings()
    : AutoLoad(true)
    , Debug(false) {}

void ScriptSettings::SetFiles(const std::string &general) {
    settingsGeneralFile = general;
}

void ScriptSettings::Read() {
    parseSettings();
}

void ScriptSettings::Save() const {
    CSimpleIniA settings;
    settings.SetUnicode();
    settings.LoadFile(settingsGeneralFile.c_str());

    settings.SetBoolValue("OPTIONS", "AutoLoad", AutoLoad);
    settings.SetBoolValue("OPTIONS", "EnableCVT", EnableCVT);

    settings.SaveFile(settingsGeneralFile.c_str());
}

void ScriptSettings::parseSettings() {
    CSimpleIniA settingsGeneral;
    settingsGeneral.SetUnicode();
    settingsGeneral.LoadFile(settingsGeneralFile.c_str());

    // [OPTIONS]
    AutoLoad = settingsGeneral.GetBoolValue("OPTIONS", "AutoLoad", true);
    EnableCVT = settingsGeneral.GetBoolValue("OPTIONS", "EnableCVT", false);

    // [DEBUG]
    Debug = settingsGeneral.GetBoolValue("DEBUG", "LogDebug", false);
}
