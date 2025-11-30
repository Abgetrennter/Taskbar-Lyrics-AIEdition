#include "jsonUtils.hpp"
#include <algorithm>

namespace JsonUtils
{
    std::string getString(const std::string& json, const std::string& key)
    {
        std::string keyPattern = "\"" + key + "\":";
        size_t pos = json.find(keyPattern);
        if (pos == std::string::npos) return "";
        
        pos += keyPattern.length();
        while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) pos++;
        
        if (pos >= json.length() || json[pos] != '"') return "";
        pos++; 
        
        size_t endPos = pos;
        while (endPos < json.length()) {
            if (json[endPos] == '"' && (endPos == 0 || json[endPos - 1] != '\\')) break;
            endPos++;
        }
        
        std::string result = json.substr(pos, endPos - pos);
        size_t found = result.find("\\\"");
        while (found != std::string::npos) {
            result.replace(found, 2, "\"");
            found = result.find("\\\"", found + 1);
        }
        return result;
    }

    int getInt(const std::string& json, const std::string& key)
    {
        std::string keyPattern = "\"" + key + "\":";
        size_t pos = json.find(keyPattern);
        if (pos == std::string::npos) return 0;
        pos += keyPattern.length();
        while (pos < json.length() && !isdigit(json[pos]) && json[pos] != '-') pos++;
        return std::atoi(json.c_str() + pos);
    }

    float getFloat(const std::string& json, const std::string& key)
    {
        std::string keyPattern = "\"" + key + "\":";
        size_t pos = json.find(keyPattern);
        if (pos == std::string::npos) return 0.0f;
        pos += keyPattern.length();
        while (pos < json.length() && !isdigit(json[pos]) && json[pos] != '-' && json[pos] != '.') pos++;
        return (float)std::atof(json.c_str() + pos);
    }

    bool getBool(const std::string& json, const std::string& key)
    {
        std::string keyPattern = "\"" + key + "\":";
        size_t pos = json.find(keyPattern);
        if (pos == std::string::npos) return false;
        pos += keyPattern.length();
        while (pos < json.length() && isspace(json[pos])) pos++;
        if (json.substr(pos, 4) == "true") return true;
        return false;
    }

    unsigned int getHex(const std::string& json, const std::string& key)
    {
        return (unsigned int)getInt(json, key);
    }
}
