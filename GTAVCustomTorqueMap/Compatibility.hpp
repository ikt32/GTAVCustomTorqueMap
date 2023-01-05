#pragma once

namespace Compatibility {
    void Setup();
    void Cleanup();
}

namespace TurboFix {
    void Setup();

    bool Active();
    float GetAbsoluteBoostMax();
}
