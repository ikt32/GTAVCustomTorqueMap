#pragma once
#include <inc/natives.h>
#include <string>
#include <vector>

enum class LoadType {
    Plate,
    Model,
    None
};

namespace LoadName {
    static std::string Model   = "autoload_model";
    static std::string None    = "undefined";
}

struct GearInfo {
    static GearInfo ParseConfig(const std::string& file);
    static void SaveConfig(const GearInfo& gearInfo, const std::string& file);

    GearInfo();
    GearInfo(std::string description, std::string modelName, Hash hash, std::string licensePlate,
        uint8_t topGear, float driveMaxVel, std::vector<float> ratios, enum class LoadType loadType);


    std::string Description;
    std::string ModelName;
    Hash ModelHash;
    std::string LicensePlate;
    uint8_t TopGear;
    float DriveMaxVel;
    std::vector<float> Ratios;
    bool ParseError;
    enum class LoadType LoadType;

    // For file management
    bool MarkedForDeletion;
    std::string Path;
};
