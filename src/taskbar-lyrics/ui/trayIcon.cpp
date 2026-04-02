#include "trayIcon.hpp"

TrayIcon::TrayIcon() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_visible = false;
}

TrayIcon::~TrayIcon() {
    Hide();
}

bool TrayIcon::Init(HWND hwnd, UINT uID, UINT uCallbackMsg, HICON hIcon, const std::wstring& tip) {
    m_nid.cbSize = sizeof(NOTIFYICONDATA);
    m_nid.hWnd = hwnd;
    m_nid.uID = uID;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = uCallbackMsg;
    m_nid.hIcon = hIcon;
    
    wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);

    return true;
}

void TrayIcon::Show() {
    if (!m_visible) {
        Shell_NotifyIcon(NIM_ADD, &m_nid);
        m_visible = true;
    }
}

void TrayIcon::Hide() {
    if (m_visible) {
        Shell_NotifyIcon(NIM_DELETE, &m_nid);
        m_visible = false;
    }
}

void TrayIcon::UpdateIcon(HICON hIcon) {
    m_nid.hIcon = hIcon;
    if (m_visible) {
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::UpdateTip(const std::wstring& tip) {
    wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);
    if (m_visible) {
        Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    }
}

void TrayIcon::ShowBalloon(const std::wstring& title, const std::wstring& text, DWORD dwInfoFlags) {
    m_nid.uFlags |= NIF_INFO;
    wcsncpy_s(m_nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(m_nid.szInfo, text.c_str(), _TRUNCATE);
    m_nid.dwInfoFlags = dwInfoFlags;
    
    Shell_NotifyIcon(NIM_MODIFY, &m_nid);
    
    // Reset flags
    m_nid.uFlags &= ~NIF_INFO;
}
