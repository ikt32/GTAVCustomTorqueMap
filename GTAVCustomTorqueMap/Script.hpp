#pragma once
#include "TorqueScript.hpp"
#include "ScriptMenu.hpp"

namespace CustomTorque {
    void ScriptMain();
    void ScriptInit();
    void ScriptTick();
    void UpdateNPC();
    void UpdateActiveConfigs();
    std::vector<CScriptMenu<CTorqueScript>::CSubmenu> BuildMenu();
    void InvalidateCachedTorqueGraphData();

    CScriptSettings& GetSettings();
    CTorqueScript* GetScript();
    uint64_t GetNPCScriptCount();
    void ClearNPCScripts();
    const std::vector<CConfig>& GetConfigs();

    uint32_t LoadConfigs();
}
