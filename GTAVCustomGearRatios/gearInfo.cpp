#include "gearInfo.h"
#include <utility>
#include <pugixml/pugixml.hpp>
#include <fmt/core.h>

#include "../../GTAVManualTransmission/Gears/Util/Logger.hpp"

using namespace pugi;

#define VERIFY_NODE(file, node, name) \
    if (!(node)) {\
        logger.Write(ERROR, "[XML %s] Missing node [%s]", (file), (name));\
        return GearInfo();\
    }

GearInfo::GearInfo()
    : Description("Default Ctor - parsing error")
    , TopGear(0)
    , DriveMaxVel(0)
    , ParseError(true)
    , LoadType(LoadType::None)
    , MarkedForDeletion(true) {}

GearInfo::GearInfo(std::string description, std::string modelName, std::string licensePlate,
    uint8_t topGear, float driveMaxVel, std::vector<float> ratios, enum class LoadType loadType)
    : Description(std::move(description))
    , ModelName(std::move(modelName))
    , LicensePlate(std::move(licensePlate))
    , TopGear(topGear)
    , DriveMaxVel(driveMaxVel)
    , Ratios(std::move(ratios))
    , ParseError(false)
    , LoadType(loadType)
    , MarkedForDeletion(false) {}

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
    VERIFY_NODE(file.c_str(), vehicleNode, "Vehicle");

    std::string nodeName = "Description";
    xml_node descriptionNode = vehicleNode.child(nodeName.c_str());
    VERIFY_NODE(file.c_str(), descriptionNode, nodeName.c_str());

    nodeName = "ModelName";
    xml_node modelNameNode = vehicleNode.child("ModelName");
    VERIFY_NODE(file.c_str(), modelNameNode, "ModelName");

    nodeName = "PlateText";
    xml_node plateTextNode = vehicleNode.child("PlateText");
    VERIFY_NODE(file.c_str(), plateTextNode, "PlateText");

    nodeName = "TopGear";
    xml_node topGearNode = vehicleNode.child("TopGear");
    VERIFY_NODE(file.c_str(), topGearNode, "TopGear");

    nodeName = "DriveMaxVel";
    xml_node driveMaxVelNode = vehicleNode.child("DriveMaxVel");
    VERIFY_NODE(file.c_str(), driveMaxVelNode, "DriveMaxVel");

    uint8_t topGear = topGearNode.text().as_int();
    float driveMaxVel = driveMaxVelNode.text().as_float();
    std::vector<float> ratios(topGear + 1);
    for (uint8_t gear = 0; gear <= topGear; ++gear) {
        nodeName = fmt::format("Gear{}", gear);
        xml_node gearNode = vehicleNode.child(nodeName.c_str());
        VERIFY_NODE(file.c_str(), gearNode, nodeName.c_str());
        ratios[gear] = gearNode.text().as_float();
    }

    enum class LoadType loadType = LoadType::Plate;

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

void GearInfo::SaveConfig(const GearInfo& gearInfo, const std::string& file) {
    xml_document doc;

    xml_node vehicleNode = doc.append_child("Vehicle");
    xml_node descriptionNode = vehicleNode.append_child("Description");
    xml_node modelNameNode = vehicleNode.append_child("ModelName");
    xml_node plateTextNode = vehicleNode.append_child("PlateText");
    xml_node topGearNode = vehicleNode.append_child("TopGear");
    xml_node driveMaxVelNode = vehicleNode.append_child("DriveMaxVel");

    descriptionNode.text() = gearInfo.Description.c_str();
    modelNameNode.text() = gearInfo.ModelName.c_str();
    plateTextNode.text() = gearInfo.LicensePlate.c_str();
    topGearNode.text() = fmt::format("{}", gearInfo.TopGear).c_str();
    driveMaxVelNode.text() = fmt::format("{}", gearInfo.DriveMaxVel).c_str();

    for (uint8_t gear = 0; gear < gearInfo.TopGear + 1; ++gear) {
        vehicleNode.append_child(fmt::format("Gear{}", gear).c_str()).text() = 
            fmt::format("{}", gearInfo.Ratios[gear]).c_str();
    }

    if (!doc.save_file(file.c_str())) {
        logger.Write(ERROR, "XML [%s] failed to save", file.c_str());
    }
}
