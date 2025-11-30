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

void LyricsRenderer::drawLyrics(HDC& hdc, RECT& rect)
{
    m_d2dRenderTarget->BindDC(hdc, &rect);
    m_d2dRenderTarget->BeginDraw();

    DWRITE_TRIMMING trimming = {
        DWRITE_TRIMMING_GRANULARITY_CHARACTER,
        0,
        0
    };

    if (extraLyrics.empty())
    {
        D2D1_RECT_F basicRect = D2D1::RectF(
            (std::max)(0.0f, rect.left + dpi(10)),
            (std::max)(0.0f, rect.top + dpi(10)),
            (std::max)(0.0f, rect.right - dpi(10)),
            (std::max)(0.0f, rect.bottom - dpi(10))
        );

        basicRect.right = (std::max)(basicRect.right, basicRect.left);
        basicRect.bottom = (std::max)(basicRect.bottom, basicRect.top);

        // Create Text Format
        m_dwriteFactory->CreateTextFormat(
            fontFamily.c_str(),
            nullptr,
            basicFontWeight,
            basicFontStyle,
            DWRITE_FONT_STRETCH_NORMAL,
            dpi(basicFontSize),
            L"zh-CN",
            &m_dwriteBasicTextFormat
        );

        // Create Text Layout
        m_dwriteFactory->CreateTextLayout(
            basicLyrics.c_str(),
            (UINT32)basicLyrics.size(),
            m_dwriteBasicTextFormat,
            (float)(basicRect.right - basicRect.left),
            (float)(basicRect.bottom - basicRect.top),
            &m_dwriteBasicTextLayout
        );

        m_dwriteBasicTextLayout->SetTrimming(&trimming, nullptr);
        m_dwriteBasicTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_dwriteBasicTextLayout->SetTextAlignment(basicTextAlign);
        m_dwriteBasicTextLayout->SetUnderline(basicUnderline, DWRITE_TEXT_RANGE{0, (UINT32)basicLyrics.size()});
        m_dwriteBasicTextLayout->SetStrikethrough(basicStrikethrough, DWRITE_TEXT_RANGE{0, (UINT32)basicLyrics.size()});
        m_d2dSolidBrush->SetColor(isLightMode ? basicLightColor : basicDarkColor);

        // Draw Text
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(basicRect.left, basicRect.top),
            m_dwriteBasicTextLayout,
            m_d2dSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );

        if (m_dwriteBasicTextFormat) { m_dwriteBasicTextFormat->Release(); m_dwriteBasicTextFormat = nullptr; }
        if (m_dwriteBasicTextLayout) { m_dwriteBasicTextLayout->Release(); m_dwriteBasicTextLayout = nullptr; }
    }
    else
    {
        D2D1_RECT_F basicRect = D2D1::RectF(
            (std::max)(0.0f, rect.left + dpi(5)),
            (std::max)(0.0f, rect.top + dpi(5)),
            (std::max)(0.0f, rect.right - dpi(5)),
            (std::max)(0.0f, rect.bottom / 2.0f)
        );

        basicRect.right = (std::max)(basicRect.right, basicRect.left);
        basicRect.bottom = (std::max)(basicRect.bottom, basicRect.top);

        // Create Text Format (Double Line)
        m_dwriteFactory->CreateTextFormat(
            fontFamily.c_str(),
            nullptr,
            basicFontWeight,
            basicFontStyle,
            DWRITE_FONT_STRETCH_NORMAL,
            dpi(basicFontSizeDoubleLine),
            L"zh-CN",
            &m_dwriteBasicTextFormat
        );

        // Create Text Layout
        m_dwriteFactory->CreateTextLayout(
            basicLyrics.c_str(),
            (UINT32)basicLyrics.size(),
            m_dwriteBasicTextFormat,
            (float)(basicRect.right - basicRect.left),
            (float)(basicRect.bottom - basicRect.top),
            &m_dwriteBasicTextLayout
        );

        m_dwriteBasicTextLayout->SetTrimming(&trimming, nullptr);
        m_dwriteBasicTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_dwriteBasicTextLayout->SetTextAlignment(basicTextAlign);
        m_dwriteBasicTextLayout->SetUnderline(basicUnderline, DWRITE_TEXT_RANGE{0, (UINT32)basicLyrics.size()});
        m_dwriteBasicTextLayout->SetStrikethrough(basicStrikethrough, DWRITE_TEXT_RANGE{0, (UINT32)basicLyrics.size()});
        m_d2dSolidBrush->SetColor(isLightMode ? basicLightColor : basicDarkColor);

        // Draw Text
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(basicRect.left, basicRect.top),
            m_dwriteBasicTextLayout,
            m_d2dSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );

        /******************************************/

        D2D1_RECT_F extraRect = D2D1::RectF(
            (std::max)(0.0f, rect.left + dpi(5)),
            (std::max)(0.0f, rect.bottom / 2.0f),
            (std::max)(0.0f, rect.right - dpi(5)),
            (std::max)(0.0f, rect.bottom - dpi(5))
        );

        extraRect.right = (std::max)(extraRect.right, extraRect.left);
        extraRect.bottom = (std::max)(extraRect.bottom, extraRect.top);

        // Create Text Format
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

        // Create Text Layout
        m_dwriteFactory->CreateTextLayout(
            extraLyrics.c_str(),
            (UINT32)extraLyrics.size(),
            m_dwriteExtraTextFormat,
            (float)(extraRect.right - extraRect.left),
            (float)(extraRect.bottom - extraRect.top),
            &m_dwriteExtraTextLayout
        );

        m_dwriteExtraTextLayout->SetTrimming(&trimming, nullptr);
        m_dwriteExtraTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
        m_dwriteExtraTextLayout->SetTextAlignment(extraTextAlign);
        m_dwriteExtraTextLayout->SetUnderline(extraUnderline, DWRITE_TEXT_RANGE{0, (UINT32)extraLyrics.size()});
        m_dwriteExtraTextLayout->SetStrikethrough(extraStrikethrough, DWRITE_TEXT_RANGE{0, (UINT32)extraLyrics.size()});
        
        m_d2dSolidBrush->SetColor(isLightMode ? extraLightColor : extraDarkColor);

        // Draw Text
        m_d2dRenderTarget->DrawTextLayout(
            D2D1::Point2F(extraRect.left, extraRect.top),
            m_dwriteExtraTextLayout,
            m_d2dSolidBrush,
            D2D1_DRAW_TEXT_OPTIONS_NO_SNAP
        );

        if (m_dwriteBasicTextFormat) { m_dwriteBasicTextFormat->Release(); m_dwriteBasicTextFormat = nullptr; }
        if (m_dwriteBasicTextLayout) { m_dwriteBasicTextLayout->Release(); m_dwriteBasicTextLayout = nullptr; }
        if (m_dwriteExtraTextFormat) { m_dwriteExtraTextFormat->Release(); m_dwriteExtraTextFormat = nullptr; }
        if (m_dwriteExtraTextLayout) { m_dwriteExtraTextLayout->Release(); m_dwriteExtraTextLayout = nullptr; }
    }

    m_d2dRenderTarget->EndDraw();
}

float LyricsRenderer::dpi(float pixelSize)
{
    UINT screenDpi = GetDpiForWindow(*m_windowHandle);
    return static_cast<float>(pixelSize * screenDpi / 96);
}
