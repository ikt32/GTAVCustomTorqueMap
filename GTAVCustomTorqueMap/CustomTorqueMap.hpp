#pragma once

#ifdef CTM_EXPORTS
#define CTM_API extern "C" __declspec(dllexport)

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 1

#else
#ifndef CTM_RUNTIME
#define CTM_API extern "C" __declspec(dllimport)
#else
// noop
#define CTM_API
#endif
#endif

struct CTM_RPMInfo {
    float IdleRPM;
    float RevLimitRPM;
    float ActualRPM;
};

CTM_API bool CTM_GetPlayerRPMInfo(CTM_RPMInfo* rpmInfo);
