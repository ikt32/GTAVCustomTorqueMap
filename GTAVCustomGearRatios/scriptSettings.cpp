#include "scriptSettings.h"

//#include <simpleini/SimpleIni.h>

ScriptSettings::ScriptSettings()
{
}

void ScriptSettings::SetFiles(const std::string &general) {
    settingsGeneralFile = general;
}

void ScriptSettings::Read() {
    parseSettings();
}

void ScriptSettings::parseSettings() {
    //CSimpleIniA settingsGeneral;
    //settingsGeneral.SetUnicode();
    //settingsGeneral.LoadFile(settingsGeneralFile.c_str());
}
