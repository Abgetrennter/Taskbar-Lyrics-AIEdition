#include "configManager.hpp"
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include <Windows.h>

using json = nlohmann::json;

// --- JSON Conversion Functions ---

// FontConfig
void to_json(json& j, const FontConfig& p) {
    j = json{{"font_family", p.fontFamily}};
}
void from_json(const json& j, FontConfig& p) {
    if(j.contains("font_family")) j.at("font_family").get_to(p.fontFamily);
}

// ColorState
void to_json(json& j, const ColorState& p) {
    j = json{{"hex_color", p.hexColor}, {"opacity", p.opacity}};
}
void from_json(const json& j, ColorState& p) {
    if(j.contains("hex_color")) j.at("hex_color").get_to(p.hexColor);
    if(j.contains("opacity")) j.at("opacity").get_to(p.opacity);
}

// ColorMode
void to_json(json& j, const ColorMode& p) {
    j = json{{"light", p.light}, {"dark", p.dark}};
}
void from_json(const json& j, ColorMode& p) {
    if(j.contains("light")) j.at("light").get_to(p.light);
    if(j.contains("dark")) j.at("dark").get_to(p.dark);
}

// ColorConfig
void to_json(json& j, const ColorConfig& p) {
    j = json{{"basic", p.basic}, {"extra", p.extra}};
}
void from_json(const json& j, ColorConfig& p) {
    if(j.contains("basic")) j.at("basic").get_to(p.basic);
    if(j.contains("extra")) j.at("extra").get_to(p.extra);
}

// SizeConfig
void to_json(json& j, const SizeConfig& p) {
    j = json{{"basic", p.basic}, {"extra", p.extra}};
}
void from_json(const json& j, SizeConfig& p) {
    if(j.contains("basic")) j.at("basic").get_to(p.basic);
    if(j.contains("extra")) j.at("extra").get_to(p.extra);
}

// TextStyleState
void to_json(json& j, const TextStyleState& p) {
    j = json{
        {"weight", {{"value", p.weightValue}, {"textContent", p.weightText}}},
        {"slope", p.slope},
        {"underline", p.underline},
        {"strikethrough", p.strikethrough}
    };
}
void from_json(const json& j, TextStyleState& p) {
    if(j.contains("weight")) {
        auto& w = j["weight"];
        if(w.contains("value")) w.at("value").get_to(p.weightValue);
        if(w.contains("textContent")) w.at("textContent").get_to(p.weightText);
    }
    // Also support flat weightValue for backward compatibility if needed, 
    // but the JS sends {weight: {value: ...}}
    
    if(j.contains("slope")) j.at("slope").get_to(p.slope);
    if(j.contains("underline")) j.at("underline").get_to(p.underline);
    if(j.contains("strikethrough")) j.at("strikethrough").get_to(p.strikethrough);
}

// StyleConfig
void to_json(json& j, const StyleConfig& p) {
    j = json{{"basic", p.basic}, {"extra", p.extra}};
}
void from_json(const json& j, StyleConfig& p) {
    if(j.contains("basic")) j.at("basic").get_to(p.basic);
    if(j.contains("extra")) j.at("extra").get_to(p.extra);
}

// PositionConfig
void to_json(json& j, const PositionConfig& p) {
    j = json{{"position", {{"value", p.value}, {"textContent", p.textContent}}}};
}
void from_json(const json& j, PositionConfig& p) {
    if(j.contains("position")) {
        auto& pos = j["position"];
        if(pos.contains("value")) pos.at("value").get_to(p.value);
        if(pos.contains("textContent")) pos.at("textContent").get_to(p.textContent);
    }
}

// MarginConfig
void to_json(json& j, const MarginConfig& p) {
    j = json{{"left", p.left}, {"right", p.right}};
}
void from_json(const json& j, MarginConfig& p) {
    if(j.contains("left")) j.at("left").get_to(p.left);
    if(j.contains("right")) j.at("right").get_to(p.right);
}

// AlignConfig
void to_json(json& j, const AlignConfig& p) {
    j = json{{"basic", p.basic}, {"extra", p.extra}};
}
void from_json(const json& j, AlignConfig& p) {
    if(j.contains("basic")) j.at("basic").get_to(p.basic);
    if(j.contains("extra")) j.at("extra").get_to(p.extra);
}

// ScreenConfig
void to_json(json& j, const ScreenConfig& p) {
    j = json{{"parent_taskbar", {{"value", p.parentTaskbarValue}, {"textContent", p.parentTaskbarText}}}};
}
void from_json(const json& j, ScreenConfig& p) {
    if(j.contains("parent_taskbar")) {
        auto& pt = j["parent_taskbar"];
        if(pt.contains("value")) pt.at("value").get_to(p.parentTaskbarValue);
        if(pt.contains("textContent")) pt.at("textContent").get_to(p.parentTaskbarText);
    }
}

// AppConfig
void to_json(json& j, const AppConfig& p) {
    j = json{
        {"font", p.font},
        {"color", p.color},
        {"size", p.size},
        {"style", p.style},
        {"position", p.position},
        {"margin", p.margin},
        {"align", p.align},
        {"screen", p.screen}
    };
}
void from_json(const json& j, AppConfig& p) {
    if(j.contains("font")) j.at("font").get_to(p.font);
    if(j.contains("color")) j.at("color").get_to(p.color);
    if(j.contains("size")) j.at("size").get_to(p.size);
    if(j.contains("style")) j.at("style").get_to(p.style);
    if(j.contains("position")) j.at("position").get_to(p.position);
    if(j.contains("margin")) j.at("margin").get_to(p.margin);
    if(j.contains("align")) j.at("align").get_to(p.align);
    if(j.contains("screen")) j.at("screen").get_to(p.screen);
}

// --- Implementation ---

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    std::string dir = exePath.substr(0, exePath.find_last_of("\\"));
    m_configPath = dir + "\\config.json";
}

void ConfigManager::Load() {
    if (!std::filesystem::exists(m_configPath)) return;
    try {
        std::ifstream i(m_configPath);
        json j;
        i >> j;
        m_config = j.get<AppConfig>();
    } catch(...) {
        // Fallback to defaults if parsing fails
    }
}

void ConfigManager::Save() {
    std::ofstream o(m_configPath);
    json j = m_config;
    o << j.dump(4);
}

void ConfigManager::Reset() {
    m_config = AppConfig();
    Save();
}

std::string ConfigManager::GetJSON() {
    json j = m_config;
    return j.dump();
}

void ConfigManager::UpdateFromJSON(const std::string& jsonStr) {
    try {
        json j = json::parse(jsonStr);
        // Uses the custom from_json to partial merge
        j.get_to(m_config);
    } catch (...) {}
}
