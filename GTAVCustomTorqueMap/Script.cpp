#include "Script.hpp"

#include "Compatibility.hpp"
#include "Constants.hpp"
#include "CustomTorqueMap.hpp"
#include "ScriptMenu.hpp"
#include "TorqueScript.hpp"
#include "TorqueScriptNPC.hpp"
#include "TorqueUtil.hpp"

#include "Util/Logger.hpp"
#include "Util/Paths.hpp"
#include "Util/String.hpp"
#include "Memory/VehicleExtensions.hpp"

#include <inc/natives.h>
#include <inc/main.h>
#include <format>
#include <memory>
#include <filesystem>

using namespace CustomTorque;

namespace {
    std::shared_ptr<CScriptSettings> settings;
    std::shared_ptr<CTorqueScript> playerScriptInst;
    std::vector<std::shared_ptr<CTorqueScriptNPC>> npcScriptInsts;
    std::unique_ptr<CScriptMenu<CTorqueScript>> scriptMenu;

    std::vector<CConfig> configs;

    bool initialized = false;
}

void CustomTorque::ScriptMain() {
    if (!initialized) {
        LOG(INFO, "Script started");
        ScriptInit();
        initialized = true;
    }
    else {
        LOG(INFO, "Script restarted");
    }
    ScriptTick();
}

void CustomTorque::ScriptInit() {
    const std::string settingsGeneralPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_general.ini";
    const std::string settingsMenuPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\settings_menu.ini";

    settings = std::make_shared<CScriptSettings>(settingsGeneralPath);
    settings->Load();
    LOG(INFO, "Settings loaded");

    CustomTorque::LoadConfigs();

    playerScriptInst = std::make_shared<CTorqueScript>(*settings, configs);

    VehicleExtensions::Init();
    Compatibility::Setup();

    scriptMenu = std::make_unique<CScriptMenu<CTorqueScript>>(settingsMenuPath,
        []() {
            // OnInit
            settings->Load();
            CustomTorque::LoadConfigs();
            InvalidateCachedTorqueGraphData();
        },
        []() {
            // OnExit
            settings->Save();
        },
        BuildMenu()
    );
}

void CustomTorque::ScriptTick() {
    while (true) {
        playerScriptInst->Tick();

        if (settings->Main.EnableNPC)
            UpdateNPC();

        scriptMenu->Tick(*playerScriptInst);
        WAIT(0);
    }
}

void CustomTorque::UpdateNPC() {
    std::vector<std::shared_ptr<CTorqueScriptNPC>> instsToDelete;
    
    std::vector<Vehicle> allVehicles(1024);
    int actualSize = worldGetAllVehicles(allVehicles.data(), 1024);
    allVehicles.resize(actualSize);
    
    for (const auto& vehicle : allVehicles) {
        if (ENTITY::IS_ENTITY_DEAD(vehicle, 0) ||
            vehicle == playerScriptInst->GetVehicle() ||
            !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(vehicle))
            continue;
    
        auto it = std::find_if(npcScriptInsts.begin(), npcScriptInsts.end(), [vehicle](const auto& inst) {
            return inst->GetVehicle() == vehicle;
            });
    
        if (it == npcScriptInsts.end()) {
            npcScriptInsts.push_back(std::make_shared<CTorqueScriptNPC>(vehicle, *settings, configs));
            auto npcScriptInst = npcScriptInsts.back();
    
            npcScriptInst->UpdateActiveConfig(false);
        }
    }
    
    for (const auto& inst : npcScriptInsts) {
        if (!ENTITY::DOES_ENTITY_EXIST(inst->GetVehicle()) ||
            ENTITY::IS_ENTITY_DEAD(inst->GetVehicle(), 0) ||
            inst->GetVehicle() == playerScriptInst->GetVehicle()) {
            instsToDelete.push_back(inst);
        }
        else {
            inst->Tick();
        }
    }
    
    for (const auto& inst : instsToDelete) {
        npcScriptInsts.erase(std::remove(npcScriptInsts.begin(), npcScriptInsts.end(), inst), npcScriptInsts.end());
    }
}

void CustomTorque::UpdateActiveConfigs() {
    if (playerScriptInst)
        playerScriptInst->UpdateActiveConfig(true);

    for (const auto& inst : npcScriptInsts) {
        inst->UpdateActiveConfig(false);
    }
}

CScriptSettings& CustomTorque::GetSettings() {
    return *settings;
}

CTorqueScript* CustomTorque::GetScript() {
    return playerScriptInst.get();
}

uint64_t CustomTorque::GetNPCScriptCount() {
    uint64_t numActive = 0;
    for (auto& npcInstance : npcScriptInsts) {
        if (!npcInstance->ActiveConfig()->Name.empty())
            ++numActive;
    }
    return numActive;
}

void CustomTorque::ClearNPCScripts() {
    npcScriptInsts.clear();
}

const std::vector<CConfig>& CustomTorque::GetConfigs() {
    return configs;
}

uint32_t CustomTorque::LoadConfigs() {
    namespace fs = std::filesystem;

    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";

    LOG(DEBUG, "Clearing and reloading configs");

    configs.clear();

    if (!(fs::exists(fs::path(configsPath)) && fs::is_directory(fs::path(configsPath)))) {
        LOG(ERROR, "Directory [{}] not found!", configsPath);
        CustomTorque::UpdateActiveConfigs();
        return 0;
    }

    for (const auto& file : fs::directory_iterator(configsPath)) {
        if (Util::to_lower(fs::path(file).extension().string()) != ".ini") {
            LOG(DEBUG, "Skipping [{}] - not .ini", file.path().stem().string());
            continue;
        }

        CConfig config = CConfig::Read(fs::path(file).string());
        if (Util::strcmpwi(config.Name, "Default")) {
            configs.insert(configs.begin(), config);
            continue;
        }

        configs.push_back(config);
        LOG(DEBUG, "Loaded vehicle config [{}]", config.Name);
    }

    if (configs.empty() ||
        !configs.empty() && !Util::strcmpwi(configs[0].Name, "Default")) {
        LOG(WARN, "No default config found, generating a default one and saving it...");
        CConfig defaultConfig;
        defaultConfig.Name = "Default";
        configs.insert(configs.begin(), defaultConfig);
        defaultConfig.Write(CConfig::ESaveType::GenericNone);
    }

    LOG(INFO, "Configs loaded: {}", configs.size());

    CustomTorque::UpdateActiveConfigs();
    return static_cast<unsigned>(configs.size());
}

bool CTM_GetPlayerRPMInfo(CTM_RPMInfo* rpmInfo) {
    auto* playerInstance = CustomTorque::GetScript();
    if (rpmInfo && playerInstance) {
        auto* playerConfig = playerInstance->ActiveConfig();
        if (playerConfig) {
            auto torqueData = CustomTorque::GetTorqueData(playerInstance->GetVehicle(), *playerConfig);
            if (torqueData.RPMData) {
                rpmInfo->IdleRPM = static_cast<float>(playerConfig->Data.IdleRPM);
                rpmInfo->RevLimitRPM = static_cast<float>(playerConfig->Data.RevLimitRPM);
                rpmInfo->ActualRPM = torqueData.RPMData->RealRPM;
                return true;
            }
        }
    }
    return false;
}
