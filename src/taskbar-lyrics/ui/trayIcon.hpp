#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <string>

class TrayIcon {
public:
    TrayIcon();
    ~TrayIcon();

    bool Init(HWND hwnd, UINT uID, UINT uCallbackMsg, HICON hIcon, const std::wstring& tip);
    void Show();
    void Hide();
    void UpdateIcon(HICON hIcon);
    void UpdateTip(const std::wstring& tip);
    void ShowBalloon(const std::wstring& title, const std::wstring& text, DWORD dwInfoFlags = NIIF_INFO);

private:
    NOTIFYICONDATA m_nid;
    bool m_visible = false;
};
