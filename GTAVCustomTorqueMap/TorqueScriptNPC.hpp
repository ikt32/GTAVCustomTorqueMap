#pragma once
#include "TorqueScript.hpp"

class CTorqueScriptNPC : public CTorqueScript {
public:
    CTorqueScriptNPC(
        Vehicle vehicle,
        CScriptSettings& settings,
        std::vector<CConfig>& configs
    );

    void Tick() override;
};

