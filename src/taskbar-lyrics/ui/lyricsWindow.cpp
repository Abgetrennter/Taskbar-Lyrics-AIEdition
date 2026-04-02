#include "lyricsWindow.hpp"
#include <chrono>
#include <cstring> // for std::memcmp
#include "../utils/configManager.hpp"
#include "../utils/hitokotoManager.hpp"

// Helper
static std::wstring utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

LyricsWindow* LyricsWindow::instance = nullptr;

LyricsWindow::LyricsWindow(HINSTANCE instanceHandle, int showCmd)
{
    LyricsWindow::instance = this;
    m_isMonitoring = true;
    // Pass the address of the member variable windowHandle
    this->renderer = new LyricsRenderer(&this->windowHandle);

    this->registerWindow(instanceHandle);
    this->createWindow(instanceHandle, showCmd);

    this->monitorRemainingWidth();
    this->monitorRegistry();
}

LyricsWindow::~LyricsWindow()
{
    m_isMonitoring = false;

    // Close registry key to break RegNotifyChangeKeyValue
    if (this->m_registryKey) {
        RegCloseKey(this->m_registryKey);
        this->m_registryKey = nullptr;
    }

    if (this->m_registryMonitorThread && this->m_registryMonitorThread->joinable()) {
        this->m_registryMonitorThread->join();
        delete this->m_registryMonitorThread;
        this->m_registryMonitorThread = nullptr;
    }

    if (this->m_widthMonitorThread && this->m_widthMonitorThread->joinable()) {
        this->m_widthMonitorThread->join();
        delete this->m_widthMonitorThread;
        this->m_widthMonitorThread = nullptr;
    }

    // Detach from Taskbar before destroying
    if (this->windowHandle) {
        SetParent(this->windowHandle, NULL);
        DestroyWindow(this->windowHandle);
        this->windowHandle = nullptr;
    }

    if (this->renderer) {
        delete this->renderer;
        this->renderer = nullptr;
    }
}

void LyricsWindow::registerWindow(HINSTANCE instanceHandle)
{
    WNDCLASSEX wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.hInstance = instanceHandle;
    wcex.lpfnWndProc = LyricsWindow::wndProc;
    wcex.lpszClassName = this->m_windowClassName.c_str();
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hCursor = nullptr;
    wcex.hIcon = nullptr;
    wcex.hIconSm = nullptr;
    wcex.lpszMenuName = nullptr;
    RegisterClassEx(&wcex);
}

void LyricsWindow::createWindow(HINSTANCE instanceHandle, int showCmd)
{
    HWND taskbarHandle = FindWindow(L"Shell_TrayWnd", NULL);
    this->windowHandle = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_TOPMOST,
        this->m_windowClassName.c_str(),
        this->m_windowName.c_str(),
        WS_POPUP,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        taskbarHandle,
        NULL,
        instanceHandle,
        NULL
    );

    SetParent(this->windowHandle, taskbarHandle);
    ShowWindow(this->windowHandle, showCmd);
    
    // Set timer for 30s check (every 1 second)
    SetTimer(this->windowHandle, 1, 1000, NULL);

    PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
}

void LyricsWindow::monitorRemainingWidth()
{
    auto threadFunc = [this]() {
        while (m_isMonitoring)
        {
            // Sleep in small chunks to allow faster exit
            for (int i = 0; i < 10; ++i) {
                if (!m_isMonitoring) return;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!m_isMonitoring) break;

            RECT taskbarRect;
            RECT startButtonRect;
            RECT activeAreaRect;
            RECT notificationAreaRect;

            if (!this->renderer) continue;

            GetWindowRect(this->renderer->taskbarHandle, &taskbarRect);
            GetWindowRect(this->renderer->startButtonHandle, &startButtonRect);
            GetWindowRect(this->renderer->activeAreaHandle, &activeAreaRect);
            GetWindowRect(this->renderer->notificationAreaHandle, &notificationAreaRect);

            // Refresh only if changed
            if (std::memcmp(&this->renderer->taskbarRect, &taskbarRect, sizeof(RECT)))
            {
                this->renderer->taskbarRect = taskbarRect;
                PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
            }

            if (this->renderer->isCentered)
            {
                if (std::memcmp(&this->renderer->startButtonRect, &startButtonRect, sizeof(RECT)))
                {
                    this->renderer->startButtonRect = startButtonRect;
                    PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
                }
            }
            else
            {
                if (std::memcmp(&this->renderer->activeAreaRect, &activeAreaRect, sizeof(RECT)))
                {
                    this->renderer->activeAreaRect = activeAreaRect;
                    PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
                }
                if (std::memcmp(&this->renderer->notificationAreaRect, &notificationAreaRect, sizeof(RECT))) {
                    this->renderer->notificationAreaRect = notificationAreaRect;
                    PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
                }
            }
        }
    };

    this->m_widthMonitorThread = new std::thread(threadFunc);
}

void LyricsWindow::monitorRegistry()
{
    auto readRegistry = [](std::wstring path, std::wstring keyName, DWORD& value) -> bool {
        HKEY key;
        DWORD bufferSize = sizeof(DWORD);
        if (RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), NULL, KEY_READ, &key)) return true; // Error
        if (RegQueryValueEx(key, keyName.c_str(), NULL, NULL, (LPBYTE)&value, &bufferSize)) {
            RegCloseKey(key);
            return true; // Error
        }
        RegCloseKey(key);
        return false; // Success
    };

    auto getRegistryValues = [this, readRegistry]() {
        if (!this->renderer) return;

        {
            DWORD value;
            std::wstring path = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize";
            std::wstring key = L"SystemUsesLightTheme";
            if (!readRegistry(path, key, value))
            {
                this->renderer->isLightMode = static_cast<bool>(value);
            }
        }

        {
            DWORD value;
            std::wstring path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
            std::wstring key = L"TaskbarAl";
            if (!readRegistry(path, key, value))
            {
                this->renderer->isCentered = static_cast<bool>(value);
            }
        }

        {
            DWORD value;
            std::wstring path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";
            std::wstring key = L"TaskbarDa";
            if (!readRegistry(path, key, value))
            {
                this->renderer->hasComponentButton = static_cast<bool>(value);
            }
        }

        PostMessage(this->windowHandle, WM_PAINT, NULL, NULL);
    };

    getRegistryValues();

    auto threadFunc = [this, getRegistryValues]() {
        // Continuous monitoring
        while (m_isMonitoring)
        {
            if (!this->m_registryKey)
            {
                std::wstring path = L"Software\\Microsoft\\Windows\\CurrentVersion";
                RegOpenKeyEx(HKEY_CURRENT_USER, path.c_str(), NULL, KEY_NOTIFY, &this->m_registryKey);
                if (!this->m_registryKey) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue;
                }
            }

            // This will block until change or key closed
            if (RegNotifyChangeKeyValue(this->m_registryKey, true, REG_NOTIFY_CHANGE_LAST_SET, NULL, false) != ERROR_SUCCESS) {
                // Probably key closed or error, exit loop
                break;
            }
            
            if (!m_isMonitoring) break;

            getRegistryValues();
        }
    };

    this->m_registryMonitorThread = new std::thread(threadFunc);
}

LRESULT CALLBACK LyricsWindow::wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_NCHITTEST:
        {
            if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
                return HTCLIENT;
            }
            return HTTRANSPARENT;
        }
        break;

        case WM_RBUTTONUP:
        {
            POINT pt;
            GetCursorPos(&pt);

            HMENU hMenu = CreatePopupMenu();
            AppendMenu(hMenu, MF_STRING, 1, L"关闭歌词");

            SetForegroundWindow(hwnd);
            int selection = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, pt.x, pt.y, 0, hwnd, NULL);
            DestroyMenu(hMenu);

            if (selection == 1) {
                DestroyWindow(hwnd);
            }
            return 0;
        }
        break;

        case WM_PAINT:
        {
            if (LyricsWindow::instance && LyricsWindow::instance->renderer) {
                LyricsWindow::instance->renderer->updateWindow();
            }
        }
        break;

        case WM_TIMER:
        {
            if (wParam == 1 && LyricsWindow::instance && LyricsWindow::instance->renderer) {
                auto renderer = LyricsWindow::instance->renderer;
                long long currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count();

                auto& config = ConfigManager::getInstance().GetConfig();
                int interval = config.hitokoto.interval * 1000; // Convert to ms
                
                bool shouldUpdate = false;
                
                if (!renderer->isShowingHitokoto) {
                     // Check if music stopped for long enough
                     if (currentTime - renderer->lastLyricsUpdateTimestamp > interval) {
                         shouldUpdate = true;
                     }
                } else {
                     // If already showing hitokoto, rotate based on interval too (optional, but good for variety)
                     // Or maybe we want to keep one until music starts? 
                     // Requirement says: "In the absence of lyrics (over 30s no new lyrics), extract one from json".
                     // Implicitly, if still no lyrics, maybe we can refresh it periodically or just keep it.
                     // Let's implement periodic refresh for hitokoto as well if user wants dynamic quotes.
                     // We can use the same interval for refreshing quotes.
                     
                     // We need a timestamp for when hitokoto was shown. 
                     // We can reuse lastLyricsUpdateTimestamp for hitokoto show time if we update it when showing hitokoto.
                     if (currentTime - renderer->lastLyricsUpdateTimestamp > interval) {
                         shouldUpdate = true;
                     }
                }

                if (shouldUpdate) {
                    std::string path = config.hitokoto.jsonPath;
                    HitokotoManager::getInstance().Load(path);
                    auto entry = HitokotoManager::getInstance().GetRandom();
                    
                    if (!entry.hitokoto.empty()) {
                        renderer->basicLyrics = utf8ToWide(entry.hitokoto);
                        std::string extra = entry.from;
                        if (!entry.from_who.empty()) {
                            if (!extra.empty()) extra += " ";
                            extra += entry.from_who;
                        }
                        renderer->extraLyrics = utf8ToWide(extra);
                        renderer->isShowingHitokoto = true;
                        // Update timestamp so we don't refresh immediately again, and so we can refresh after interval
                        renderer->lastLyricsUpdateTimestamp = currentTime; 
                        renderer->updateWindow();
                    }
                }
            }
        }
        break;

        case WM_ERASEBKGND:
        {
            return 0;
        }
        break;

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
        }
        break;

        case WM_DESTROY:
        {
            PostQuitMessage(0);
        }
        break;

        default:
        {
            return DefWindowProc(hwnd, message, wParam, lParam);
        }
        break;
    }

    return 0;
}
