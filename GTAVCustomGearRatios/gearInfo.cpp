#include "gearInfo.h"
#include <utility>
#include <pugixml/pugixml.hpp>
#include <fmt/core.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"

using namespace pugi;

#define VERIFY_NODE(node) \
    if (!(node)) {\
        logger.Write(ERROR, "[XML %s] Missing node %s", file.c_str(), #node);\
        return GearInfo();\
    }

GearInfo::GearInfo()
    : mDescription("Default Ctor - parsing error")
    , mTopGear(0)
    , mDriveMaxVel(0)
    , mParseError(true)
    , mLoadType(LoadType::None)
    , mMarkedForDeletion(true) {}

GearInfo::GearInfo(std::string description, std::string modelName, std::string licensePlate,
    uint8_t topGear, float driveMaxVel, std::vector<float> ratios, LoadType loadType)
    : mDescription(std::move(description))
    , mModelName(std::move(modelName))
    , mLicensePlate(std::move(licensePlate))
    , mTopGear(topGear)
    , mDriveMaxVel(driveMaxVel)
    , mRatios(std::move(ratios))
    , mParseError(false)
    , mLoadType(loadType)
    , mMarkedForDeletion(false) {}

GearInfo GearInfo::ParseConfig(const std::string& file) {
    xml_document doc;
    xml_parse_result result = doc.load_file(file.c_str());

    if (!result) {
        logger.Write(ERROR, "XML [%s] parsed with errors", file.c_str());
        logger.Write(ERROR, "    Error: %s", result.description());
        logger.Write(ERROR, "    Offset: %td", result.offset);
        return GearInfo();
    }

    xml_node vehicleNode = doc.child("Vehicle");
    VERIFY_NODE(vehicleNode);

    xml_node descriptionNode = vehicleNode.child("Description");
    xml_node modelNameNode = vehicleNode.child("ModelName");
    xml_node plateTextNode = vehicleNode.child("PlateText");
    xml_node topGearNode = vehicleNode.child("TopGear");
    xml_node driveMaxVelNode = vehicleNode.child("DriveMaxVel");

    VERIFY_NODE(descriptionNode);
    VERIFY_NODE(modelNameNode);
    VERIFY_NODE(plateTextNode);
    VERIFY_NODE(topGearNode);
    VERIFY_NODE(driveMaxVelNode);

    uint8_t topGear = topGearNode.text().as_int();
    float driveMaxVel = driveMaxVelNode.text().as_float();
    std::vector<float> ratios(topGear + 1);
    for (uint8_t gear = 0; gear <= topGear; ++gear) {
        xml_node gearNode = vehicleNode.child(fmt::format("Gear{}", gear).c_str());
        VERIFY_NODE(gearNode);
        ratios[gear] = gearNode.text().as_float();
    }

    LoadType loadType = LoadType::Plate;

    if (plateTextNode.text().as_string() == LoadName::None)
        loadType = LoadType::None;
    if (plateTextNode.text().as_string() == LoadName::Model)
        loadType = LoadType::Model;

    return GearInfo(
        descriptionNode.text().as_string(),
        modelNameNode.text().as_string(),
        plateTextNode.text().as_string(),
        topGear,
        driveMaxVel,
        ratios,
        loadType
    );
}

void GearInfo::SaveConfig(const std::string& file) {
    xml_document doc;

    xml_node vehicleNode = doc.append_child("Vehicle");
    xml_node descriptionNode = vehicleNode.append_child("Description");
    xml_node modelNameNode = vehicleNode.append_child("ModelName");
    xml_node plateTextNode = vehicleNode.append_child("PlateText");
    xml_node topGearNode = vehicleNode.append_child("TopGear");
    xml_node driveMaxVelNode = vehicleNode.append_child("DriveMaxVel");

    descriptionNode.text() = mDescription.c_str();
    modelNameNode.text() = mModelName.c_str();
    plateTextNode.text() = mLicensePlate.c_str();
    topGearNode.text() = fmt::format("{}", mTopGear).c_str();
    driveMaxVelNode.text() = fmt::format("{}", mDriveMaxVel).c_str();

    for (uint8_t gear = 0; gear < mTopGear + 1; ++gear) {
        vehicleNode.append_child(fmt::format("Gear{}", gear).c_str()).text() = 
            fmt::format("{}", mRatios[gear]).c_str();
    }

    if (!doc.save_file(file.c_str())) {
        logger.Write(ERROR, "XML [%s] failed to save", file.c_str());
    }
}
