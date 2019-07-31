#include <inc/main.h>

#include <filesystem>

#include "script.h"

#include "../../GTAVManualTransmission/Gears/Util/FileVersion.h"
#include "../../GTAVManualTransmission/Gears/Util/Paths.h"
#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Memory/Versions.h"
#include "../../GTAVManualTransmission/Gears/Memory/VehicleExtensions.hpp"

namespace fs = std::filesystem;

BOOL APIENTRY DllMain(HMODULE hInstance, DWORD reason, LPVOID lpReserved) {
    const std::string modPath = Paths::GetModuleFolder(hInstance) + MOD_DIRECTORY;
    const std::string logFile = modPath + "\\" + Paths::GetModuleNameWithoutExtension(hInstance) + ".log";

    if (!fs::is_directory(modPath) || !fs::exists(modPath)) {
        fs::create_directory(modPath);
    }

    logger.SetFile(logFile);
    Paths::SetOurModuleHandle(hInstance);

    switch (reason) {
        case DLL_PROCESS_ATTACH: {
            int scriptingVersion = getGameVersion();
            logger.Clear();
            logger.Write(INFO, "GTAVCustomGearRatios %s (build %s)", DISPLAY_VERSION, __DATE__);
            logger.Write(INFO, "Game version " + eGameVersionToString(scriptingVersion));
            if (scriptingVersion < G_VER_1_0_877_1_STEAM) {
                logger.Write(WARN, "Unsupported game version! Update your game.");
            }

            SVersion exeVersion = getExeInfo();
            int actualVersion = findNextLowest(ExeVersionMap, exeVersion);
            if (scriptingVersion % 2) {
                scriptingVersion--;
            }
            if (actualVersion != scriptingVersion) {
                logger.Write(WARN, "Version mismatch!");
                logger.Write(WARN, "    Detected: %s", eGameVersionToString(actualVersion).c_str());
                logger.Write(WARN, "    Reported: %s", eGameVersionToString(scriptingVersion).c_str());
                if (actualVersion == -1) {
                    logger.Write(WARN, "Version detection failed");
                    logger.Write(WARN, "    Using reported version (%s)", eGameVersionToString(scriptingVersion).c_str());
                    actualVersion = scriptingVersion;
                }
                else {
                    logger.Write(WARN, "Using detected version (%s)", eGameVersionToString(actualVersion).c_str());
                }
                VehicleExtensions::ChangeVersion(actualVersion);
            }

            scriptRegister(hInstance, ScriptMain);

            logger.Write(INFO, "Script registered");
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
