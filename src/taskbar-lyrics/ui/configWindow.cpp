#include "configWindow.hpp"
#include "../utils/configManager.hpp"
#include <commctrl.h>

#pragma comment(lib, "Comctl32.lib")

#define ID_BTN_SAVE 101
#define ID_BTN_CANCEL 102
#define ID_EDIT_PATH 103
#define ID_EDIT_INTERVAL 104

HWND ConfigWindow::s_hwnd = NULL;
HINSTANCE ConfigWindow::s_hInstance = NULL;

void ConfigWindow::Show(HINSTANCE hInstance) {
    if (s_hwnd) {
        SetForegroundWindow(s_hwnd);
        return;
    }

    s_hInstance = hInstance;

    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"TaskbarLyricsConfig";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassEx(&wc);

    // Center window
    int width = 500;
    int height = 250;
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenWidth - width) / 2;
    int y = (screenHeight - height) / 2;

    s_hwnd = CreateWindowEx(0, L"TaskbarLyricsConfig", L"Taskbar Lyrics Settings",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        x, y, width, height, NULL, NULL, hInstance, NULL);

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
}

bool ConfigWindow::IsOpen() {
    return s_hwnd != NULL;
}

LRESULT CALLBACK ConfigWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            CreateControls(hwnd);
            LoadValues(hwnd);
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_SAVE) {
                SaveValues(hwnd);
                DestroyWindow(hwnd);
            } else if (LOWORD(wParam) == ID_BTN_CANCEL) {
                DestroyWindow(hwnd);
            }
            break;
        case WM_DESTROY:
            s_hwnd = NULL;
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void ConfigWindow::CreateControls(HWND hwnd) {
    // Hitokoto JSON Path
    CreateWindow(L"STATIC", L"Hitokoto JSON Path:", WS_VISIBLE | WS_CHILD,
        20, 20, 150, 20, hwnd, NULL, s_hInstance, NULL);
    
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL,
        180, 20, 280, 20, hwnd, (HMENU)ID_EDIT_PATH, s_hInstance, NULL);

    // Hitokoto Interval
    CreateWindow(L"STATIC", L"Interval (seconds):", WS_VISIBLE | WS_CHILD,
        20, 60, 150, 20, hwnd, NULL, s_hInstance, NULL);
    
    CreateWindow(L"EDIT", L"", WS_VISIBLE | WS_CHILD | WS_BORDER | ES_NUMBER,
        180, 60, 100, 20, hwnd, (HMENU)ID_EDIT_INTERVAL, s_hInstance, NULL);

    // Buttons
    CreateWindow(L"BUTTON", L"Save", WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        150, 120, 80, 30, hwnd, (HMENU)ID_BTN_SAVE, s_hInstance, NULL);

    CreateWindow(L"BUTTON", L"Cancel", WS_VISIBLE | WS_CHILD,
        250, 120, 80, 30, hwnd, (HMENU)ID_BTN_CANCEL, s_hInstance, NULL);
}

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

void ConfigWindow::LoadValues(HWND hwnd) {
    auto& config = ConfigManager::getInstance().GetConfig();
    
    // Path
    std::wstring path = StringToWString(config.hitokoto.jsonPath);
    SetDlgItemText(hwnd, ID_EDIT_PATH, path.c_str());

    // Interval
    SetDlgItemInt(hwnd, ID_EDIT_INTERVAL, config.hitokoto.interval, FALSE);
}

void ConfigWindow::SaveValues(HWND hwnd) {
    auto& config = ConfigManager::getInstance().GetConfigMutable();

    // Path
    wchar_t path[MAX_PATH];
    GetDlgItemText(hwnd, ID_EDIT_PATH, path, MAX_PATH);
    config.hitokoto.jsonPath = WStringToString(path);

    // Interval
    BOOL success;
    int interval = GetDlgItemInt(hwnd, ID_EDIT_INTERVAL, &success, FALSE);
    if (success && interval > 0) {
        config.hitokoto.interval = interval;
    }

    ConfigManager::getInstance().Save();
    MessageBox(hwnd, L"Settings saved successfully!", L"Success", MB_OK | MB_ICONINFORMATION);
}
