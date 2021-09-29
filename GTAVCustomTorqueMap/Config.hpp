#pragma once

#include <inc/types.h>
#include <string>
#include <vector>
#include <map>

class CConfig {
public:
    enum class ESaveType {
        Specific,       // [ID] writes Model + Plate
        GenericModel,   // [ID] writes Model
        GenericNone,    // [ID] writes none
    };

    CConfig() = default;
    static CConfig Read(const std::string& configFile);

    void Write(ESaveType saveType);
    bool Write(const std::string& newName, Hash model, std::string plate, ESaveType saveType);

    std::string Name;

    Hash ModelHash;
    std::string ModelName;
    std::string Plate;

    // Data
    struct {
        // RPM, Mult
        // Size >= 2 required
        // Map[0     ] = (<any>, <any>)
        // Map[Size-1] = (1.0, <any>)
        std::map<float, float> TorqueMultMap;
    } Data;
};
