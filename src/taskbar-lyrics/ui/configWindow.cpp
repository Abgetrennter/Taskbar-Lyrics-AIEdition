#include <winsock2.h>
#include "configWindow.hpp"
#include "../utils/configManager.hpp"
#include "../core/taskbarLyrics.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// Control IDs
#define ID_BTN_SAVE 101
#define ID_BTN_CANCEL 102
#define ID_BTN_RESET 103
#define ID_TAB_CONTROL 1000

// Shared message (must match taskbarLyrics.hpp)
#ifndef WM_RELOAD_CONFIG
#define WM_RELOAD_CONFIG (WM_USER + 2)
#endif

// General page
#define ID_EDIT_PATH 2001
#define ID_EDIT_INTERVAL 2002
#define ID_EDIT_PARENT_TASKBAR 2003

// Appearance page
#define ID_BTN_FONT 3001
#define ID_EDIT_SIZE_BASIC 3002
#define ID_EDIT_SIZE_EXTRA 3003
#define ID_BTN_COLOR_LB 3004
#define ID_EDIT_OPACITY_LB 3005
#define ID_BTN_COLOR_DB 3006
#define ID_EDIT_OPACITY_DB 3007
#define ID_BTN_COLOR_LE 3008
#define ID_EDIT_OPACITY_LE 3009
#define ID_BTN_COLOR_DE 3010
#define ID_EDIT_OPACITY_DE 3011
#define ID_COMBO_WEIGHT_B 3012
#define ID_COMBO_SLOPE_B 3013
#define ID_CHECK_UNDERLINE_B 3014
#define ID_CHECK_STRIKE_B 3015
#define ID_COMBO_WEIGHT_E 3016
#define ID_COMBO_SLOPE_E 3017
#define ID_CHECK_UNDERLINE_E 3018
#define ID_CHECK_STRIKE_E 3019

// Lyrics page
#define ID_COMBO_RETRIEVAL 5001
#define ID_CHECK_KARAOKE 5002

// Effect page
#define ID_COMBO_NEXT_POS 6001
#define ID_COMBO_EXTRA_SHOW 6002
#define ID_EDIT_ADJUST 6003

// Layout page
#define ID_COMBO_POS 4001
#define ID_EDIT_MARGIN_L 4002
#define ID_EDIT_MARGIN_R 4003
#define ID_COMBO_ALIGN_B 4004
#define ID_COMBO_ALIGN_E 4005

#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

HWND ConfigWindow::s_hwnd = NULL;
HINSTANCE ConfigWindow::s_hInstance = NULL;
HWND ConfigWindow::s_hTab = NULL;
HWND ConfigWindow::s_hPageGeneral = NULL;
HWND ConfigWindow::s_hPageAppearance = NULL;
HWND ConfigWindow::s_hPageLyrics = NULL;
HWND ConfigWindow::s_hPageEffect = NULL;
HWND ConfigWindow::s_hPageLayout = NULL;
static HFONT s_hFont = NULL;

static COLORREF g_CustomColors[16] = { 0 };

static std::wstring StringToWString(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

static std::string WStringToString(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static std::wstring ColorToHex(DWORD color) {
    wchar_t buf[16];
    swprintf_s(buf, L"#%06X", color & 0xFFFFFF);
    return buf;
}

static DWORD HexToColor(const std::wstring& hex) {
    unsigned int r, g, b;
    if (swscanf_s(hex.c_str(), L"#%02x%02x%02x", &r, &g, &b) == 3) {
        return RGB(r, g, b);
    }
    return 0;
}

// Weight value to combo index: 100->0, 200->1, 300->2, 400->3, 500->4, 600->5, 700->6, 800->7, 900->8
static int WeightToIndex(int weight) {
    int idx = (weight / 100) - 1;
    if (idx < 0) idx = 3;
    if (idx > 8) idx = 8;
    return idx;
}

static int IndexToWeight(int idx) {
    if (idx < 0 || idx > 8) return 400;
    return (idx + 1) * 100;
}

void ConfigWindow::CreateLabel(HWND hParent, const wchar_t* text, int x, int y, int w, int h) {
    HWND hStatic = CreateWindow(L"STATIC", text, WS_VISIBLE | WS_CHILD,
        x, y, w, h, hParent, NULL, ConfigWindow::s_hInstance, NULL);
    SendMessage(hStatic, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

void ConfigWindow::Show(HINSTANCE hInstance) {
    if (s_hwnd) {
        SetForegroundWindow(s_hwnd);
        return;
    }

    s_hInstance = hInstance;

    if (!s_hFont) {
        s_hFont = CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Segoe UI");
    }

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TaskbarLyricsConfig";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wc);

    int width = 620;
    int height = 540;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    s_hwnd = CreateWindowEx(0, L"TaskbarLyricsConfig", L"Taskbar Lyrics 设置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height, NULL, NULL, hInstance, NULL);

    BOOL value = TRUE;
    DwmSetWindowAttribute(s_hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
}

bool ConfigWindow::IsOpen() {
    return s_hwnd != NULL;
}

void ConfigWindow::HandleColorBtn(HWND hBtn) {
    wchar_t buf[32];
    GetWindowText(hBtn, buf, 32);
    DWORD currentColor = HexToColor(buf);

    CHOOSECOLOR cc = { sizeof(CHOOSECOLOR) };
    cc.hwndOwner = ConfigWindow::s_hwnd;
    cc.lpCustColors = g_CustomColors;
    cc.rgbResult = currentColor;
    cc.Flags = CC_FULLOPEN | CC_RGBINIT;

    if (ChooseColor(&cc)) {
        int r = GetRValue(cc.rgbResult);
        int g = GetGValue(cc.rgbResult);
        int b = GetBValue(cc.rgbResult);
        swprintf_s(buf, L"#%02X%02X%02X", r, g, b);
        SetWindowText(hBtn, buf);
    }
}

LRESULT CALLBACK ConfigWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls(hwnd);
            LoadValues();
            break;
        case WM_NOTIFY: {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if (pnmh->code == TCN_SELCHANGE) {
                OnTabSelChanged();
            }
            break;
        }
        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_SAVE) {
                SaveValues(hwnd);
                DestroyWindow(hwnd);
            } else if (id == ID_BTN_CANCEL) {
                DestroyWindow(hwnd);
            } else if (id == ID_BTN_RESET) {
                if (MessageBox(hwnd, L"确定要恢复默认设置吗？", L"重置", MB_YESNO | MB_ICONQUESTION) == IDYES) {
                    ConfigManager::getInstance().Reset();
                    LoadValues();
                    // Notify reload
                    HWND hMsg = FindWindow(L"TaskbarLyricsTrayMsg", L"Taskbar Lyrics Tray");
                    if (hMsg) SendMessage(hMsg, WM_RELOAD_CONFIG, 0, 0);
                    MessageBox(hwnd, L"已恢复默认设置", L"成功", MB_OK | MB_ICONINFORMATION);
                }
            } else if (id == ID_BTN_FONT) {
                CHOOSEFONT cf = { sizeof(CHOOSEFONT) };
                LOGFONT lf = { 0 };
                cf.hwndOwner = hwnd;
                cf.lpLogFont = &lf;
                cf.Flags = CF_SCREENFONTS | CF_INITTOLOGFONTSTRUCT;

                wchar_t currentFont[100];
                GetDlgItemText(s_hPageAppearance, ID_BTN_FONT, currentFont, 100);
                wcscpy_s(lf.lfFaceName, currentFont);

                if (ChooseFont(&cf)) {
                    SetDlgItemText(s_hPageAppearance, ID_BTN_FONT, lf.lfFaceName);
                }
            } else if (id >= ID_BTN_COLOR_LB && id <= ID_BTN_COLOR_DE) {
                HandleColorBtn((HWND)lParam);
            }
            break;
        }
        case WM_DESTROY:
            if (s_hFont) {
                DeleteObject(s_hFont);
                s_hFont = NULL;
            }
            s_hwnd = NULL;
            s_hTab = NULL;
            s_hPageGeneral = NULL;
            s_hPageAppearance = NULL;
            s_hPageLyrics = NULL;
            s_hPageEffect = NULL;
            s_hPageLayout = NULL;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ConfigWindow::CreateControls(HWND hwnd) {
    CreateTabControl(hwnd);

    RECT rc;
    GetClientRect(s_hTab, &rc);
    TabCtrl_AdjustRect(s_hTab, FALSE, &rc);
    int pw = rc.right - rc.left;
    int ph = rc.bottom - rc.top;

    // Create pages as children of the tab control
    s_hPageGeneral = CreateWindow(L"STATIC", NULL, WS_CHILD | WS_VISIBLE,
        rc.left, rc.top, pw, ph, s_hTab, NULL, s_hInstance, NULL);
    CreateGeneralPage(s_hPageGeneral);

    s_hPageAppearance = CreateWindow(L"STATIC", NULL, WS_CHILD,
        rc.left, rc.top, pw, ph, s_hTab, NULL, s_hInstance, NULL);
    CreateAppearancePage(s_hPageAppearance);

    s_hPageLyrics = CreateWindow(L"STATIC", NULL, WS_CHILD,
        rc.left, rc.top, pw, ph, s_hTab, NULL, s_hInstance, NULL);
    CreateLyricsPage(s_hPageLyrics);

    s_hPageEffect = CreateWindow(L"STATIC", NULL, WS_CHILD,
        rc.left, rc.top, pw, ph, s_hTab, NULL, s_hInstance, NULL);
    CreateEffectPage(s_hPageEffect);

    s_hPageLayout = CreateWindow(L"STATIC", NULL, WS_CHILD,
        rc.left, rc.top, pw, ph, s_hTab, NULL, s_hInstance, NULL);
    CreateLayoutPage(s_hPageLayout);

    // Save / Cancel / Reset buttons
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int btnW = 100;
    int btnH = 32;
    int btnY = rcClient.bottom - btnH - 12;
    int btnStart = (rcClient.right - (btnW * 3 + 20)) / 2;

    HWND hBtnReset = CreateWindow(L"BUTTON", L"恢复默认", WS_VISIBLE | WS_CHILD,
        btnStart, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_RESET, s_hInstance, NULL);
    SendMessage(hBtnReset, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hBtnSave = CreateWindow(L"BUTTON", L"保存", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        btnStart + btnW + 10, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_SAVE, s_hInstance, NULL);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hBtnCancel = CreateWindow(L"BUTTON", L"取消", WS_VISIBLE | WS_CHILD,
        btnStart + (btnW + 10) * 2, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_CANCEL, s_hInstance, NULL);
    SendMessage(hBtnCancel, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

void ConfigWindow::CreateTabControl(HWND hwnd) {
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);

    s_hTab = CreateWindow(WC_TABCONTROL, L"", WS_CHILD | WS_CLIPSIBLINGS | WS_VISIBLE,
        10, 10, rcClient.right - 20, rcClient.bottom - 60, hwnd, (HMENU)ID_TAB_CONTROL, s_hInstance, NULL);
    SendMessage(s_hTab, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    TCITEM tie;
    tie.mask = TCIF_TEXT;

    tie.pszText = (LPWSTR)L"常规";
    TabCtrl_InsertItem(s_hTab, 0, &tie);

    tie.pszText = (LPWSTR)L"外观";
    TabCtrl_InsertItem(s_hTab, 1, &tie);

    tie.pszText = (LPWSTR)L"歌词";
    TabCtrl_InsertItem(s_hTab, 2, &tie);

    tie.pszText = (LPWSTR)L"效果";
    TabCtrl_InsertItem(s_hTab, 3, &tie);

    tie.pszText = (LPWSTR)L"布局";
    TabCtrl_InsertItem(s_hTab, 4, &tie);
}

void ConfigWindow::OnTabSelChanged() {
    int iPage = TabCtrl_GetCurSel(s_hTab);
    ShowWindow(s_hPageGeneral, iPage == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageAppearance, iPage == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageLyrics, iPage == 2 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageEffect, iPage == 3 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageLayout, iPage == 4 ? SW_SHOW : SW_HIDE);
}

// --- General Page ---
void ConfigWindow::CreateGeneralPage(HWND hParent) {
    int margin = 20;
    int labelW = 150;
    int inputW = 300;
    int h = 25;
    int y = margin;

    CreateLabel(hParent, L"一言 JSON 路径:", margin, y, labelW, h);
    HWND hEditPath = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_EDIT_PATH, s_hInstance, NULL);
    SendMessage(hEditPath, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"轮换间隔(秒):", margin, y, labelW, h);
    HWND hEditInt = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 100, h, hParent, (HMENU)ID_EDIT_INTERVAL, s_hInstance, NULL);
    SendMessage(hEditInt, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"父任务栏类名:", margin, y, labelW, h);
    HWND hEditParent = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_EDIT_PARENT_TASKBAR, s_hInstance, NULL);
    SendMessage(hEditParent, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

// --- Appearance Page ---
void ConfigWindow::CreateAppearancePage(HWND hParent) {
    int margin = 15;
    int labelW = 120;
    int inputW = 200;
    int h = 25;
    int y = margin;

    // Font
    CreateLabel(hParent, L"字体:", margin, y, labelW, h);
    HWND hBtnFont = CreateWindow(L"BUTTON", L"选择字体...", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_BTN_FONT, s_hInstance, NULL);
    SendMessage(hBtnFont, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 15;
    CreateLabel(hParent, L"主歌词大小:", margin, y, labelW, h);
    HWND hEditSB = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_SIZE_BASIC, s_hInstance, NULL);
    SendMessage(hEditSB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 15;
    CreateLabel(hParent, L"副歌词大小:", margin, y, labelW, h);
    HWND hEditSE = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_SIZE_EXTRA, s_hInstance, NULL);
    SendMessage(hEditSE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    // Colors
    y += h + 20;
    int colorBtnW = 80;
    int opacityW = 50;

    const struct { const wchar_t* label; int colorId; int opacityId; } colors[] = {
        { L"主歌词(浅色):", ID_BTN_COLOR_LB, ID_EDIT_OPACITY_LB },
        { L"主歌词(深色):", ID_BTN_COLOR_DB, ID_EDIT_OPACITY_DB },
        { L"副歌词(浅色):", ID_BTN_COLOR_LE, ID_EDIT_OPACITY_LE },
        { L"副歌词(深色):", ID_BTN_COLOR_DE, ID_EDIT_OPACITY_DE },
    };
    for (auto& c : colors) {
        CreateLabel(hParent, c.label, margin, y, labelW, h);
        HWND hBtnColor = CreateWindow(L"BUTTON", L"#000000", WS_VISIBLE | WS_CHILD,
            margin + labelW, y, colorBtnW, h, hParent, (HMENU)c.colorId, s_hInstance, NULL);
        SendMessage(hBtnColor, WM_SETFONT, (WPARAM)s_hFont, TRUE);

        HWND hEditOpacity = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER,
            margin + labelW + colorBtnW + 5, y, opacityW, h, hParent, (HMENU)c.opacityId, s_hInstance, NULL);
        SendMessage(hEditOpacity, WM_SETFONT, (WPARAM)s_hFont, TRUE);
        y += h + 12;
    }

    // Styles
    y += 8;
    const wchar_t* weights[] = {
        L"Thin (100)", L"ExtraLight (200)", L"Light (300)", L"Normal (400)",
        L"Medium (500)", L"SemiBold (600)", L"Bold (700)", L"ExtraBold (800)", L"Black (900)"
    };
    const wchar_t* slopes[] = { L"Normal", L"Oblique", L"Italic" };

    const struct { const wchar_t* label; int weightId; int slopeId; int ulId; int strikeId; } styles[] = {
        { L"主歌词样式:", ID_COMBO_WEIGHT_B, ID_COMBO_SLOPE_B, ID_CHECK_UNDERLINE_B, ID_CHECK_STRIKE_B },
        { L"副歌词样式:", ID_COMBO_WEIGHT_E, ID_COMBO_SLOPE_E, ID_CHECK_UNDERLINE_E, ID_CHECK_STRIKE_E },
    };
    for (auto& s : styles) {
        CreateLabel(hParent, s.label, margin, y, labelW, h);

        HWND hComboW = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            margin + labelW, y, 130, 300, hParent, (HMENU)s.weightId, s_hInstance, NULL);
        SendMessage(hComboW, WM_SETFONT, (WPARAM)s_hFont, TRUE);
        for (auto w : weights) SendMessage(hComboW, CB_ADDSTRING, 0, (LPARAM)w);

        HWND hComboSlope = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
            margin + labelW + 135, y, 85, 200, hParent, (HMENU)s.slopeId, s_hInstance, NULL);
        SendMessage(hComboSlope, WM_SETFONT, (WPARAM)s_hFont, TRUE);
        for (auto sl : slopes) SendMessage(hComboSlope, CB_ADDSTRING, 0, (LPARAM)sl);

        y += h + 5;
        CreateLabel(hParent, L"", margin, y, labelW, h);
        HWND hCheckU = CreateWindow(L"BUTTON", L"下划线", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            margin + labelW, y, 70, h, hParent, (HMENU)s.ulId, s_hInstance, NULL);
        SendMessage(hCheckU, WM_SETFONT, (WPARAM)s_hFont, TRUE);

        HWND hCheckS = CreateWindow(L"BUTTON", L"删除线", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
            margin + labelW + 80, y, 70, h, hParent, (HMENU)s.strikeId, s_hInstance, NULL);
        SendMessage(hCheckS, WM_SETFONT, (WPARAM)s_hFont, TRUE);

        y += h + 15;
    }
}

// --- Lyrics Page ---
void ConfigWindow::CreateLyricsPage(HWND hParent) {
    int margin = 20;
    int labelW = 150;
    int inputW = 250;
    int h = 25;
    int y = margin;

    CreateLabel(hParent, L"歌词获取方式:", margin, y, labelW, h);
    HWND hComboRetrieval = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_RETRIEVAL, s_hInstance, NULL);
    SendMessage(hComboRetrieval, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboRetrieval, CB_ADDSTRING, 0, (LPARAM)L"监听内置歌词");
    SendMessage(hComboRetrieval, CB_ADDSTRING, 0, (LPARAM)L"LibLyric 解析");
    SendMessage(hComboRetrieval, CB_ADDSTRING, 0, (LPARAM)L"RefinedNowPlaying");

    y += h + 25;
    HWND hCheckKaraoke = CreateWindow(L"BUTTON", L"逐字歌词（卡拉OK模式）", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin, y, 280, h, hParent, (HMENU)ID_CHECK_KARAOKE, s_hInstance, NULL);
    SendMessage(hCheckKaraoke, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

// --- Effect Page ---
void ConfigWindow::CreateEffectPage(HWND hParent) {
    int margin = 20;
    int labelW = 150;
    int inputW = 250;
    int h = 25;
    int y = margin;

    CreateLabel(hParent, L"下句歌词位置:", margin, y, labelW, h);
    HWND hComboNextPos = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_NEXT_POS, s_hInstance, NULL);
    SendMessage(hComboNextPos, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboNextPos, CB_ADDSTRING, 0, (LPARAM)L"副歌词显示下句");
    SendMessage(hComboNextPos, CB_ADDSTRING, 0, (LPARAM)L"主歌词显示下句");
    SendMessage(hComboNextPos, CB_ADDSTRING, 0, (LPARAM)L"交错式");

    y += h + 25;
    CreateLabel(hParent, L"副歌词显示:", margin, y, labelW, h);
    HWND hComboExtraShow = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_EXTRA_SHOW, s_hInstance, NULL);
    SendMessage(hComboExtraShow, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboExtraShow, CB_ADDSTRING, 0, (LPARAM)L"隐藏");
    SendMessage(hComboExtraShow, CB_ADDSTRING, 0, (LPARAM)L"下句歌词");
    SendMessage(hComboExtraShow, CB_ADDSTRING, 0, (LPARAM)L"当前翻译");
    SendMessage(hComboExtraShow, CB_ADDSTRING, 0, (LPARAM)L"当前音译");

    y += h + 25;
    CreateLabel(hParent, L"延迟调整(秒):", margin, y, labelW, h);
    HWND hEditAdjust = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW, y, 100, h, hParent, (HMENU)ID_EDIT_ADJUST, s_hInstance, NULL);
    SendMessage(hEditAdjust, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

// --- Layout Page ---
void ConfigWindow::CreateLayoutPage(HWND hParent) {
    int margin = 20;
    int labelW = 120;
    int inputW = 200;
    int h = 25;
    int y = margin;

    // Position
    CreateLabel(hParent, L"窗口位置:", margin, y, labelW, h);
    HWND hComboPos = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_POS, s_hInstance, NULL);
    SendMessage(hComboPos, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"自动");
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"靠左");
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"居中");
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"靠右");

    y += h + 25;
    CreateLabel(hParent, L"左边距:", margin, y, labelW, h);
    HWND hEditML = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_MARGIN_L, s_hInstance, NULL);
    SendMessage(hEditML, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 25;
    CreateLabel(hParent, L"右边距:", margin, y, labelW, h);
    HWND hEditMR = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_MARGIN_R, s_hInstance, NULL);
    SendMessage(hEditMR, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 30;
    CreateLabel(hParent, L"主歌词对齐:", margin, y, labelW, h);
    HWND hComboAB = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_ALIGN_B, s_hInstance, NULL);
    SendMessage(hComboAB, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"左对齐");
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"右对齐");
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"居中");

    y += h + 25;
    CreateLabel(hParent, L"副歌词对齐:", margin, y, labelW, h);
    HWND hComboAE = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_ALIGN_E, s_hInstance, NULL);
    SendMessage(hComboAE, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"左对齐");
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"右对齐");
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"居中");
}

// --- Load ---
void ConfigWindow::LoadValues() {
    LoadGeneralValues(s_hPageGeneral);
    LoadAppearanceValues(s_hPageAppearance);
    LoadLyricsValues(s_hPageLyrics);
    LoadEffectValues(s_hPageEffect);
    LoadLayoutValues(s_hPageLayout);
}

void ConfigWindow::LoadGeneralValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();
    std::wstring path = StringToWString(config.hitokoto.jsonPath);
    SetDlgItemText(hPage, ID_EDIT_PATH, path.c_str());
    SetDlgItemInt(hPage, ID_EDIT_INTERVAL, config.hitokoto.interval, FALSE);
    std::wstring parentClass = StringToWString(config.screen.parentTaskbarValue);
    SetDlgItemText(hPage, ID_EDIT_PARENT_TASKBAR, parentClass.c_str());
}

void ConfigWindow::LoadAppearanceValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();

    // Font
    std::wstring font = StringToWString(config.font.fontFamily);
    SetDlgItemText(hPage, ID_BTN_FONT, font.c_str());

    // Sizes
    wchar_t buf[32];
    swprintf_s(buf, L"%.1f", config.size.basic);
    SetDlgItemText(hPage, ID_EDIT_SIZE_BASIC, buf);
    swprintf_s(buf, L"%.1f", config.size.extra);
    SetDlgItemText(hPage, ID_EDIT_SIZE_EXTRA, buf);

    // Colors
    struct { unsigned int hex; float opacity; int colorId; int opacityId; } colors[] = {
        { config.color.basic.light.hexColor, config.color.basic.light.opacity, ID_BTN_COLOR_LB, ID_EDIT_OPACITY_LB },
        { config.color.basic.dark.hexColor, config.color.basic.dark.opacity, ID_BTN_COLOR_DB, ID_EDIT_OPACITY_DB },
        { config.color.extra.light.hexColor, config.color.extra.light.opacity, ID_BTN_COLOR_LE, ID_EDIT_OPACITY_LE },
        { config.color.extra.dark.hexColor, config.color.extra.dark.opacity, ID_BTN_COLOR_DE, ID_EDIT_OPACITY_DE },
    };
    for (auto& c : colors) {
        SetDlgItemText(hPage, c.colorId, ColorToHex(c.hex).c_str());
        swprintf_s(buf, L"%.2f", c.opacity);
        SetDlgItemText(hPage, c.opacityId, buf);
    }

    // Weight
    SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_B), CB_SETCURSEL, WeightToIndex(config.style.basic.weightValue), 0);
    SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_E), CB_SETCURSEL, WeightToIndex(config.style.extra.weightValue), 0);

    // Slope (combo: 0=Normal, 1=Oblique, 2=Italic)
    SendMessage(GetDlgItem(hPage, ID_COMBO_SLOPE_B), CB_SETCURSEL, config.style.basic.slope, 0);
    SendMessage(GetDlgItem(hPage, ID_COMBO_SLOPE_E), CB_SETCURSEL, config.style.extra.slope, 0);

    // Underline / Strikethrough
    CheckDlgButton(hPage, ID_CHECK_UNDERLINE_B, config.style.basic.underline ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_STRIKE_B, config.style.basic.strikethrough ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_UNDERLINE_E, config.style.extra.underline ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_STRIKE_E, config.style.extra.strikethrough ? BST_CHECKED : BST_UNCHECKED);
}

void ConfigWindow::LoadLyricsValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();

    // Retrieval method (combo index matches value: 0, 1, 2)
    SendMessage(GetDlgItem(hPage, ID_COMBO_RETRIEVAL), CB_SETCURSEL, config.lyrics.retrievalMethodValue, 0);
    CheckDlgButton(hPage, ID_CHECK_KARAOKE, config.lyrics.karaoke ? BST_CHECKED : BST_UNCHECKED);
}

void ConfigWindow::LoadEffectValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();

    SendMessage(GetDlgItem(hPage, ID_COMBO_NEXT_POS), CB_SETCURSEL, config.effect.nextLinePositionValue, 0);
    SendMessage(GetDlgItem(hPage, ID_COMBO_EXTRA_SHOW), CB_SETCURSEL, config.effect.extraShowValue, 0);

    wchar_t buf[32];
    swprintf_s(buf, L"%.1f", config.effect.adjust);
    SetDlgItemText(hPage, ID_EDIT_ADJUST, buf);
}

void ConfigWindow::LoadLayoutValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();

    // Position: combo index = position value (0=Auto, 1=Left, 2=Center, 3=Right)
    SendMessage(GetDlgItem(hPage, ID_COMBO_POS), CB_SETCURSEL, config.position.value, 0);

    SetDlgItemInt(hPage, ID_EDIT_MARGIN_L, config.margin.left, FALSE);
    SetDlgItemInt(hPage, ID_EDIT_MARGIN_R, config.margin.right, FALSE);

    // Alignment: DWRITE_TEXT_ALIGNMENT_LEADING=0, TRAILING=1, CENTER=2
    SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_B), CB_SETCURSEL, config.align.basic, 0);
    SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_E), CB_SETCURSEL, config.align.extra, 0);
}

// --- Save ---
void ConfigWindow::SaveValues(HWND hwnd) {
    // Ensure all pages are visible so saving can read hidden page controls
    ShowWindow(s_hPageGeneral, SW_SHOW);
    ShowWindow(s_hPageAppearance, SW_SHOW);
    ShowWindow(s_hPageLyrics, SW_SHOW);
    ShowWindow(s_hPageEffect, SW_SHOW);
    ShowWindow(s_hPageLayout, SW_SHOW);


    // Now save all values from visible pages
    SaveGeneralValues(s_hPageGeneral);
    SaveAppearanceValues(s_hPageAppearance);
    SaveLyricsValues(s_hPageLyrics);
    SaveEffectValues(s_hPageEffect);
    SaveLayoutValues(s_hPageLayout);

    ConfigManager::getInstance().Save();

    // Notify TaskbarLyrics to reload
    HWND hMsg = FindWindow(L"TaskbarLyricsTrayMsg", L"Taskbar Lyrics Tray");
    if (hMsg) {
        SendMessage(hMsg, WM_RELOAD_CONFIG, 0, 0);
    }

    MessageBox(hwnd, L"设置已保存", L"成功", MB_OK | MB_ICONINFORMATION);
}

void ConfigWindow::SaveGeneralValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    wchar_t path[MAX_PATH];
    GetDlgItemText(hPage, ID_EDIT_PATH, path, MAX_PATH);
    config.hitokoto.jsonPath = WStringToString(path);

    BOOL success;
    int interval = GetDlgItemInt(hPage, ID_EDIT_INTERVAL, &success, FALSE);
    if (success && interval > 0) config.hitokoto.interval = interval;

    wchar_t parent[256];
    GetDlgItemText(hPage, ID_EDIT_PARENT_TASKBAR, parent, 256);
    config.screen.parentTaskbarValue = WStringToString(parent);
}

void ConfigWindow::SaveAppearanceValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    // Font
    wchar_t font[100];
    GetDlgItemText(hPage, ID_BTN_FONT, font, 100);
    config.font.fontFamily = WStringToString(font);

    // Sizes
    wchar_t buf[32];
    GetDlgItemText(hPage, ID_EDIT_SIZE_BASIC, buf, 32);
    try { config.size.basic = std::stof(WStringToString(buf)); } catch (...) {}
    GetDlgItemText(hPage, ID_EDIT_SIZE_EXTRA, buf, 32);
    try { config.size.extra = std::stof(WStringToString(buf)); } catch (...) {}

    // Colors
    wchar_t hex[32], opacity[32];

    GetDlgItemText(hPage, ID_BTN_COLOR_LB, hex, 32);
    config.color.basic.light.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_LB, opacity, 32);
    try { config.color.basic.light.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_DB, hex, 32);
    config.color.basic.dark.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_DB, opacity, 32);
    try { config.color.basic.dark.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_LE, hex, 32);
    config.color.extra.light.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_LE, opacity, 32);
    try { config.color.extra.light.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_DE, hex, 32);
    config.color.extra.dark.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_DE, opacity, 32);
    try { config.color.extra.dark.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    // Weight
    const char* weightTexts[] = { "Thin (100)", "ExtraLight (200)", "Light (300)", "Normal (400)",
                                  "Medium (500)", "SemiBold (600)", "Bold (700)", "ExtraBold (800)", "Black (900)" };

    int weightIdxB = SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_B), CB_GETCURSEL, 0, 0);
    if (weightIdxB != CB_ERR && weightIdxB >= 0 && weightIdxB < 9) {
        config.style.basic.weightValue = IndexToWeight(weightIdxB);
        config.style.basic.weightText = weightTexts[weightIdxB];
    }
    int slopeIdxB = SendMessage(GetDlgItem(hPage, ID_COMBO_SLOPE_B), CB_GETCURSEL, 0, 0);
    if (slopeIdxB != CB_ERR && slopeIdxB >= 0 && slopeIdxB <= 2) {
        config.style.basic.slope = slopeIdxB;
    }
    config.style.basic.underline = IsDlgButtonChecked(hPage, ID_CHECK_UNDERLINE_B) == BST_CHECKED;
    config.style.basic.strikethrough = IsDlgButtonChecked(hPage, ID_CHECK_STRIKE_B) == BST_CHECKED;

    int weightIdxE = SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_E), CB_GETCURSEL, 0, 0);
    if (weightIdxE != CB_ERR && weightIdxE >= 0 && weightIdxE < 9) {
        config.style.extra.weightValue = IndexToWeight(weightIdxE);
        config.style.extra.weightText = weightTexts[weightIdxE];
    }
    int slopeIdxE = SendMessage(GetDlgItem(hPage, ID_COMBO_SLOPE_E), CB_GETCURSEL, 0, 0);
    if (slopeIdxE != CB_ERR && slopeIdxE >= 0 && slopeIdxE <= 2) {
        config.style.extra.slope = slopeIdxE;
    }
    config.style.extra.underline = IsDlgButtonChecked(hPage, ID_CHECK_UNDERLINE_E) == BST_CHECKED;
    config.style.extra.strikethrough = IsDlgButtonChecked(hPage, ID_CHECK_STRIKE_E) == BST_CHECKED;
}

void ConfigWindow::SaveLyricsValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    int retrieval = SendMessage(GetDlgItem(hPage, ID_COMBO_RETRIEVAL), CB_GETCURSEL, 0, 0);
    if (retrieval != CB_ERR) {
        config.lyrics.retrievalMethodValue = retrieval;
        const char* texts[] = { "监听内置歌词", "使用LibLyric解析获取歌词", "使用RefinedNowPlaying歌词" };
        if (retrieval >= 0 && retrieval < 3) config.lyrics.retrievalMethodText = texts[retrieval];
    }
    config.lyrics.karaoke = IsDlgButtonChecked(hPage, ID_CHECK_KARAOKE) == BST_CHECKED;
}

void ConfigWindow::SaveEffectValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    int nextPos = SendMessage(GetDlgItem(hPage, ID_COMBO_NEXT_POS), CB_GETCURSEL, 0, 0);
    if (nextPos != CB_ERR) {
        config.effect.nextLinePositionValue = nextPos;
        const char* texts[] = { "副歌词，下句歌词显示在这", "主歌词，下句歌词显示在这", "交错式，就跟桌面歌词一样" };
        if (nextPos >= 0 && nextPos < 3) config.effect.nextLinePositionText = texts[nextPos];
    }

    int extraShow = SendMessage(GetDlgItem(hPage, ID_COMBO_EXTRA_SHOW), CB_GETCURSEL, 0, 0);
    if (extraShow != CB_ERR) {
        config.effect.extraShowValue = extraShow;
        const char* texts[] = { "隐藏歌词，让歌词独占两行", "下句歌词，没则用上个选项", "当前翻译，没则用上个选项", "当前音译，没则用上个选项" };
        if (extraShow >= 0 && extraShow < 4) config.effect.extraShowText = texts[extraShow];
    }

    wchar_t buf[32];
    GetDlgItemText(hPage, ID_EDIT_ADJUST, buf, 32);
    try { config.effect.adjust = std::stof(WStringToString(buf)); } catch (...) {}
}

void ConfigWindow::SaveLayoutValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    int pos = SendMessage(GetDlgItem(hPage, ID_COMBO_POS), CB_GETCURSEL, 0, 0);
    if (pos != CB_ERR) {
        config.position.value = pos;
        const char* texts[] = { "自动，自适应选择左或右", "靠左，占满剩余空间宽度", "中间，基于任务栏的宽度", "靠右，占满剩余空间宽度" };
        if (pos >= 0 && pos < 4) config.position.textContent = texts[pos];
    }

    BOOL success;
    int ml = GetDlgItemInt(hPage, ID_EDIT_MARGIN_L, &success, FALSE);
    if (success) config.margin.left = ml;
    int mr = GetDlgItemInt(hPage, ID_EDIT_MARGIN_R, &success, FALSE);
    if (success) config.margin.right = mr;

    int ab = SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_B), CB_GETCURSEL, 0, 0);
    if (ab != CB_ERR) config.align.basic = ab;
    int ae = SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_E), CB_GETCURSEL, 0, 0);
    if (ae != CB_ERR) config.align.extra = ae;
}
