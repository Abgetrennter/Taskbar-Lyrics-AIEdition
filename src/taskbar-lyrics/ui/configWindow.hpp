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
    static void CreateTabControl(HWND hwnd);
    static void CreateGeneralPage(HWND hParent);
    static void CreateAppearancePage(HWND hParent);
    static void CreateLayoutPage(HWND hParent);
    static void OnTabSelChanged();
    
    static void LoadValues();
    static void LoadGeneralValues(HWND hPage);
    static void LoadAppearanceValues(HWND hPage);
    static void LoadLayoutValues(HWND hPage);
    
    static void SaveValues(HWND hwnd);
    static void SaveGeneralValues(HWND hPage);
    static void SaveAppearanceValues(HWND hPage);
    static void SaveLayoutValues(HWND hPage);

    // Helpers
    static void CreateLabel(HWND hParent, const wchar_t* text, int x, int y, int w, int h);
    static void HandleColorBtn(HWND hBtn);

    static HWND s_hwnd;
    static HINSTANCE s_hInstance;
    static HWND s_hTab;
    static HWND s_hPageGeneral;
    static HWND s_hPageAppearance;
    static HWND s_hPageLayout;
};
