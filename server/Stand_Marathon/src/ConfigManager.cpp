//Stand_Marathon/src/ConfigManager.cpp
#include "ConfigManager.h"
#include <windows.h>
#include <sstream>
#include <algorithm>

ConfigManager::ConfigManager(const std::string& filePath)
    : path(filePath) {}

static std::string fixLocaleNumber(std::string s) {
    std::replace(s.begin(), s.end(), ',', '.');
    return s;
}

int ConfigManager::readInt(const std::string& section, const std::string& key, int defaultValue) {
    return GetPrivateProfileIntA(section.c_str(), key.c_str(), defaultValue, path.c_str());
}

float ConfigManager::readFloat(const std::string& section, const std::string& key, float defaultValue) {
    char buffer[64];
    GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buffer, sizeof(buffer), path.c_str());
    if (buffer[0] == '\0') return defaultValue;
    try {
        return std::stof(fixLocaleNumber(buffer));
    } catch (...) {
        return defaultValue;
    }
}

bool ConfigManager::readBool(const std::string& section, const std::string& key, bool defaultValue) {
    char buffer[16];
    GetPrivateProfileStringA(section.c_str(), key.c_str(), "", buffer, sizeof(buffer), path.c_str());
    std::string val(buffer);
    std::transform(val.begin(), val.end(), val.begin(), ::tolower);
    if (val == "true" || val == "1") return true;
    if (val == "false" || val == "0") return false;
    return defaultValue;
}

std::string ConfigManager::readString(const std::string& section, const std::string& key, const std::string& defaultValue) {
    char buffer[256];
    GetPrivateProfileStringA(section.c_str(), key.c_str(), defaultValue.c_str(), buffer, sizeof(buffer), path.c_str());
    return std::string(buffer);
}

void ConfigManager::writeInt(const std::string& section, const std::string& key, int value) {
    WritePrivateProfileStringA(section.c_str(), key.c_str(), std::to_string(value).c_str(), path.c_str());
}

void ConfigManager::writeFloat(const std::string& section, const std::string& key, float value) {
    std::ostringstream oss;
    oss.imbue(std::locale::classic());
    oss << value;
    WritePrivateProfileStringA(section.c_str(), key.c_str(), oss.str().c_str(), path.c_str());
}

void ConfigManager::writeBool(const std::string& section, const std::string& key, bool value) {
    WritePrivateProfileStringA(section.c_str(), key.c_str(), value ? "TRUE" : "FALSE", path.c_str());
}

void ConfigManager::writeString(const std::string& section, const std::string& key, const std::string& value) {
    WritePrivateProfileStringA(section.c_str(), key.c_str(), value.c_str(), path.c_str());
}


bool ConfigManager::load(DataModel& d) {
    // --- [Path_Settings] ---
    d.configFilePath = readString("Path_Settings", "Config_File", d.configFilePath);

    // --- [Can] ---
    d.canBoard   = readInt("Can", "Can-Board", 0);
    d.canChannel = readInt("Can", "Can-Ch", 0);
    d.canBaud    = readInt("Can", "Can-Baud", 0);
    d.canFlags   = readInt("Can", "Can-Flags", 0);
    d.acode      = std::stoul(readString("Can", "ACode-Filter", "0x0032"), nullptr, 16);
    d.amask      = std::stoul(readString("Can", "AMask-Filter", "0x00000000"), nullptr, 16);

    // --- [Motor] ---
    d.Kl_15        = readBool("Motor", "Kl_15", false);
    d.En_rem       = readBool("Motor", "En_rem", false);
    d.Dampf_U      = readInt("Motor", "Dampf_U", 0);
    d.Brake_active = readBool("Motor", "Brake active", false);
    d.TCS_active   = readBool("Motor", "TCS activ", false);

    d.M_min        = readFloat("Motor", "М_min", -10.0f);
    d.M_grad_max   = readFloat("Motor", "М_grad_max", 4000.0f);
    d.M_max        = readFloat("Motor", "М_max", 10.0f);
    d.dM_damp_Ctrl = readFloat("Motor", "dМ_damp_Ctrl", 0.1f);
    d.i_R          = readFloat("Motor", "i_R", 9.78f);
    d.En_Is        = readBool("Motor", "En_Is", false);
    d.Isd        = readFloat("Motor", "Is_sd", 0.0f);
    d.Isq        = readFloat("Motor", "Is_sq", 0.0f);
    d.n_max        = readFloat("Motor", "n_max", 1000.0f);
    d.T_Str_max    = readFloat("Motor", "T_Str_max", 85.0f);

    return true;
}


bool ConfigManager::save(const DataModel& d) {
    writeString("Path_Settings", "Config_File", d.configFilePath);

    writeInt("Can", "Can-Board", d.canBoard);
    writeInt("Can", "Can-Ch", d.canChannel);
    writeInt("Can", "Can-Baud", d.canBaud);
    writeInt("Can", "Can-Flags", d.canFlags);
    auto to_hex8 = [](uint32_t v){
        char buf[11];
        std::snprintf(buf, sizeof(buf), "0x%08X", v);
        return std::string(buf);
    };
    writeString("Can", "ACode-Filter", to_hex8(d.acode));
    writeString("Can", "AMask-Filter", to_hex8(d.amask));


    writeBool("Motor", "Kl_15", d.Kl_15);
    writeBool("Motor", "En_rem", d.En_rem);
    writeInt("Motor", "Dampf_U", d.Dampf_U);
    writeBool("Motor", "Brake active", d.Brake_active);
    writeBool("Motor", "TCS activ", d.TCS_active);

    writeFloat("Motor", "М_min", d.M_min);
    writeFloat("Motor", "М_grad_max", d.M_grad_max);
    writeFloat("Motor", "М_max", d.M_max);
    writeFloat("Motor", "dМ_damp_Ctrl", d.dM_damp_Ctrl);
    writeFloat("Motor", "i_R", d.i_R);
    writeBool("Motor", "En_Is", d.En_Is);
    writeFloat("Motor", "Is_sd", d.Isd);
    writeFloat("Motor", "Is_sq", d.Isq);
    writeFloat("Motor", "n_max", d.n_max);
    writeFloat("Motor", "T_Str_max", d.T_Str_max);

    return true;
}

