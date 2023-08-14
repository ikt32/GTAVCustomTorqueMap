#include "Script.hpp"
#include "Constants.hpp"

#include "Util/FileVersion.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Memory/Versions.h"
#include "Memory/VehicleExtensions.hpp"

#include <inc/main.h>

#include <filesystem>


namespace fs = std::filesystem;

void resolveVersion() {
    int shvVersion = getGameVersion();

    LOG(INFO, "SHV API Game version: {} ({})", eGameVersionToString(shvVersion), shvVersion);
    // Also prints the other stuff, annoyingly.
    SVersion exeVersion = getExeInfo();

    // Version we *explicitly* support
    std::vector<int> exeVersionsSupp = findNextLowest(ExeVersionMap, exeVersion);
    if (exeVersionsSupp.empty() || exeVersionsSupp.size() == 1 && exeVersionsSupp[0] == -1) {
        LOG(ERROR, "Failed to find a corresponding game version.");
        LOG(WARN, "    Using SHV API version [{}] ({})",
            eGameVersionToString(shvVersion), shvVersion);
        VehicleExtensions::SetVersion(shvVersion);
        return;
    }

    int highestSupportedVersion = *std::max_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion > highestSupportedVersion) {
        LOG(WARN, "Game newer than last supported version");
        LOG(WARN, "    You might experience instabilities or crashes");
        LOG(WARN, "    Using SHV API version [{}] ({})",
            eGameVersionToString(shvVersion), shvVersion);
        VehicleExtensions::SetVersion(shvVersion);
        return;
    }

    int lowestSupportedVersion = *std::min_element(std::begin(exeVersionsSupp), std::end(exeVersionsSupp));
    if (shvVersion < lowestSupportedVersion) {
        LOG(WARN, "SHV API reported lower version than actual EXE version.");
        LOG(WARN, "    EXE version     [{}] ({})",
            eGameVersionToString(lowestSupportedVersion), lowestSupportedVersion);
        LOG(WARN, "    SHV API version [{}] ({})",
            eGameVersionToString(shvVersion), shvVersion);
        LOG(WARN, "    Using EXE version, or highest supported version [{}] ({})",
            eGameVersionToString(lowestSupportedVersion), lowestSupportedVersion);
        VehicleExtensions::SetVersion(lowestSupportedVersion);
        return;
    }

    LOG(DEBUG, "Using offsets based on SHV API version [{}] ({})",
        eGameVersionToString(shvVersion), shvVersion);
    VehicleExtensions::SetVersion(shvVersion);
}

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + Constants::ModDir;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    g_Logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            g_Logger.SetMinLevel(DEBUG);
            g_Logger.Clear();
            LOG(INFO, "Custom Torque Map {} (built {} {})", Constants::DisplayVersion, __DATE__, __TIME__);
            resolveVersion();

            scriptRegister(hInstance, CustomTorque::ScriptMain);
            LOG(INFO, "Script registered");
            break;
        }
        case DLL_PROCESS_DETACH: {
            scriptUnregister(hInstance);
            break;
        }
        default: {
            break;
        }
    }
    return TRUE;
}
