#pragma once
#include <inc/types.h>
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
};

class GearInfo {
public:
    GearInfo();
    GearInfo(std::string description, std::string modelName, std::string licensePlate,
        uint8_t topGear, float driveMaxVel, std::vector<float> ratios, LoadType loadType);

    static GearInfo ParseConfig(const std::string& file);

    void SaveConfig(const std::string& file);

//protected:
    std::string mDescription;
    std::string mModelName;
    std::string mLicensePlate;
    uint8_t mTopGear;
    float mDriveMaxVel;
    std::vector<float> mRatios;
    bool mParseError;
    LoadType mLoadType;

    // For file management
    bool mMarkedForDeletion;
    std::string mPath;
};
