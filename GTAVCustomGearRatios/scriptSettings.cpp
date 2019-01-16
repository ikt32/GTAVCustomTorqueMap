#include "scriptSettings.h"

#include <simpleini/SimpleIni.h>

ScriptSettings::ScriptSettings()
    : AutoLoad(true)
    , AutoLoadGeneric(true)
    , RestoreRatios(true)
    , EnableCVT(false)
    , AutoNotify(true)
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
    settings.SetBoolValue("OPTIONS", "AutoLoadGeneric", AutoLoadGeneric);
    settings.SetBoolValue("OPTIONS", "RestoreRatios", RestoreRatios);
    settings.SetBoolValue("OPTIONS", "EnableCVT", EnableCVT);
    settings.SetBoolValue("Options", "AutoNotify", AutoNotify);

    settings.SaveFile(settingsGeneralFile.c_str());
}

void ScriptSettings::parseSettings() {
    CSimpleIniA settings;
    settings.SetUnicode();
    settings.LoadFile(settingsGeneralFile.c_str());

    // [OPTIONS]
    AutoLoad = settings.GetBoolValue("OPTIONS", "AutoLoad", true);
    AutoLoadGeneric = settings.GetBoolValue("OPTIONS", "AutoLoadGeneric", true);
    RestoreRatios = settings.GetBoolValue("OPTIONS", "RestoreRatios", true);
    EnableCVT = settings.GetBoolValue("OPTIONS", "EnableCVT", false);
    AutoNotify = settings.GetBoolValue("OPTIONS", "AutoNotify", true);

    // [DEBUG]
    Debug = settings.GetBoolValue("DEBUG", "LogDebug", false);
}
