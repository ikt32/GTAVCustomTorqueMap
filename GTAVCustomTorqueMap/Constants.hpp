#pragma once
#include "CustomTorqueMap.hpp"
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

namespace Constants {
    static const char* const NotificationPrefix = "~b~Custom Torque Map~w~";
    static const char* const DisplayVersion = "v" STR(VERSION_MAJOR) "."  STR(VERSION_MINOR) "." STR(VERSION_PATCH);
    static const char* const ModDir = "\\CustomTorqueMap";
}
