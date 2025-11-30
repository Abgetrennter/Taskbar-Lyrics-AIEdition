#include "lyricsRenderer.hpp"
#include <algorithm> // for max

#pragma comment (lib, "d2d1.lib")
#pragma comment (lib, "dwrite.lib")

LyricsRenderer::LyricsRenderer(HWND* windowHandle)
{
    m_windowHandle = windowHandle;

    taskbarHandle = FindWindow(L"Shell_TrayWnd", NULL);
    notificationAreaHandle = FindWindowEx(taskbarHandle, NULL, L"TrayNotifyWnd", NULL);
    startButtonHandle = FindWindowEx(taskbarHandle, NULL, L"Start", NULL);
    HWND reBarWindow = FindWindowEx(taskbarHandle, NULL, L"ReBarWindow32", NULL);
    activeAreaHandle = FindWindowEx(reBarWindow, NULL, L"MSTaskSwWClass", NULL);

    // Create D2D Factory
    D2D1CreateFactory(
        D2D1_FACTORY_TYPE_SINGLE_THREADED,
        &m_d2dFactory
    );

    D2D1_RENDER_TARGET_PROPERTIES renderTargetProperties = D2D1::RenderTargetProperties();
    renderTargetProperties.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    renderTargetProperties.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;

    // Create DC Render Target
    m_d2dFactory->CreateDCRenderTarget(
        &renderTargetProperties,
        &m_d2dRenderTarget
    );

    // Create Solid Brush
    m_d2dRenderTarget->CreateSolidColorBrush(
        D2D1::ColorF(0x000000, 1),
        &m_d2dSolidBrush
    );

    // Create DWrite Factory
    DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(&m_dwriteFactory)
    );
}

LyricsRenderer::~LyricsRenderer()
{
    if (m_dwriteBasicTextFormat) { m_dwriteBasicTextFormat->Release(); m_dwriteBasicTextFormat = nullptr; }
    if (m_dwriteBasicTextLayout) { m_dwriteBasicTextLayout->Release(); m_dwriteBasicTextLayout = nullptr; }
    if (m_dwriteExtraTextFormat) { m_dwriteExtraTextFormat->Release(); m_dwriteExtraTextFormat = nullptr; }
    if (m_dwriteExtraTextLayout) { m_dwriteExtraTextLayout->Release(); m_dwriteExtraTextLayout = nullptr; }

    if (m_d2dFactory) { m_d2dFactory->Release(); m_d2dFactory = nullptr; }
    if (m_d2dRenderTarget) { m_d2dRenderTarget->Release(); m_d2dRenderTarget = nullptr; }
    if (m_d2dSolidBrush) { m_d2dSolidBrush->Release(); m_d2dSolidBrush = nullptr; }
    if (m_dwriteFactory) { m_dwriteFactory->Release(); m_dwriteFactory = nullptr; }

    m_windowHandle = nullptr;
}

void LyricsRenderer::updateWindow()
{
    GetWindowRect(taskbarHandle, &taskbarRect);
    GetWindowRect(notificationAreaHandle, &notificationAreaRect);
    GetWindowRect(startButtonHandle, &startButtonRect);
    GetWindowRect(activeAreaHandle, &activeAreaRect);

    long left = 0;
    long top = 0;
    long width = 0;
    long height = taskbarRect.bottom - taskbarRect.top;

    switch (windowAlignment)
    {
        case WindowAlignment::Adaptive:
        {
            if (isCentered)
            {
                left = static_cast<long>(hasComponentButton ? dpi(160) : 0) + leftMargin;
                width = startButtonRect.left - static_cast<long>(hasComponentButton ? dpi(160) : 0) - leftMargin - rightMargin;
            }
            else
            {
                left = activeAreaRect.right + leftMargin;
                width = notificationAreaRect.left - activeAreaRect.right - leftMargin - rightMargin;
            }
        }
        break;

        case WindowAlignment::Left:
        {
            if (isCentered)
            {
                left = static_cast<long>(hasComponentButton ? dpi(160) : 0) + leftMargin;
                width = startButtonRect.left - static_cast<long>(hasComponentButton ? dpi(160) : 0) - leftMargin - rightMargin;
            }
            else
            {
                left = 0 + leftMargin;
                width = notificationAreaRect.left - 0 - leftMargin - rightMargin;
            }
        }
        break;

        case WindowAlignment::Center:
        {
            int center = (taskbarRect.right - taskbarRect.left) / 2;
            int lw = activeAreaRect.right - startButtonRect.left;
            int rw = notificationAreaRect.right - notificationAreaRect.left;

            if (lw > rw)
            {
                left = lw + leftMargin;
                width = (center - lw) * 2 - leftMargin - rightMargin;
            }
            else
            {
                left = center - (center - rw) + leftMargin;
                width = (center - rw) * 2 - leftMargin - rightMargin;
            }
        }
        break;

        case WindowAlignment::Right:
        {
            left = activeAreaRect.right + leftMargin;
            width = notificationAreaRect.left - activeAreaRect.right - leftMargin - rightMargin;
        }
        break;
    }

    MoveWindow(*m_windowHandle, left, top, width, height, false);
    drawWindow(left, top, width, height);
}

void LyricsRenderer::drawWindow(long left, long top, long width, long height)
{
    RECT rect = {};
    GetClientRect(*m_windowHandle, &rect);

    HDC hdc = GetDC(*m_windowHandle);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP memBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP oldBitmap = HBITMAP(SelectObject(memDC, memBitmap));

    drawLyrics(memDC, rect);

    BLENDFUNCTION blend = {
        AC_SRC_OVER,
        0,
        255,
        AC_SRC_ALPHA
    };

    POINT targetPos = { left, top };
    SIZE size = { width, height };
    POINT sourcePos = { 0, 0 };

    UpdateLayeredWindow(*m_windowHandle, hdc, &targetPos, &size, memDC, &sourcePos, 0, &blend, ULW_ALPHA);

    SelectObject(memDC, oldBitmap);
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(*m_windowHandle, hdc);
}

void LyricsRenderer::updateResources(float width, float height)
{
    bool isDoubleLine = !extraLyrics.empty();
    bool layoutChanged = (width != cachedLayoutWidth || height != cachedLayoutHeight || isDoubleLine != cachedIsDoubleLine);

    // Check Basic Text Format
    bool basicFormatChanged = false;
    if (fontFamily != cachedFontFamily ||
        basicFontWeight != cachedBasicFontWeight ||
        basicFontStyle != cachedBasicFontStyle ||
        (isDoubleLine ? basicFontSizeDoubleLine : basicFontSize) != (isDoubleLine ? cachedBasicFontSizeDoubleLine : cachedBasicFontSize) ||
        !m_dwriteBasicTextFormat)
    {
        basicFormatChanged = true;
        if (m_dwriteBasicTextFormat) { m_dwriteBasicTextFormat->Release(); m_dwriteBasicTextFormat = nullptr; }

        m_dwriteFactory->CreateTextFormat(
            fontFamily.c_str(),
            nullptr,
            basicFontWeight,
            basicFontStyle,
            DWRITE_FONT_STRETCH_NORMAL,
            dpi(isDoubleLine ? basicFontSizeDoubleLine : basicFontSize),
            L"zh-CN",
            &m_dwriteBasicTextFormat
        );

        cachedFontFamily = fontFamily;
        cachedBasicFontWeight = basicFontWeight;
        cachedBasicFontStyle = basicFontStyle;
        if (isDoubleLine) cachedBasicFontSizeDoubleLine = basicFontSizeDoubleLine;
        else cachedBasicFontSize = basicFontSize;
    }

    // Check Basic Text Layout
    if (basicFormatChanged ||
        layoutChanged ||
        basicLyrics != cachedBasicLyrics ||
        basicTextAlign != cachedBasicTextAlign ||
        basicUnderline != cachedBasicUnderline ||
        basicStrikethrough != cachedBasicStrikethrough ||
        !m_dwriteBasicTextLayout)
    {
        if (m_dwriteBasicTextLayout) { m_dwriteBasicTextLayout->Release(); m_dwriteBasicTextLayout = nullptr; }

        float layoutWidth, layoutHeight;
        if (!isDoubleLine) {
            layoutWidth = (std::max)(0.0f, width - dpi(20));
            layoutHeight = (std::max)(0.0f, height - dpi(20));
        } else {
            layoutWidth = (std::max)(0.0f, width - dpi(10));
            layoutHeight = (std::max)(0.0f, height / 2.0f - dpi(5));
        }

        m_dwriteFactory->CreateTextLayout(
            basicLyrics.c_str(),
            (UINT32)basicLyrics.size(),
            m_dwriteBasicTextFormat,
            layoutWidth,
            layoutHeight,
            &m_dwriteBasicTextLayout
        );

        DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
        m_dwriteBasicTextLayout->SetTrimming(&trimming, nullptr);
        m_dwriteBasicTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_dwriteBasicTextLayout->SetTextAlignment(basicTextAlign);
        m_dwriteBasicTextLayout->SetUnderline(basicUnderline, DWRITE_TEXT_RANGE{ 0, (UINT32)basicLyrics.size() });
        m_dwriteBasicTextLayout->SetStrikethrough(basicStrikethrough, DWRITE_TEXT_RANGE{ 0, (UINT32)basicLyrics.size() });

        cachedBasicLyrics = basicLyrics;
        cachedBasicTextAlign = basicTextAlign;
        cachedBasicUnderline = basicUnderline;
        cachedBasicStrikethrough = basicStrikethrough;
    }

    // Check Extra Text Format & Layout (only if double line)
    if (isDoubleLine)
    {
        bool extraFormatChanged = false;
        if (fontFamily != cachedFontFamily || // Use same font family logic
            extraFontWeight != cachedExtraFontWeight ||
            extraFontStyle != cachedExtraFontStyle ||
            extraFontSize != cachedExtraFontSize ||
            !m_dwriteExtraTextFormat)
        {
            extraFormatChanged = true;
            if (m_dwriteExtraTextFormat) { m_dwriteExtraTextFormat->Release(); m_dwriteExtraTextFormat = nullptr; }

            m_dwriteFactory->CreateTextFormat(
                fontFamily.c_str(),
                nullptr,
                extraFontWeight,
                extraFontStyle,
                DWRITE_FONT_STRETCH_NORMAL,
                dpi(extraFontSize),
                L"zh-CN",
                &m_dwriteExtraTextFormat
            );

            cachedExtraFontWeight = extraFontWeight;
            cachedExtraFontStyle = extraFontStyle;
            cachedExtraFontSize = extraFontSize;
        }

        if (extraFormatChanged ||
            layoutChanged ||
            extraLyrics != cachedExtraLyrics ||
            extraTextAlign != cachedExtraTextAlign ||
            extraUnderline != cachedExtraUnderline ||
            extraStrikethrough != cachedExtraStrikethrough ||
            !m_dwriteExtraTextLayout)
        {
            if (m_dwriteExtraTextLayout) { m_dwriteExtraTextLayout->Release(); m_dwriteExtraTextLayout = nullptr; }

            float layoutWidth = (std::max)(0.0f, width - dpi(10));
            float layoutHeight = (std::max)(0.0f, height - height / 2.0f - dpi(5));

            m_dwriteFactory->CreateTextLayout(
                extraLyrics.c_str(),
                (UINT32)extraLyrics.size(),
                m_dwriteExtraTextFormat,
                layoutWidth,
                layoutHeight,
                &m_dwriteExtraTextLayout
            );

            DWRITE_TRIMMING trimming = { DWRITE_TRIMMING_GRANULARITY_CHARACTER, 0, 0 };
            m_dwriteExtraTextLayout->SetTrimming(&trimming, nullptr);
            m_dwriteExtraTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
            m_dwriteExtraTextLayout->SetTextAlignment(extraTextAlign);
            m_dwriteExtraTextLayout->SetUnderline(extraUnderline, DWRITE_TEXT_RANGE{ 0, (UINT32)extraLyrics.size() });
            m_dwriteExtraTextLayout->SetStrikethrough(extraStrikethrough, DWRITE_TEXT_RANGE{ 0, (UINT32)extraLyrics.size() });

            cachedExtraLyrics = extraLyrics;
            cachedExtraTextAlign = extraTextAlign;
            cachedExtraUnderline = extraUnderline;
            cachedExtraStrikethrough = extraStrikethrough;
        }
    }
    else
    {
        // Release extra resources if not needed
        if (m_dwriteExtraTextFormat) { m_dwriteExtraTextFormat->Release(); m_dwriteExtraTextFormat = nullptr; }
        if (m_dwriteExtraTextLayout) { m_dwriteExtraTextLayout->Release(); m_dwriteExtraTextLayout = nullptr; }
    }

    cachedLayoutWidth = width;
    cachedLayoutHeight = height;
    cachedIsDoubleLine = isDoubleLine;
}

void LyricsRenderer::drawLyrics(HDC& hdc, RECT& rect)
{
    m_d2dRenderTarget->BindDC(hdc, &rect);
    m_d2dRenderTarget->BeginDraw();
    m_d2dRenderTarget->Clear(D2D1::ColorF(0, 0.0f));

    updateResources((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

    // Draw Basic
    if (m_dwriteBasicTextLayout) {
        float left, top;
        if (!cachedIsDoubleLine) {
             left = (float)rect.left + dpi(10);
             top = (float)rect.top + dpi(10);
        } else {
             left = (float)rect.left + dpi(5);
             top = (float)rect.top + dpi(5);
        }
        
        m_d2dSolidBrush->SetColor(isLightMode ? basicLightColor : basicDarkColor);
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(left, top),
            m_dwriteBasicTextLayout,
            m_d2dSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );
    }

    // Draw Extra
    if (cachedIsDoubleLine && m_dwriteExtraTextLayout) {
        float left = (float)rect.left + dpi(5);
        float top = (float)rect.bottom / 2.0f;

        m_d2dSolidBrush->SetColor(isLightMode ? extraLightColor : extraDarkColor);
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(left, top),
            m_dwriteExtraTextLayout,
            m_d2dSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );
    }

    m_d2dRenderTarget->EndDraw();
}

float LyricsRenderer::dpi(float pixelSize)
{
    UINT screenDpi = GetDpiForWindow(*m_windowHandle);
    return static_cast<float>(pixelSize * screenDpi / 96);
}
