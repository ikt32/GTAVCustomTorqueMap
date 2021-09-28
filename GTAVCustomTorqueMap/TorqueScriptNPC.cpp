#include "TorqueScriptNPC.hpp"

CTorqueScriptNPC::CTorqueScriptNPC(
    Vehicle vehicle,
    CScriptSettings& settings,
    std::vector<CConfig>& configs)
    : CTorqueScript(settings, configs) {
    mIsNPC = true;
    mVehicle = vehicle;
}

void CTorqueScriptNPC::Tick() {
    updateTorque();
}
