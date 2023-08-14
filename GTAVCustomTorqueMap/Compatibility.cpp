#include "Compatibility.hpp"

#include "Util/Logger.hpp"
#include <Windows.h>

namespace {
    HMODULE TurboFixModule = nullptr;
    bool(*TF_Active)() = nullptr;
    float(*TF_GetAbsoluteBoostMax)() = nullptr;
}

template <typename T>
T CheckAddr(HMODULE lib, const std::string& funcName) {
    FARPROC func = GetProcAddress(lib, funcName.c_str());
    if (!func) {
        LOG(ERROR, "[Compat] Couldn't get function [{}]", funcName);
        return nullptr;
    }
    LOG(DEBUG, "[Compat] Found function [{}]", funcName);
    return reinterpret_cast<T>(func);
}

void Compatibility::Setup() {
    TurboFix::Setup();
}

void Compatibility::Cleanup() {
    // Nothing yet
}

void TurboFix::Setup() {
    LOG(INFO, "[Compat] Setting up TurboFix");
    if (TurboFixModule &&
        TF_Active && TF_GetAbsoluteBoostMax) {
        LOG(INFO, "[Compat] TurboFix already loaded");
        return;
    }

    TurboFixModule = GetModuleHandle(L"TurboFix.asi");
    if (!TurboFixModule) {
        LOG(INFO, "[Compat] TurboFix.asi not found");
        return;
    }

    TF_Active = CheckAddr<bool(*)()>(TurboFixModule, "TF_Active");
    TF_GetAbsoluteBoostMax = CheckAddr<float(*)()>(TurboFixModule, "TF_GetAbsoluteBoostMax");
}

bool TurboFix::Active() {
    if (TF_Active)
        return TF_Active();
    return false;
}

float TurboFix::GetAbsoluteBoostMax() {
    if (TF_GetAbsoluteBoostMax)
        return TF_GetAbsoluteBoostMax();
    return 0.0f;
}
