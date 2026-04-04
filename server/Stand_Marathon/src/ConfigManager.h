//Stand_Marathon/src/ConfigManager.h
#pragma once

#include "DataModel.h"
#include <string>

class ConfigManager {
public:
    explicit ConfigManager(const std::string& filePath);

    bool load(DataModel& data);
    bool save(const DataModel& data);

private:
    std::string path;

    int readInt(const std::string& section, const std::string& key, int defaultValue = 0);
    float readFloat(const std::string& section, const std::string& key, float defaultValue = 0.0f);
    bool readBool(const std::string& section, const std::string& key, bool defaultValue = false);
    std::string readString(const std::string& section, const std::string& key, const std::string& defaultValue = "");

    void writeInt(const std::string& section, const std::string& key, int value);
    void writeFloat(const std::string& section, const std::string& key, float value);
    void writeBool(const std::string& section, const std::string& key, bool value);
    void writeString(const std::string& section, const std::string& key, const std::string& value);
};
