#pragma once

#include "Config.hpp"
#include "TorqueScript.hpp"
#include <inc/types.h>

namespace CustomTorque {
    void DrawCurve(CTorqueScript& context, const CConfig& config, Vehicle vehicle);

    void DrawTachometer(CTorqueScript& context, Vehicle vehicle);
}
