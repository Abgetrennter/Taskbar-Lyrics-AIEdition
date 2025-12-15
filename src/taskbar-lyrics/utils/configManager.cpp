#include "configManager.hpp"
#include "jsonUtils.hpp"
#include "../utils/logger.hpp"
#include <fstream>
#include <sstream>
#include <iomanip>

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    // Get executable path
    char buffer[MAX_PATH];
    GetModuleFileNameA(NULL, buffer, MAX_PATH);
    std::string exePath(buffer);
    std::string dir = exePath.substr(0, exePath.find_last_of("\\"));
    m_configPath = dir + "\\config.json";
}

void ConfigManager::Load() {
    std::ifstream file(m_configPath);
    if (!file.is_open()) {
        Logger::Info("Config file not found, using defaults.");
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    UpdateFromJSON(json);
    Logger::Info("Config loaded from %s", m_configPath.c_str());
}

// Helper to manual stringify since we don't have a json library
std::string quote(const std::string& s) {
    return "\"" + s + "\"";
}

void ConfigManager::Save() {
    std::ofstream file(m_configPath);
    if (!file.is_open()) {
        Logger::Error("Failed to save config to %s", m_configPath.c_str());
        return;
    }

    file << GetJSON();
    Logger::Info("Config saved.");
}

std::string ConfigManager::GetJSON() {
    std::stringstream ss;
    ss << "{"
       << "\"font\":{\"font_family\":" << quote(m_config.font.fontFamily) << "},"
       
       << "\"color\":{"
       << "\"basic\":{"
           << "\"light\":{\"hex_color\":" << m_config.color.basic.light.hexColor << ",\"opacity\":" << m_config.color.basic.light.opacity << "},"
           << "\"dark\":{\"hex_color\":" << m_config.color.basic.dark.hexColor << ",\"opacity\":" << m_config.color.basic.dark.opacity << "}"
       << "},"
       << "\"extra\":{"
           << "\"light\":{\"hex_color\":" << m_config.color.extra.light.hexColor << ",\"opacity\":" << m_config.color.extra.light.opacity << "},"
           << "\"dark\":{\"hex_color\":" << m_config.color.extra.dark.hexColor << ",\"opacity\":" << m_config.color.extra.dark.opacity << "}"
       << "}"
       << "},"

       << "\"size\":{\"basic\":" << m_config.size.basic << ",\"extra\":" << m_config.size.extra << "},"

       << "\"style\":{"
       << "\"basic\":{"
           << "\"weight\":{\"value\":" << m_config.style.basic.weightValue << ",\"textContent\":" << quote(m_config.style.basic.weightText) << "},"
           << "\"slope\":" << m_config.style.basic.slope << ","
           << "\"underline\":" << (m_config.style.basic.underline ? "true" : "false") << ","
           << "\"strikethrough\":" << (m_config.style.basic.strikethrough ? "true" : "false")
       << "},"
       << "\"extra\":{"
           << "\"weight\":{\"value\":" << m_config.style.extra.weightValue << ",\"textContent\":" << quote(m_config.style.extra.weightText) << "},"
           << "\"slope\":" << m_config.style.extra.slope << ","
           << "\"underline\":" << (m_config.style.extra.underline ? "true" : "false") << ","
           << "\"strikethrough\":" << (m_config.style.extra.strikethrough ? "true" : "false")
       << "}"
       << "},"

       << "\"position\":{\"position\":{\"value\":" << m_config.position.value << ",\"textContent\":" << quote(m_config.position.textContent) << "}},"
       
       << "\"margin\":{\"left\":" << m_config.margin.left << ",\"right\":" << m_config.margin.right << "},"
       
       << "\"align\":{\"basic\":" << m_config.align.basic << ",\"extra\":" << m_config.align.extra << "},"

       << "\"screen\":{\"parent_taskbar\":{\"value\":" << quote(m_config.screen.parentTaskbarValue) << ",\"textContent\":" << quote(m_config.screen.parentTaskbarText) << "}}"
       
       << "}";
    
    return ss.str();
}

static std::string getScope(const std::string& j, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t start = j.find(search);
    if (start == std::string::npos) return "";
    start += search.length();
    
    // Find start of value
    while (start < j.length() && (j[start] == ' ' || j[start] == '\n' || j[start] == '\r' || j[start] == '\t')) {
        start++;
    }
    
    if (start >= j.length()) return "";

    if (j[start] != '{') {
        // Not an object, return primitive value up to comma or closing brace
        // This helper is intended for object scopes, but let's be safe
        return "";
    }

    size_t brace = start;
    int depth = 1;
    size_t pos = brace + 1;
    while (pos < j.length() && depth > 0) {
        if (j[pos] == '{') depth++;
        else if (j[pos] == '}') depth--;
        pos++;
    }
    return j.substr(brace, pos - brace);
}

void ConfigManager::UpdateFromJSON(const std::string& json) {
    // Font
    std::string fontScope = getScope(json, "font");
    if (!fontScope.empty()) {
        m_config.font.fontFamily = JsonUtils::getString(fontScope, "font_family");
    } else if (json.find("font_family") != std::string::npos) {
        // Fallback for flat JSON
        m_config.font.fontFamily = JsonUtils::getString(json, "font_family");
    }

    // Color
    std::string colorScope = getScope(json, "color");
    if (!colorScope.empty()) {
        std::string basicScope = getScope(colorScope, "basic");
        if (!basicScope.empty()) {
            std::string lightScope = getScope(basicScope, "light");
            if (!lightScope.empty()) {
                m_config.color.basic.light.hexColor = JsonUtils::getHex(lightScope, "hex_color");
                m_config.color.basic.light.opacity = JsonUtils::getFloat(lightScope, "opacity");
            }
            std::string darkScope = getScope(basicScope, "dark");
            if (!darkScope.empty()) {
                m_config.color.basic.dark.hexColor = JsonUtils::getHex(darkScope, "hex_color");
                m_config.color.basic.dark.opacity = JsonUtils::getFloat(darkScope, "opacity");
            }
        }
        std::string extraScope = getScope(colorScope, "extra");
        if (!extraScope.empty()) {
            std::string lightScope = getScope(extraScope, "light");
            if (!lightScope.empty()) {
                m_config.color.extra.light.hexColor = JsonUtils::getHex(lightScope, "hex_color");
                m_config.color.extra.light.opacity = JsonUtils::getFloat(lightScope, "opacity");
            }
            std::string darkScope = getScope(extraScope, "dark");
            if (!darkScope.empty()) {
                m_config.color.extra.dark.hexColor = JsonUtils::getHex(darkScope, "hex_color");
                m_config.color.extra.dark.opacity = JsonUtils::getFloat(darkScope, "opacity");
            }
        }
    } else {
        // Fallback for flat keys (used by legacy handlers if they were to use this function, but they don't anymore)
        // But checking just in case
        if (json.find("basic_light_hex_color") != std::string::npos) m_config.color.basic.light.hexColor = JsonUtils::getHex(json, "basic_light_hex_color");
        // ... omitted for brevity as we are moving to nested
    }

    // Size
    std::string sizeScope = getScope(json, "size");
    if (!sizeScope.empty()) {
        m_config.size.basic = JsonUtils::getFloat(sizeScope, "basic");
        m_config.size.extra = JsonUtils::getFloat(sizeScope, "extra");
    }

    // Align
    std::string alignScope = getScope(json, "align");
    if (!alignScope.empty()) {
        m_config.align.basic = JsonUtils::getInt(alignScope, "basic");
        m_config.align.extra = JsonUtils::getInt(alignScope, "extra");
    }

    // Margin
    std::string marginScope = getScope(json, "margin");
    if (!marginScope.empty()) {
        m_config.margin.left = JsonUtils::getInt(marginScope, "left");
        m_config.margin.right = JsonUtils::getInt(marginScope, "right");
    }

    // Style
    std::string styleScope = getScope(json, "style");
    if (!styleScope.empty()) {
        auto parseStyle = [&](std::string scope, TextStyleState& state) {
            std::string wScope = getScope(scope, "weight");
            if (!wScope.empty()) {
                state.weightValue = JsonUtils::getInt(wScope, "value");
                state.weightText = JsonUtils::getString(wScope, "textContent");
            } else if (JsonUtils::getInt(scope, "weightValue") > 0) {
                 state.weightValue = JsonUtils::getInt(scope, "weightValue");
            }
            state.slope = JsonUtils::getInt(scope, "slope");
            state.underline = JsonUtils::getBool(scope, "underline");
            state.strikethrough = JsonUtils::getBool(scope, "strikethrough");
        };
        
        std::string basicScope = getScope(styleScope, "basic");
        if (!basicScope.empty()) parseStyle(basicScope, m_config.style.basic);
        
        std::string extraScope = getScope(styleScope, "extra");
        if (!extraScope.empty()) parseStyle(extraScope, m_config.style.extra);
    }

    // Position
    std::string posScope = getScope(json, "position");
    if (!posScope.empty()) {
        std::string pScope = getScope(posScope, "position");
        if (!pScope.empty()) {
            m_config.position.value = JsonUtils::getInt(pScope, "value");
            m_config.position.textContent = JsonUtils::getString(pScope, "textContent");
        }
    }

    // Screen
    std::string screenScope = getScope(json, "screen");
    if (!screenScope.empty()) {
        std::string ptScope = getScope(screenScope, "parent_taskbar");
        if (!ptScope.empty()) {
            m_config.screen.parentTaskbarValue = JsonUtils::getString(ptScope, "value");
            m_config.screen.parentTaskbarText = JsonUtils::getString(ptScope, "textContent");
        }
    }
}

void ConfigManager::Reset() {
    m_config = AppConfig(); // Reset to default constructed values
    Save();
}
