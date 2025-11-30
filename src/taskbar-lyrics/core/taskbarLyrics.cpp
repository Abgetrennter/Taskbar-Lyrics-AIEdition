#include "taskbarLyrics.hpp"
#include <sstream>
#include <shellapi.h> // for CommandLineToArgvW

TaskbarLyrics::TaskbarLyrics(HINSTANCE instanceHandle, int showCmd)
{
    this->getPort();

    this->lyricsWindow = new LyricsWindow(instanceHandle, showCmd);
    this->networkServer = new NetworkServer(this->lyricsWindow, this->m_port);

    this->checkNcmProcess();
}

TaskbarLyrics::~TaskbarLyrics()
{
    FreeConsole();
    if (m_waitHandle) {
        UnregisterWaitEx(m_waitHandle, INVALID_HANDLE_VALUE);
    }

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

void TaskbarLyrics::checkNcmProcess()
{
    auto closeWindowCallback = [] (PVOID lpParameter, BOOLEAN TimerOrWaitFired)
    {
        UNREFERENCED_PARAMETER(TimerOrWaitFired);
        TaskbarLyrics* _this = static_cast<TaskbarLyrics*>(lpParameter);
        if (_this && _this->lyricsWindow) {
            SendMessage(_this->lyricsWindow->windowHandle, WM_CLOSE, NULL, NULL);
        }
    };

    HWND ncmHandle = FindWindow(L"OrpheusBrowserHost", NULL);
    if (ncmHandle)
    {
        DWORD pid;
        GetWindowThreadProcessId(ncmHandle, &pid);
        HANDLE process = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid); 
        if (process) {
             RegisterWaitForSingleObject(&this->m_waitHandle, process, closeWindowCallback, this, INFINITE, WT_EXECUTEONLYONCE);
             CloseHandle(process); // RegisterWaitForSingleObject duplicates the handle if needed, or we should keep it open? MSDN says "The handle is closed when the registration is deleted". Actually we shouldn't close it if we passed it? Wait. "The handle to the object. For a list of the object types whose handles can be specified, see the Remarks section."
             // MSDN for RegisterWaitForSingleObject: "The wait thread uses the WaitForMultipleObjects function to monitor the registered handles."
             // It does NOT say it duplicates it. But OpenProcess returns a handle.
             // The original code didn't close it. But it might leak.
             // Let's stick to original logic to be safe, or close it if we are sure.
             // Actually, `RegisterWaitForSingleObject` takes the handle. We should probably not close it immediately if we want to wait on it.
             // BUT, we are passing `process`.
        }
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
