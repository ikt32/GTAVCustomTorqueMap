#include "ScriptSettings.hpp"
#include "SettingsCommon.hpp"

#include "Util/Logger.hpp"

#include <simpleini/SimpleIni.h>

#define CHECK_LOG_SI_ERROR(result, operation) \
    if (result < 0) { \
        LOG(ERROR, "[Settings] {} Failed to {}, SI_Error [{}]", \
        __FUNCTION__, operation, static_cast<int>(result)); \
    }

#define SAVE_VAL(section, key, option) \
    SetValue(ini, section, key, option)

#define LOAD_VAL(section, key, option) \
    option = GetValue(ini, section, key, option)

CScriptSettings::CScriptSettings(std::string settingsFile)
    : mSettingsFile(std::move(settingsFile)) {

}

void CScriptSettings::Load() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    LOAD_VAL("Main", "EnableNPC", Main.EnableNPC);

    LOAD_VAL("UI", "Measurement", UI.Measurement);
    if (UI.Measurement < 0 || UI.Measurement > 2) {
        LOG(ERROR, "UI.Measurement was {}, fixing range");
        UI.Measurement = 0;
    }

    LOAD_VAL("UI.Tachometer", "Enable", UI.Tachometer.Enable);
    LOAD_VAL("UI.Tachometer", "X", UI.Tachometer.X);
    LOAD_VAL("UI.Tachometer", "Y", UI.Tachometer.Y);
    LOAD_VAL("UI.Tachometer", "W", UI.Tachometer.W);
    LOAD_VAL("UI.Tachometer", "H", UI.Tachometer.H);

    LOAD_VAL("UI.Tachometer", "BackgroundColor.R", UI.Tachometer.BackgroundColor.R);
    LOAD_VAL("UI.Tachometer", "BackgroundColor.G", UI.Tachometer.BackgroundColor.G);
    LOAD_VAL("UI.Tachometer", "BackgroundColor.B", UI.Tachometer.BackgroundColor.B);
    LOAD_VAL("UI.Tachometer", "BackgroundColor.A", UI.Tachometer.BackgroundColor.A);
    LOAD_VAL("UI.Tachometer", "NormalColor.R", UI.Tachometer.NormalColor.R);
    LOAD_VAL("UI.Tachometer", "NormalColor.G", UI.Tachometer.NormalColor.G);
    LOAD_VAL("UI.Tachometer", "NormalColor.B", UI.Tachometer.NormalColor.B);
    LOAD_VAL("UI.Tachometer", "NormalColor.A", UI.Tachometer.NormalColor.A);
    LOAD_VAL("UI.Tachometer", "NormalHiColor.R", UI.Tachometer.NormalHiColor.R);
    LOAD_VAL("UI.Tachometer", "NormalHiColor.G", UI.Tachometer.NormalHiColor.G);
    LOAD_VAL("UI.Tachometer", "NormalHiColor.B", UI.Tachometer.NormalHiColor.B);
    LOAD_VAL("UI.Tachometer", "NormalHiColor.A", UI.Tachometer.NormalHiColor.A);
    LOAD_VAL("UI.Tachometer", "RedlineColor.R", UI.Tachometer.RedlineColor.R);
    LOAD_VAL("UI.Tachometer", "RedlineColor.G", UI.Tachometer.RedlineColor.G);
    LOAD_VAL("UI.Tachometer", "RedlineColor.B", UI.Tachometer.RedlineColor.B);
    LOAD_VAL("UI.Tachometer", "RedlineColor.A", UI.Tachometer.RedlineColor.A);
    LOAD_VAL("UI.Tachometer", "RedlineHiColor.R", UI.Tachometer.RedlineHiColor.R);
    LOAD_VAL("UI.Tachometer", "RedlineHiColor.G", UI.Tachometer.RedlineHiColor.G);
    LOAD_VAL("UI.Tachometer", "RedlineHiColor.B", UI.Tachometer.RedlineHiColor.B);
    LOAD_VAL("UI.Tachometer", "RedlineHiColor.A", UI.Tachometer.RedlineHiColor.A);

    LOAD_VAL("UI.TorqueGraph", "X", UI.TorqueGraph.X);
    LOAD_VAL("UI.TorqueGraph", "Y", UI.TorqueGraph.Y);
    LOAD_VAL("UI.TorqueGraph", "W", UI.TorqueGraph.W);
    LOAD_VAL("UI.TorqueGraph", "H", UI.TorqueGraph.H);

    LOAD_VAL("Debug", "DisplayInfo", Debug.DisplayInfo);
}

void CScriptSettings::Save() {
    CSimpleIniA ini;
    ini.SetUnicode();
    SI_Error result = ini.LoadFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load");

    SAVE_VAL("Main", "EnableNPC", Main.EnableNPC);

    SAVE_VAL("UI", "Measurement", UI.Measurement);

    SAVE_VAL("UI.Tachometer", "Enable", UI.Tachometer.Enable);
    SAVE_VAL("UI.Tachometer", "X", UI.Tachometer.X);
    SAVE_VAL("UI.Tachometer", "Y", UI.Tachometer.Y);
    SAVE_VAL("UI.Tachometer", "W", UI.Tachometer.W);
    SAVE_VAL("UI.Tachometer", "H", UI.Tachometer.H);

    SAVE_VAL("UI.Tachometer", "BackgroundColor.R", UI.Tachometer.BackgroundColor.R);
    SAVE_VAL("UI.Tachometer", "BackgroundColor.G", UI.Tachometer.BackgroundColor.G);
    SAVE_VAL("UI.Tachometer", "BackgroundColor.B", UI.Tachometer.BackgroundColor.B);
    SAVE_VAL("UI.Tachometer", "BackgroundColor.A", UI.Tachometer.BackgroundColor.A);
    SAVE_VAL("UI.Tachometer", "NormalColor.R", UI.Tachometer.NormalColor.R);
    SAVE_VAL("UI.Tachometer", "NormalColor.G", UI.Tachometer.NormalColor.G);
    SAVE_VAL("UI.Tachometer", "NormalColor.B", UI.Tachometer.NormalColor.B);
    SAVE_VAL("UI.Tachometer", "NormalColor.A", UI.Tachometer.NormalColor.A);
    SAVE_VAL("UI.Tachometer", "NormalHiColor.R", UI.Tachometer.NormalHiColor.R);
    SAVE_VAL("UI.Tachometer", "NormalHiColor.G", UI.Tachometer.NormalHiColor.G);
    SAVE_VAL("UI.Tachometer", "NormalHiColor.B", UI.Tachometer.NormalHiColor.B);
    SAVE_VAL("UI.Tachometer", "NormalHiColor.A", UI.Tachometer.NormalHiColor.A);
    SAVE_VAL("UI.Tachometer", "RedlineColor.R", UI.Tachometer.RedlineColor.R);
    SAVE_VAL("UI.Tachometer", "RedlineColor.G", UI.Tachometer.RedlineColor.G);
    SAVE_VAL("UI.Tachometer", "RedlineColor.B", UI.Tachometer.RedlineColor.B);
    SAVE_VAL("UI.Tachometer", "RedlineColor.A", UI.Tachometer.RedlineColor.A);
    SAVE_VAL("UI.Tachometer", "RedlineHiColor.R", UI.Tachometer.RedlineHiColor.R);
    SAVE_VAL("UI.Tachometer", "RedlineHiColor.G", UI.Tachometer.RedlineHiColor.G);
    SAVE_VAL("UI.Tachometer", "RedlineHiColor.B", UI.Tachometer.RedlineHiColor.B);
    SAVE_VAL("UI.Tachometer", "RedlineHiColor.A", UI.Tachometer.RedlineHiColor.A);

    SAVE_VAL("UI.TorqueGraph", "X", UI.TorqueGraph.X);
    SAVE_VAL("UI.TorqueGraph", "Y", UI.TorqueGraph.Y);
    SAVE_VAL("UI.TorqueGraph", "W", UI.TorqueGraph.W);
    SAVE_VAL("UI.TorqueGraph", "H", UI.TorqueGraph.H);

    SAVE_VAL("Debug", "DisplayInfo", Debug.DisplayInfo);

    result = ini.SaveFile(mSettingsFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save");
}

