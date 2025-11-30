#pragma once

#include <winsock2.h>
#include <windows.h>
#include "../network/networkServer.hpp"
#include "../ui/lyricsWindow.hpp"

class TaskbarLyrics
{
public:
    TaskbarLyrics(HINSTANCE instanceHandle, int showCmd);
    ~TaskbarLyrics();

    NetworkServer* networkServer = nullptr;
    LyricsWindow* lyricsWindow = nullptr;

private:
    HANDLE m_waitHandle = nullptr;
    unsigned short m_port = 3798;

    void checkNcmProcess();
    void getPort();
};
