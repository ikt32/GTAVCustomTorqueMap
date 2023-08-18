#pragma once

#include <string>

namespace Compatibility {
    void Setup();
    void Cleanup();
}

namespace TurboFix {
    void Setup();

    bool Active();
    float GetAbsoluteBoostMax();

    std::string GetActiveConfigName();
    float GetAbsoluteBoostConfig(const std::string& configName, float rpm);
}
