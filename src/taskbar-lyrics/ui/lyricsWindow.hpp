#pragma once

#include "lyricsRenderer.hpp"
#include <Windows.h>
#include <string>
#include <thread>

class LyricsWindow
{
public:
    static LyricsWindow* instance;
    LyricsRenderer* renderer = nullptr;
    HWND windowHandle = nullptr;

    LyricsWindow(HINSTANCE instanceHandle, int showCmd);
    ~LyricsWindow();

private:
    std::thread* m_widthMonitorThread = nullptr;
    void monitorRemainingWidth();

    HKEY m_registryKey = nullptr;
    std::thread* m_registryMonitorThread = nullptr;
    void monitorRegistry();

    std::wstring m_windowClassName = L"测试";
    std::wstring m_windowName = L"BetterNCM Taskbar Lyrics";

    void registerWindow(HINSTANCE instanceHandle);
    void createWindow(HINSTANCE instanceHandle, int showCmd);

    static LRESULT CALLBACK wndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
};
