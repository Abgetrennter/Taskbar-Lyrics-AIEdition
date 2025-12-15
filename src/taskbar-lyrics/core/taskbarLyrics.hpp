#pragma once

#include <winsock2.h>
#include <windows.h>
#include "../network/networkServer.hpp"
#include "../ui/lyricsWindow.hpp"
#include "../ui/trayIcon.hpp"

#define WM_TRAYICON (WM_USER + 1)

class TaskbarLyrics
{
public:
    TaskbarLyrics(HINSTANCE instanceHandle, int showCmd);
    ~TaskbarLyrics();

    NetworkServer* networkServer = nullptr;
    LyricsWindow* lyricsWindow = nullptr;

    void handleTrayCommand(int commandId);

private:
    unsigned short m_port = 27232;
    TrayIcon* m_trayIcon = nullptr;
    HWND m_msgHwnd = NULL;
    HINSTANCE m_hInstance = NULL;
    
    static TaskbarLyrics* s_instance;

    void getPort();
    void createMessageWindow();
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    // Commands
    void onStartService();
    void onStopService();
    void onConfig();
    void onStatus();
    void onOpenLogs();
    void onExit();
};
