#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <string>

enum class WindowAlignment
{
    Adaptive = 0,
    Left = 1,
    Center = 2,
    Right = 3
};

class LyricsRenderer
{
public:
    LyricsRenderer(HWND* windowHandle);
    ~LyricsRenderer();

    void updateWindow();

    // Public properties to be set by NetworkServer
    bool isLightMode = false;
    bool hasComponentButton = false;
    bool isCentered = true;

    std::wstring basicLyrics = L"Taskbar Lyrics Started";
    std::wstring extraLyrics = L"Waiting for lyrics...";

    std::wstring fontFamily = L"Microsoft YaHei UI";

    D2D1::ColorF basicLightColor = D2D1::ColorF(0x000000, 1);
    D2D1::ColorF extraLightColor = D2D1::ColorF(0x000000, 1);
    D2D1::ColorF basicDarkColor = D2D1::ColorF(0xFFFFFF, 1);
    D2D1::ColorF extraDarkColor = D2D1::ColorF(0xFFFFFF, 1);

    DWRITE_FONT_WEIGHT basicFontWeight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_WEIGHT extraFontWeight = DWRITE_FONT_WEIGHT_NORMAL;
    DWRITE_FONT_STYLE basicFontStyle = DWRITE_FONT_STYLE_NORMAL;
    DWRITE_FONT_STYLE extraFontStyle = DWRITE_FONT_STYLE_NORMAL;
    
    bool basicUnderline = false;
    bool extraUnderline = false;
    bool basicStrikethrough = false;
    bool extraStrikethrough = false;

    float basicFontSize = 20.0f;
    float basicFontSizeDoubleLine = 15.0f;
    float extraFontSize = 15.0f;

    WindowAlignment windowAlignment = WindowAlignment::Adaptive;

    int leftMargin = 0;
    int rightMargin = 0;

    DWRITE_TEXT_ALIGNMENT basicTextAlign = DWRITE_TEXT_ALIGNMENT_LEADING;
    DWRITE_TEXT_ALIGNMENT extraTextAlign = DWRITE_TEXT_ALIGNMENT_LEADING;

    // Public handles accessed by WindowManager
    HWND taskbarHandle = nullptr;
    HWND notificationAreaHandle = nullptr;
    HWND startButtonHandle = nullptr;
    HWND activeAreaHandle = nullptr;

    RECT taskbarRect = {};
    RECT notificationAreaRect = {};
    RECT startButtonRect = {};
    RECT activeAreaRect = {};

private:
    HWND* m_windowHandle = nullptr;

    ID2D1Factory* m_d2dFactory = nullptr;
    ID2D1DCRenderTarget* m_d2dRenderTarget = nullptr;
    ID2D1SolidColorBrush* m_d2dSolidBrush = nullptr;

    IDWriteFactory* m_dwriteFactory = nullptr;
    IDWriteTextFormat* m_dwriteBasicTextFormat = nullptr;
    IDWriteTextLayout* m_dwriteBasicTextLayout = nullptr;
    IDWriteTextFormat* m_dwriteExtraTextFormat = nullptr;
    IDWriteTextLayout* m_dwriteExtraTextLayout = nullptr;

    // Cached state for TextFormat and TextLayout
    std::wstring cachedFontFamily;
    DWRITE_FONT_WEIGHT cachedBasicFontWeight;
    DWRITE_FONT_STYLE cachedBasicFontStyle;
    float cachedBasicFontSize = -1.0f;
    float cachedBasicFontSizeDoubleLine = -1.0f;
    
    DWRITE_FONT_WEIGHT cachedExtraFontWeight;
    DWRITE_FONT_STYLE cachedExtraFontStyle;
    float cachedExtraFontSize = -1.0f;

    std::wstring cachedBasicLyrics;
    std::wstring cachedExtraLyrics;
    DWRITE_TEXT_ALIGNMENT cachedBasicTextAlign;
    DWRITE_TEXT_ALIGNMENT cachedExtraTextAlign;
    bool cachedBasicUnderline;
    bool cachedExtraUnderline;
    bool cachedBasicStrikethrough;
    bool cachedExtraStrikethrough;
    
    float cachedLayoutWidth = -1.0f;
    float cachedLayoutHeight = -1.0f;
    bool cachedIsDoubleLine = false; // To track if we were in double line mode

    void updateResources(float width, float height);
    void drawWindow(long left, long top, long width, long height);
    void drawLyrics(HDC& hdc, RECT& rect);
    float dpi(float pixelSize);
};
