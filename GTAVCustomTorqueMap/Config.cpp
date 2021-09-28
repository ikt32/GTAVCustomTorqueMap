#include "Config.hpp"

CConfig CConfig::Read(const std::string& configFile) {
    return CConfig();
}

void CConfig::Write(ESaveType saveType) {
}

bool CConfig::Write(const std::string& newName, Hash model, std::string plate, ESaveType saveType) {
    return false;
}
