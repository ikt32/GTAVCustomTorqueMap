#include "Config.hpp"

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

void SetValue(CSimpleIniA& ini, const char* section, const char* key, int val) {
    ini.SetLongValue(section, key, val);
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, const std::string& val) {
    ini.SetValue(section, key, val.c_str());
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, bool val) {
    ini.SetBoolValue(section, key, val);
}

void SetValue(CSimpleIniA& ini, const char* section, const char* key, float val) {
    ini.SetDoubleValue(section, key, static_cast<double>(val));
}

int GetValue(CSimpleIniA& ini, const char* section, const char* key, int val) {
    return ini.GetLongValue(section, key, val);
}

std::string GetValue(CSimpleIniA& ini, const char* section, const char* key, std::string val) {
    return ini.GetValue(section, key, val.c_str());
}

bool GetValue(CSimpleIniA& ini, const char* section, const char* key, bool val) {
    return ini.GetBoolValue(section, key, val);
}

float GetValue(CSimpleIniA& ini, const char* section, const char* key, float val) {
    return static_cast<float>(ini.GetDoubleValue(section, key, val));
}

CConfig CConfig::Read(const std::string& configFile) {
    CConfig config{};

    CSimpleIniA ini;
    ini.SetUnicode();
    ini.SetMultiLine(true);
    SI_Error result = ini.LoadFile(configFile.c_str());
    CHECK_LOG_SI_ERROR(result, "load", configFile.c_str());

    config.Name = std::filesystem::path(configFile).stem().string();

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

    // TODO: Verify torque mult map
    return config;
}

void CConfig::Write(ESaveType saveType) {
}

bool CConfig::Write(const std::string& newName, Hash model, std::string plate, ESaveType saveType) {
    return false;
}
