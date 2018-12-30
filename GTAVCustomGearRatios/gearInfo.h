#pragma once
#include <inc/types.h>
#include <vector>

class GearInfo {
public:
    GearInfo();
    GearInfo(std::string description, std::string modelName, std::string licensePlate,
        uint8_t topGear, float driveMaxVel, std::vector<float> ratios);

    static GearInfo ParseConfig(const std::string& file);

//protected:
    std::string mDescription;
    std::string mModelName;
    std::string mLicensePlate;
    uint8_t mTopGear;
    float mDriveMaxVel;
    std::vector<float> mRatios;
    bool mParseError;
};
