#include "configWindow.hpp"
#include "../utils/configManager.hpp"
#include <commctrl.h>
#include <commdlg.h>
#include <dwmapi.h>
#include <stdio.h>
#include <string>
#include <vector>

#pragma comment(lib, "Comctl32.lib")
#pragma comment(lib, "Dwmapi.lib")
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

// IDs
#define ID_BTN_SAVE 101
#define ID_BTN_CANCEL 102
#define ID_TAB_CONTROL 1000

#define WM_RELOAD_CONFIG (WM_USER + 2)

// General
#define ID_EDIT_PATH 2001
#define ID_EDIT_INTERVAL 2002

// Appearance
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
#define ID_CHECK_ITALIC_B 3013
#define ID_CHECK_UNDERLINE_B 3014
#define ID_CHECK_STRIKE_B 3015

#define ID_COMBO_WEIGHT_E 3016
#define ID_CHECK_ITALIC_E 3017
#define ID_CHECK_UNDERLINE_E 3018
#define ID_CHECK_STRIKE_E 3019

// General (Added)
#define ID_EDIT_PARENT_TASKBAR 2003

// Layout
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
HWND ConfigWindow::s_hPageLayout = NULL;
HFONT s_hFont = NULL;

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

    int width = 600;
    int height = 500;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    s_hwnd = CreateWindowEx(0, L"TaskbarLyricsConfig", L"Taskbar Lyrics Settings",
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
        // Convert back to Hex string #RRGGBB
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
    
    // Create Pages
    s_hPageGeneral = CreateWindow(L"STATIC", NULL, WS_CHILD | WS_VISIBLE, 
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, s_hTab, NULL, s_hInstance, NULL);
    CreateGeneralPage(s_hPageGeneral);

    s_hPageAppearance = CreateWindow(L"STATIC", NULL, WS_CHILD, 
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, s_hTab, NULL, s_hInstance, NULL);
    CreateAppearancePage(s_hPageAppearance);

    s_hPageLayout = CreateWindow(L"STATIC", NULL, WS_CHILD, 
        rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, s_hTab, NULL, s_hInstance, NULL);
    CreateLayoutPage(s_hPageLayout);

    // Initial Hide
    ShowWindow(s_hPageAppearance, SW_HIDE);
    ShowWindow(s_hPageLayout, SW_HIDE);

    // Save/Cancel Buttons
    RECT rcClient;
    GetClientRect(hwnd, &rcClient);
    int btnW = 100;
    int btnH = 32;
    int btnY = rcClient.bottom - btnH - 15;
    int btnStart = (rcClient.right - (btnW * 2 + 20)) / 2;

    HWND hBtnSave = CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        btnStart, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_SAVE, s_hInstance, NULL);
    SendMessage(hBtnSave, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hBtnCancel = CreateWindow(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD,
        btnStart + btnW + 20, btnY, btnW, btnH, hwnd, (HMENU)ID_BTN_CANCEL, s_hInstance, NULL);
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
    
    tie.pszText = (LPWSTR)L"General";
    TabCtrl_InsertItem(s_hTab, 0, &tie);
    
    tie.pszText = (LPWSTR)L"Appearance";
    TabCtrl_InsertItem(s_hTab, 1, &tie);

    tie.pszText = (LPWSTR)L"Layout";
    TabCtrl_InsertItem(s_hTab, 2, &tie);
}

void ConfigWindow::OnTabSelChanged() {
    int iPage = TabCtrl_GetCurSel(s_hTab);
    ShowWindow(s_hPageGeneral, iPage == 0 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageAppearance, iPage == 1 ? SW_SHOW : SW_HIDE);
    ShowWindow(s_hPageLayout, iPage == 2 ? SW_SHOW : SW_HIDE);
}

void ConfigWindow::CreateGeneralPage(HWND hParent) {
    int margin = 20;
    int labelW = 150;
    int inputW = 300;
    int h = 25;
    int y = margin;

    CreateLabel(hParent, L"Hitokoto JSON Path:", margin, y, labelW, h);
    HWND hEditPath = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_EDIT_PATH, s_hInstance, NULL);
    SendMessage(hEditPath, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Interval (seconds):", margin, y, labelW, h);
    HWND hEditInt = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 100, h, hParent, (HMENU)ID_EDIT_INTERVAL, s_hInstance, NULL);
    SendMessage(hEditInt, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Parent Class:", margin, y, labelW, h);
    HWND hEditParent = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_EDIT_PARENT_TASKBAR, s_hInstance, NULL);
    SendMessage(hEditParent, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

void ConfigWindow::CreateAppearancePage(HWND hParent) {
    int margin = 20;
    int labelW = 120;
    int inputW = 200;
    int h = 25;
    int y = margin;

    // Font
    CreateLabel(hParent, L"Font Family:", margin, y, labelW, h);
    HWND hBtnFont = CreateWindow(L"BUTTON", L"Select Font...", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, inputW, h, hParent, (HMENU)ID_BTN_FONT, s_hInstance, NULL);
    SendMessage(hBtnFont, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    
    y += h + 20;
    CreateLabel(hParent, L"Basic Size (px):", margin, y, labelW, h);
    HWND hEditSB = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_SIZE_BASIC, s_hInstance, NULL);
    SendMessage(hEditSB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Extra Size (px):", margin, y, labelW, h);
    HWND hEditSE = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_SIZE_EXTRA, s_hInstance, NULL);
    SendMessage(hEditSE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    // Colors
    y += h + 30;
    int colorBtnW = 80;
    int opacityW = 50;

    CreateLabel(hParent, L"Basic Text (Light):", margin, y, labelW, h);
    HWND hBtnCLB = CreateWindow(L"BUTTON", L"#000000", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, colorBtnW, h, hParent, (HMENU)ID_BTN_COLOR_LB, s_hInstance, NULL);
    SendMessage(hBtnCLB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hEditOLB = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW + colorBtnW + 10, y, opacityW, h, hParent, (HMENU)ID_EDIT_OPACITY_LB, s_hInstance, NULL);
    SendMessage(hEditOLB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Basic Text (Dark):", margin, y, labelW, h);
    HWND hBtnCDB = CreateWindow(L"BUTTON", L"#FFFFFF", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, colorBtnW, h, hParent, (HMENU)ID_BTN_COLOR_DB, s_hInstance, NULL);
    SendMessage(hBtnCDB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hEditODB = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW + colorBtnW + 10, y, opacityW, h, hParent, (HMENU)ID_EDIT_OPACITY_DB, s_hInstance, NULL);
    SendMessage(hEditODB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Extra Text (Light):", margin, y, labelW, h);
    HWND hBtnCLE = CreateWindow(L"BUTTON", L"#000000", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, colorBtnW, h, hParent, (HMENU)ID_BTN_COLOR_LE, s_hInstance, NULL);
    SendMessage(hBtnCLE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hEditOLE = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW + colorBtnW + 10, y, opacityW, h, hParent, (HMENU)ID_EDIT_OPACITY_LE, s_hInstance, NULL);
    SendMessage(hEditOLE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Extra Text (Dark):", margin, y, labelW, h);
    HWND hBtnCDE = CreateWindow(L"BUTTON", L"#FFFFFF", WS_VISIBLE | WS_CHILD,
        margin + labelW, y, colorBtnW, h, hParent, (HMENU)ID_BTN_COLOR_DE, s_hInstance, NULL);
    SendMessage(hBtnCDE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hEditODE = CreateWindow(L"EDIT", L"1.0", WS_VISIBLE | WS_CHILD | WS_BORDER,
        margin + labelW + colorBtnW + 10, y, opacityW, h, hParent, (HMENU)ID_EDIT_OPACITY_DE, s_hInstance, NULL);
    SendMessage(hEditODE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    // Styles
    y += h + 30;
    CreateLabel(hParent, L"Basic Style:", margin, y, labelW, h);
    
    HWND hComboWB = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, 120, 300, hParent, (HMENU)ID_COMBO_WEIGHT_B, s_hInstance, NULL);
    SendMessage(hComboWB, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    const wchar_t* weights[] = { L"Thin (100)", L"ExtraLight (200)", L"Light (300)", L"Normal (400)", L"Medium (500)", L"SemiBold (600)", L"Bold (700)", L"ExtraBold (800)", L"Black (900)" };
    for (auto w : weights) SendMessage(hComboWB, CB_ADDSTRING, 0, (LPARAM)w);

    HWND hCheckIB = CreateWindow(L"BUTTON", L"Italic", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 130, y, 60, h, hParent, (HMENU)ID_CHECK_ITALIC_B, s_hInstance, NULL);
    SendMessage(hCheckIB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hCheckUB = CreateWindow(L"BUTTON", L"Underline", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 190, y, 80, h, hParent, (HMENU)ID_CHECK_UNDERLINE_B, s_hInstance, NULL);
    SendMessage(hCheckUB, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    
    HWND hCheckSB = CreateWindow(L"BUTTON", L"Strike", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 270, y, 60, h, hParent, (HMENU)ID_CHECK_STRIKE_B, s_hInstance, NULL);
    SendMessage(hCheckSB, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Extra Style:", margin, y, labelW, h);
    
    HWND hComboWE = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, 120, 300, hParent, (HMENU)ID_COMBO_WEIGHT_E, s_hInstance, NULL);
    SendMessage(hComboWE, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    for (auto w : weights) SendMessage(hComboWE, CB_ADDSTRING, 0, (LPARAM)w);

    HWND hCheckIE = CreateWindow(L"BUTTON", L"Italic", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 130, y, 60, h, hParent, (HMENU)ID_CHECK_ITALIC_E, s_hInstance, NULL);
    SendMessage(hCheckIE, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    HWND hCheckUE = CreateWindow(L"BUTTON", L"Underline", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 190, y, 80, h, hParent, (HMENU)ID_CHECK_UNDERLINE_E, s_hInstance, NULL);
    SendMessage(hCheckUE, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    
    HWND hCheckSE = CreateWindow(L"BUTTON", L"Strike", WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX,
        margin + labelW + 270, y, 60, h, hParent, (HMENU)ID_CHECK_STRIKE_E, s_hInstance, NULL);
    SendMessage(hCheckSE, WM_SETFONT, (WPARAM)s_hFont, TRUE);
}

void ConfigWindow::CreateLayoutPage(HWND hParent) {
    int margin = 20;
    int labelW = 120;
    int inputW = 150;
    int h = 25;
    int y = margin;

    // Position
    CreateLabel(hParent, L"Position:", margin, y, labelW, h);
    HWND hComboPos = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_POS, s_hInstance, NULL);
    SendMessage(hComboPos, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"Left");
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"Center");
    SendMessage(hComboPos, CB_ADDSTRING, 0, (LPARAM)L"Right");

    y += h + 20;
    CreateLabel(hParent, L"Margin Left:", margin, y, labelW, h);
    HWND hEditML = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_MARGIN_L, s_hInstance, NULL);
    SendMessage(hEditML, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 20;
    CreateLabel(hParent, L"Margin Right:", margin, y, labelW, h);
    HWND hEditMR = CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        margin + labelW, y, 80, h, hParent, (HMENU)ID_EDIT_MARGIN_R, s_hInstance, NULL);
    SendMessage(hEditMR, WM_SETFONT, (WPARAM)s_hFont, TRUE);

    y += h + 30;
    CreateLabel(hParent, L"Align (Basic):", margin, y, labelW, h);
    HWND hComboAB = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_ALIGN_B, s_hInstance, NULL);
    SendMessage(hComboAB, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"Leading (Left)");
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"Trailing (Right)");
    SendMessage(hComboAB, CB_ADDSTRING, 0, (LPARAM)L"Center");

    y += h + 20;
    CreateLabel(hParent, L"Align (Extra):", margin, y, labelW, h);
    HWND hComboAE = CreateWindow(L"COMBOBOX", L"", WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST,
        margin + labelW, y, inputW, 200, hParent, (HMENU)ID_COMBO_ALIGN_E, s_hInstance, NULL);
    SendMessage(hComboAE, WM_SETFONT, (WPARAM)s_hFont, TRUE);
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"Leading (Left)");
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"Trailing (Right)");
    SendMessage(hComboAE, CB_ADDSTRING, 0, (LPARAM)L"Center");
}

void ConfigWindow::LoadValues() {
    LoadGeneralValues(s_hPageGeneral);
    LoadAppearanceValues(s_hPageAppearance);
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
    SetDlgItemText(hPage, ID_BTN_COLOR_LB, ColorToHex(config.color.basic.light.hexColor).c_str());
    swprintf_s(buf, L"%.2f", config.color.basic.light.opacity);
    SetDlgItemText(hPage, ID_EDIT_OPACITY_LB, buf);

    SetDlgItemText(hPage, ID_BTN_COLOR_DB, ColorToHex(config.color.basic.dark.hexColor).c_str());
    swprintf_s(buf, L"%.2f", config.color.basic.dark.opacity);
    SetDlgItemText(hPage, ID_EDIT_OPACITY_DB, buf);

    SetDlgItemText(hPage, ID_BTN_COLOR_LE, ColorToHex(config.color.extra.light.hexColor).c_str());
    swprintf_s(buf, L"%.2f", config.color.extra.light.opacity);
    SetDlgItemText(hPage, ID_EDIT_OPACITY_LE, buf);

    SetDlgItemText(hPage, ID_BTN_COLOR_DE, ColorToHex(config.color.extra.dark.hexColor).c_str());
    swprintf_s(buf, L"%.2f", config.color.extra.dark.opacity);
    SetDlgItemText(hPage, ID_EDIT_OPACITY_DE, buf);

    // Styles
    // Basic
    int weightIdxB = (config.style.basic.weightValue / 100) - 1;
    if (weightIdxB < 0) weightIdxB = 3; // Default Normal
    if (weightIdxB > 8) weightIdxB = 8;
    SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_B), CB_SETCURSEL, weightIdxB, 0);

    CheckDlgButton(hPage, ID_CHECK_ITALIC_B, config.style.basic.slope == DWRITE_FONT_STYLE_ITALIC ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_UNDERLINE_B, config.style.basic.underline ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_STRIKE_B, config.style.basic.strikethrough ? BST_CHECKED : BST_UNCHECKED);

    // Extra
    int weightIdxE = (config.style.extra.weightValue / 100) - 1;
    if (weightIdxE < 0) weightIdxE = 3;
    if (weightIdxE > 8) weightIdxE = 8;
    SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_E), CB_SETCURSEL, weightIdxE, 0);

    CheckDlgButton(hPage, ID_CHECK_ITALIC_E, config.style.extra.slope == DWRITE_FONT_STYLE_ITALIC ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_UNDERLINE_E, config.style.extra.underline ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(hPage, ID_CHECK_STRIKE_E, config.style.extra.strikethrough ? BST_CHECKED : BST_UNCHECKED);
}

void ConfigWindow::LoadLayoutValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfig();
    
    // Position (0=Left, 1=Right usually, or 0=Left, 2=Right? Let's check logic)
    // struct PositionConfig { int value = 0; std::string textContent = "左侧"; };
    // Assuming value maps to index.
    SendMessage(GetDlgItem(hPage, ID_COMBO_POS), CB_SETCURSEL, config.position.value, 0);

    SetDlgItemInt(hPage, ID_EDIT_MARGIN_L, config.margin.left, FALSE);
    SetDlgItemInt(hPage, ID_EDIT_MARGIN_R, config.margin.right, FALSE);

    // Alignment: DWRITE_TEXT_ALIGNMENT_LEADING=0, TRAILING=1, CENTER=2
    SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_B), CB_SETCURSEL, config.align.basic, 0);
    SendMessage(GetDlgItem(hPage, ID_COMBO_ALIGN_E), CB_SETCURSEL, config.align.extra, 0);
}

void ConfigWindow::SaveValues(HWND hwnd) {
    SaveGeneralValues(s_hPageGeneral);
    SaveAppearanceValues(s_hPageAppearance);
    SaveLayoutValues(s_hPageLayout);

    ConfigManager::getInstance().Save();

    // Notify TaskbarLyrics to reload
    HWND hMsg = FindWindow(L"TaskbarLyricsTrayMsg", L"Taskbar Lyrics Tray");
    if (hMsg) {
        SendMessage(hMsg, WM_RELOAD_CONFIG, 0, 0);
    }

    MessageBox(hwnd, L"Settings saved successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
}

void ConfigWindow::SaveGeneralValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();
    
    wchar_t path[MAX_PATH];
    GetDlgItemText(hPage, ID_EDIT_PATH, path, MAX_PATH);
    config.hitokoto.jsonPath = WStringToString(path);

    BOOL success;
    int interval = GetDlgItemInt(hPage, ID_EDIT_INTERVAL, &success, FALSE);
    if (success && interval > 0) config.hitokoto.interval = interval;

    wchar_t parent[100];
    GetDlgItemText(hPage, ID_EDIT_PARENT_TASKBAR, parent, 100);
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
    config.size.basic = std::stof(WStringToString(buf));
    GetDlgItemText(hPage, ID_EDIT_SIZE_EXTRA, buf, 32);
    config.size.extra = std::stof(WStringToString(buf));

    // Colors
    wchar_t hex[32];
    wchar_t opacity[32];

    GetDlgItemText(hPage, ID_BTN_COLOR_LB, hex, 32); config.color.basic.light.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_LB, opacity, 32); 
    try { config.color.basic.light.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_DB, hex, 32); config.color.basic.dark.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_DB, opacity, 32);
    try { config.color.basic.dark.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_LE, hex, 32); config.color.extra.light.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_LE, opacity, 32);
    try { config.color.extra.light.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    GetDlgItemText(hPage, ID_BTN_COLOR_DE, hex, 32); config.color.extra.dark.hexColor = HexToColor(hex);
    GetDlgItemText(hPage, ID_EDIT_OPACITY_DE, opacity, 32);
    try { config.color.extra.dark.opacity = std::stof(WStringToString(opacity)); } catch (...) {}

    // Styles
    const char* weightTexts[] = { "Thin (100)", "ExtraLight (200)", "Light (300)", "Normal (400)", "Medium (500)", "SemiBold (600)", "Bold (700)", "ExtraBold (800)", "Black (900)" };

    // Basic
    int weightIdxB = SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_B), CB_GETCURSEL, 0, 0);
    if (weightIdxB != CB_ERR && weightIdxB >= 0 && weightIdxB < 9) {
        config.style.basic.weightValue = (weightIdxB + 1) * 100;
        config.style.basic.weightText = weightTexts[weightIdxB];
    }
    config.style.basic.slope = IsDlgButtonChecked(hPage, ID_CHECK_ITALIC_B) == BST_CHECKED ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    config.style.basic.underline = IsDlgButtonChecked(hPage, ID_CHECK_UNDERLINE_B) == BST_CHECKED;
    config.style.basic.strikethrough = IsDlgButtonChecked(hPage, ID_CHECK_STRIKE_B) == BST_CHECKED;

    // Extra
    int weightIdxE = SendMessage(GetDlgItem(hPage, ID_COMBO_WEIGHT_E), CB_GETCURSEL, 0, 0);
    if (weightIdxE != CB_ERR && weightIdxE >= 0 && weightIdxE < 9) {
        config.style.extra.weightValue = (weightIdxE + 1) * 100;
        config.style.extra.weightText = weightTexts[weightIdxE];
    }
    config.style.extra.slope = IsDlgButtonChecked(hPage, ID_CHECK_ITALIC_E) == BST_CHECKED ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
    config.style.extra.underline = IsDlgButtonChecked(hPage, ID_CHECK_UNDERLINE_E) == BST_CHECKED;
    config.style.extra.strikethrough = IsDlgButtonChecked(hPage, ID_CHECK_STRIKE_E) == BST_CHECKED;
}

void ConfigWindow::SaveLayoutValues(HWND hPage) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    int pos = SendMessage(GetDlgItem(hPage, ID_COMBO_POS), CB_GETCURSEL, 0, 0);
    if (pos != CB_ERR) config.position.value = pos;

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
