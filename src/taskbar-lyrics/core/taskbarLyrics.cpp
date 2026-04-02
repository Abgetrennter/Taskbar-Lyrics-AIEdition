#include "taskbarLyrics.hpp"
#include <sstream>
#include <shellapi.h> // for CommandLineToArgvW
#include "../utils/logger.hpp"
#include "../utils/configManager.hpp"
#include "../ui/configWindow.hpp"

#define IDM_START_SERVICE 1001
#define IDM_STOP_SERVICE 1002
#define IDM_CONFIG 1003
#define IDM_STATUS 1004
#define IDM_OPEN_LOGS 1005
#define IDM_EXIT 1006

TaskbarLyrics* TaskbarLyrics::s_instance = nullptr;

static std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

TaskbarLyrics::TaskbarLyrics(HINSTANCE instanceHandle, int showCmd)
{
    s_instance = this;
    m_hInstance = instanceHandle;
    
    // Init Logger
    wchar_t path[MAX_PATH];
    GetModuleFileName(NULL, path, MAX_PATH);
    std::wstring exePath(path);
    std::wstring logDir = exePath.substr(0, exePath.find_last_of(L"\\")) + L"\\logs";
    Logger::Init(logDir);
    Logger::Info("Application started");

    // Load Config
    ConfigManager::getInstance().Load();

    this->getPort();
    Logger::Info("Listening on port: %d", m_port);

    createMessageWindow();

    m_trayIcon = new TrayIcon();
    HICON hIcon = LoadIcon(NULL, IDI_APPLICATION);
    m_trayIcon->Init(m_msgHwnd, 1, WM_TRAYICON, hIcon, L"Taskbar Lyrics");
    m_trayIcon->Show();

    this->lyricsWindow = new LyricsWindow(instanceHandle, showCmd);
    
    // Apply Config
    ReloadConfig();

    this->networkServer = new NetworkServer(this->lyricsWindow, this->m_port);

    m_trayIcon->ShowBalloon(L"Taskbar Lyrics", L"服务启动成功", NIIF_INFO);
}

TaskbarLyrics::~TaskbarLyrics()
{
    if (m_trayIcon) {
        delete m_trayIcon;
        m_trayIcon = nullptr;
    }

    if (m_msgHwnd) {
        DestroyWindow(m_msgHwnd);
        m_msgHwnd = NULL;
    }

    FreeConsole();

    if (this->networkServer) {
        delete this->networkServer;
        this->networkServer = nullptr;
    }

    if (this->lyricsWindow) {
        delete this->lyricsWindow;
        this->lyricsWindow = nullptr;
    }
}

void TaskbarLyrics::getPort()
{
    int argCount;
    LPWSTR* szArgList = CommandLineToArgvW(GetCommandLine(), &argCount);

    if (szArgList && argCount > 1 && szArgList[1])
    {
        std::wstringstream ss;
        ss << szArgList[1];
        ss >> this->m_port;
    }

    if (szArgList) LocalFree(szArgList);
}

void TaskbarLyrics::createMessageWindow() {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = m_hInstance;
    wc.lpszClassName = L"TaskbarLyricsTrayMsg";
    
    RegisterClassEx(&wc);
    
    m_msgHwnd = CreateWindowEx(0, L"TaskbarLyricsTrayMsg", L"Taskbar Lyrics Tray", 
                               0, 0, 0, 0, 0, HWND_MESSAGE, NULL, m_hInstance, NULL);
}

LRESULT CALLBACK TaskbarLyrics::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_TRAYICON) {
        if (lParam == WM_RBUTTONUP) {
            POINT pt;
            GetCursorPos(&pt);
            SetForegroundWindow(hwnd);
            
            HMENU hMenu = CreatePopupMenu();
            
            bool isRunning = s_instance && s_instance->networkServer && s_instance->networkServer->isRunning();
            
            if (isRunning) {
                AppendMenu(hMenu, MF_STRING, IDM_STOP_SERVICE, L"停止服务");
            } else {
                AppendMenu(hMenu, MF_STRING, IDM_START_SERVICE, L"启动服务");
            }
            
            AppendMenu(hMenu, MF_STRING, IDM_CONFIG, L"设置");
            AppendMenu(hMenu, MF_STRING, IDM_STATUS, L"运行状态");
            AppendMenu(hMenu, MF_STRING, IDM_OPEN_LOGS, L"打开日志目录");
            AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(hMenu, MF_STRING, IDM_EXIT, L"退出程序");
            
            TrackPopupMenu(hMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);
        }
        else if (lParam == WM_LBUTTONDBLCLK) {
             if (s_instance) s_instance->onStatus();
        }
    }
    else if (msg == WM_RELOAD_CONFIG) {
        if (s_instance) s_instance->ReloadConfig();
    }
    else if (msg == WM_COMMAND) {
        int id = LOWORD(wParam);
        if (s_instance) s_instance->handleTrayCommand(id);
    }
    
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void TaskbarLyrics::handleTrayCommand(int commandId) {
    switch(commandId) {
        case IDM_START_SERVICE: onStartService(); break;
        case IDM_STOP_SERVICE: onStopService(); break;
        case IDM_CONFIG: onConfig(); break;
        case IDM_STATUS: onStatus(); break;
        case IDM_OPEN_LOGS: onOpenLogs(); break;
        case IDM_EXIT: onExit(); break;
    }
}

void TaskbarLyrics::onStartService() {
    if (networkServer) {
        networkServer->start();
        m_trayIcon->UpdateIcon(LoadIcon(NULL, IDI_APPLICATION));
        m_trayIcon->ShowBalloon(L"Taskbar Lyrics", L"服务已启动", NIIF_INFO);
        m_trayIcon->UpdateTip(L"Taskbar Lyrics (Running)");
    }
}

void TaskbarLyrics::onStopService() {
    if (networkServer) {
        networkServer->stop();
        m_trayIcon->UpdateIcon(LoadIcon(NULL, IDI_WARNING));
        m_trayIcon->ShowBalloon(L"Taskbar Lyrics", L"服务已停止", NIIF_WARNING);
        m_trayIcon->UpdateTip(L"Taskbar Lyrics (Stopped)");
    }
}

void TaskbarLyrics::onConfig() {
    ConfigWindow::Show(m_hInstance);
}

void TaskbarLyrics::onStatus() {
    std::wstring status = L"服务状态: ";
    if (networkServer && networkServer->isRunning()) {
        status += L"运行中\n端口: " + std::to_wstring(m_port);
    } else {
        status += L"已停止";
    }
    MessageBox(NULL, status.c_str(), L"Taskbar Lyrics 状态", MB_OK | MB_ICONINFORMATION);
}

void TaskbarLyrics::onOpenLogs() {
    // Open logs directory
    std::wstring logPath = Logger::GetLogPath();
    std::wstring logDir = logPath.substr(0, logPath.find_last_of(L"\\"));
    ShellExecute(NULL, L"open", logDir.c_str(), NULL, NULL, SW_SHOW);
}

void TaskbarLyrics::onExit() {
    PostQuitMessage(0);
}

void TaskbarLyrics::ReloadConfig() {
    const AppConfig& config = ConfigManager::getInstance().GetConfig();
    if (this->lyricsWindow && this->lyricsWindow->renderer) {
        auto r = this->lyricsWindow->renderer;
        r->fontFamily = utf8ToWide(config.font.fontFamily);
        if (config.size.basic > 0) {
            r->basicFontSize = config.size.basic;
            r->basicFontSizeDoubleLine = config.size.basic;
        }
        if (config.size.extra > 0) r->extraFontSize = config.size.extra;
        
        r->basicLightColor = D2D1::ColorF(config.color.basic.light.hexColor, config.color.basic.light.opacity);
        r->basicDarkColor = D2D1::ColorF(config.color.basic.dark.hexColor, config.color.basic.dark.opacity);
        r->extraLightColor = D2D1::ColorF(config.color.extra.light.hexColor, config.color.extra.light.opacity);
        r->extraDarkColor = D2D1::ColorF(config.color.extra.dark.hexColor, config.color.extra.dark.opacity);

        r->basicFontWeight = (DWRITE_FONT_WEIGHT)config.style.basic.weightValue;
        r->extraFontWeight = (DWRITE_FONT_WEIGHT)config.style.extra.weightValue;
        r->basicFontStyle = (DWRITE_FONT_STYLE)config.style.basic.slope;
        r->extraFontStyle = (DWRITE_FONT_STYLE)config.style.extra.slope;
        r->basicUnderline = config.style.basic.underline;
        r->extraUnderline = config.style.extra.underline;
        r->basicStrikethrough = config.style.basic.strikethrough;
        r->extraStrikethrough = config.style.extra.strikethrough;

        r->updateParentTaskbar(config.screen.parentTaskbarValue);
    }
}

int APIENTRY wWinMain(
    _In_        HINSTANCE   instanceHandle,
    _In_opt_    HINSTANCE   prevInstance,
    _In_        LPWSTR      commandLine,
    _In_        int         showCmd
) {
    UNREFERENCED_PARAMETER(prevInstance);
    UNREFERENCED_PARAMETER(commandLine);

    #ifdef _DEBUG
        AllocConsole();
        SetConsoleOutputCP(65001);
        FILE* stream;
        freopen_s(&stream, "conout$", "w", stdout);
    #endif

    TaskbarLyrics app(instanceHandle, showCmd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
