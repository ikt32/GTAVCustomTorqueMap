#include "Config.hpp"
#include "SettingsCommon.hpp"

#include "Constants.hpp"
#include "Util/AddonSpawnerCache.hpp"
#include "Util/Paths.hpp"
#include "Util/Logger.hpp"
#include "Util/String.hpp"

#include <simpleini/SimpleIni.h>
#include <fmt/format.h>
#include <filesystem>
#include <sstream>

#define CHECK_LOG_SI_ERROR(result, operation, file) \
    if ((result) < 0) { \
        logger.Write(ERROR, "[Config] %s Failed to %s, SI_Error [%d]", \
        file, operation, result); \
    }

#define SAVE_VAL(section, key, option) \
    { \
        SetValue(ini, section, key, option); \
    }

#define LOAD_VAL(section, key, option) \
    { \
        option = (GetValue(ini, section, key, option)); \
    }

CConfig CConfig::Read(const std::string& configFile) {
    CConfig config{};

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiLine(true);
    SI_Error result = ini.LoadFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load", configFile.c_str());

    config.Name = std::filesystem::path(configFile).stem().string();
    logger.Write(INFO, "Loading vehicle config [%s]", config.Name.c_str());

    // [ID]
    std::string modelNamesAll = ini.GetValue("ID", "Models", "");

    std::string modelHashStr = ini.GetValue("ID", "ModelHash", "");
    std::string modelName = ini.GetValue("ID", "ModelName", "");


    if (modelHashStr.empty() && modelName.empty()) {
        // This is a no-vehicle config. Nothing to be done.
    }
    else if (modelHashStr.empty()) {
        // This config only has a model name.
        config.ModelHash = Util::joaat(modelName.c_str());
        config.ModelName = modelName;
    }
    else {
        // This config only has a hash.
        Hash modelHash = 0;
        int found = sscanf_s(modelHashStr.c_str(), "%X", &modelHash);

        if (found == 1) {
            config.ModelHash = modelHash;

            auto& asCache = ASCache::Get();
            auto it = asCache.find(modelHash);
            std::string modelName = it == asCache.end() ? std::string() : it->second;
            config.ModelName = modelName;
        }
    }

    config.Plate = ini.GetValue("ID", "Plate", "");

    // [Data]
    LOAD_VAL("Data", "IdleRPM", config.Data.IdleRPM);
    LOAD_VAL("Data", "RevLimitRPM", config.Data.RevLimitRPM);

    std::string torqueMapString = ini.GetValue("Data", "TorqueMultMap", "");
    config.Data.TorqueMultMap.clear();

    if (!torqueMapString.empty()) {

        std::stringstream torqueMapStream = std::stringstream(torqueMapString);
        std::string line;
        while (torqueMapStream >> line) {
            float rpm, mult;
            auto scanned = sscanf_s(line.c_str(), "%f|%f", &rpm, &mult);
            if (scanned != 2) {
                logger.Write(ERROR, "Failed to read line in torque map file '%s'", line.c_str());
                continue;
            }
            config.Data.TorqueMultMap.emplace(rpm, mult);
        }
    }
    else {
        logger.Write(ERROR, "Missing torque mult map. Raw data: [%s]", torqueMapString.c_str());
    }

    if (config.Data.TorqueMultMap.begin() != config.Data.TorqueMultMap.end() &&
        config.Data.TorqueMultMap.begin()->first > 1.0f) {
        logger.Write(DEBUG, "RPM|Torque map detected, converting into relative values");

        float maxRpm = 0.0f;
        float maxTorque = 0.0f;

        // Need to iterate twice over the whole list... :(
        for (const auto& [rpm, torque] : config.Data.TorqueMultMap) {
            if (rpm > maxRpm)
                maxRpm = rpm;
            if (torque > maxTorque)
                maxTorque = torque;
        }

        std::map<float, float> mappedTorqueMap;
        for (auto& [rpm, torque] : config.Data.TorqueMultMap) {
            mappedTorqueMap.emplace(rpm / maxRpm, torque / maxTorque);
        }

        config.Data.TorqueMultMap = mappedTorqueMap;
    }

    return config;
}

void CConfig::Write(ESaveType saveType) {
    Write(Name, 0, std::string(), saveType);
}

bool CConfig::Write(const std::string& newName, Hash model, std::string plate, ESaveType saveType) {
    const std::string configsPath =
        Paths::GetModuleFolder(Paths::GetOurModuleHandle()) +
        Constants::ModDir +
        "\\Configs";
    const std::string configFile = fmt::format("{}\\{}.ini", configsPath, newName);

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiLine(true);

    // This here MAY fail on first save, in which case, it can be ignored.
    // _Not_ having this just nukes the entire file, including any comments.
    SI_Error result = ini.LoadFile(configFile.c_str());
    if (result < 0) {
        logger.Write(WARN, "[Config] %s Failed to load, SI_Error [%d]. (No problem if no file exists yet)",
            configFile.c_str(), result);
    }

    // [ID]
    if (saveType != ESaveType::GenericNone) {
        if (model != 0) {
            ModelHash = model;
        }

        ini.SetValue("ID", "ModelHash", fmt::format("{:X}", ModelHash).c_str());

        auto& asCache = ASCache::Get();
        auto it = asCache.find(ModelHash);
        std::string modelName = it == asCache.end() ? std::string() : it->second;
        if (!modelName.empty()) {
            ModelName = modelName;
            ini.SetValue("ID", "ModelName", modelName.c_str());
        }

        if (saveType == ESaveType::Specific) {
            Plate = plate;
            ini.SetValue("ID", "Plate", plate.c_str());
        }
    }

    // [Data]
    SAVE_VAL("Data", "IdleRPM", Data.IdleRPM);
    SAVE_VAL("Data", "RevLimitRPM", Data.RevLimitRPM);

    std::string torqueMultMap = "<<<END_OF_MAP\n";
    for (auto [rpm, mult] : Data.TorqueMultMap) {
        torqueMultMap += fmt::format("{:.3f}|{:.3f}\n", rpm, mult);
    }
    torqueMultMap += "END_OF_MAP\n";

    ini.SetValue("Data", "TorqueMultMap", torqueMultMap.c_str());

    result = ini.SaveFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "save", configFile.c_str());
    if (result < 0)
        return false;
    return true;
}
