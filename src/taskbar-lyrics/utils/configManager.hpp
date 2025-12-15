#pragma once

#include <string>
#include <dwrite.h>
#include <d2d1.h>

struct FontConfig {
    std::string fontFamily = "Microsoft YaHei";
};

struct ColorState {
    unsigned int hexColor = 0xFFFFFF;
    float opacity = 1.0f;
};

struct ColorMode {
    ColorState light;
    ColorState dark;
};

struct ColorConfig {
    ColorMode basic;
    ColorMode extra;
};

struct SizeConfig {
    float basic = 20.0f;
    float extra = 16.0f;
};

struct TextStyleState {
    int weightValue = 400; // Normal
    std::string weightText = "Normal (400)";
    int slope = DWRITE_FONT_STYLE_NORMAL;
    bool underline = false;
    bool strikethrough = false;
};

struct StyleConfig {
    TextStyleState basic;
    TextStyleState extra;
};

struct PositionConfig {
    int value = 0; // Left
    std::string textContent = "左侧";
};

struct MarginConfig {
    int left = 0;
    int right = 0;
};

struct AlignConfig {
    int basic = DWRITE_TEXT_ALIGNMENT_LEADING;
    int extra = DWRITE_TEXT_ALIGNMENT_LEADING;
};

struct ScreenConfig {
    std::string parentTaskbarValue = "Shell_TrayWnd";
    std::string parentTaskbarText = "主屏幕";
};

struct AppConfig {
    FontConfig font;
    ColorConfig color;
    SizeConfig size;
    StyleConfig style;
    PositionConfig position;
    MarginConfig margin;
    AlignConfig align;
    ScreenConfig screen;
};

class ConfigManager {
public:
    static ConfigManager& getInstance();

    void Load();
    void Save();
    void Reset();
    
    std::string GetJSON();
    void UpdateFromJSON(const std::string& json);
    const AppConfig& GetConfig() const { return m_config; }
    AppConfig& GetConfigMutable() { return m_config; }

private:
    ConfigManager();
    ~ConfigManager() = default;

    std::string m_configPath;
    AppConfig m_config;
};
