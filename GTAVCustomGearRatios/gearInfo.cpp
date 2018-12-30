#include "gearInfo.h"
#include <pugixml/pugixml.hpp>
#include <utility>
#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"
#include "../../GTAVManualTransmission/Gears/Util/StringFormat.h"

using namespace pugi;

#define VERIFY_NODE(node) \
    if (!node) {\
        logger.Write(ERROR, "[XML %s] Missing node %s", file.c_str(), #node);\
        return GearInfo();\
    }

GearInfo::GearInfo()
    : mDescription("Default Ctor - parsing error")
    , mTopGear(0)
    , mDriveMaxVel(0)
    , mParseError(true) {}

GearInfo::GearInfo(std::string description, std::string modelName, std::string licensePlate,
    uint8_t topGear, float driveMaxVel, std::vector<float> ratios)
    : mDescription(std::move(description))
    , mModelName(std::move(modelName))
    , mLicensePlate(std::move(licensePlate))
    , mTopGear(topGear)
    , mDriveMaxVel(driveMaxVel)
    , mRatios(std::move(ratios))
    , mParseError(false) {}

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
    std::vector<float> ratios(topGear);
    for (uint8_t gear = 0; gear <= topGear; ++gear) {
        xml_node gearNode = vehicleNode.child(fmt("Gear%d", gear).c_str());
        VERIFY_NODE(gearNode);
        ratios[gear] = gearNode.text().as_float();
    }

    return GearInfo(
        descriptionNode.text().as_string(),
        modelNameNode.text().as_string(),
        plateTextNode.text().as_string(),
        topGear,
        driveMaxVel,
        ratios
    );
}
