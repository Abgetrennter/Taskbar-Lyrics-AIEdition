#pragma once
#include <windows.h>
#include <string>

class ConfigWindow {
public:
    static void Show(HINSTANCE hInstance);
    static bool IsOpen();

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void CreateControls(HWND hwnd);
    static void LoadValues(HWND hwnd);
    static void SaveValues(HWND hwnd);

    static HWND s_hwnd;
    static HINSTANCE s_hInstance;
};
