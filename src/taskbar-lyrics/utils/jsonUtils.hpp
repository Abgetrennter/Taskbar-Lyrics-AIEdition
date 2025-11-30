#pragma once

#include <string>

namespace JsonUtils
{
    std::string getString(const std::string& json, const std::string& key);
    int getInt(const std::string& json, const std::string& key);
    float getFloat(const std::string& json, const std::string& key);
    bool getBool(const std::string& json, const std::string& key);
    unsigned int getHex(const std::string& json, const std::string& key);
}
