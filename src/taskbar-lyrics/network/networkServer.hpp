#pragma once

#include <winsock2.h>
#include "../ui/lyricsWindow.hpp"
#include <string>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

class NetworkServer
{
public:
    NetworkServer(LyricsWindow* window, unsigned short port);
    ~NetworkServer();

    void start();
    void stop();
    bool isRunning() const { return m_isRunning; }

private:
    LyricsWindow* m_window = nullptr;
    bool m_isRunning = false;
    unsigned short m_port = 27232;
    
    std::thread* m_serverThread = nullptr;
    SOCKET m_listenSocket = INVALID_SOCKET;

    void listenThreadFunc(unsigned short port);
    void handleConnection(SOCKET clientSocket);
    
    // Business Logic
    void handleFont(const std::string& body);
    void handleColor(const std::string& body);
    void handleStyle(const std::string& body);
    void handleSize(const std::string& body);
    void handleLyrics(const std::string& body);
    void handleAlign(const std::string& body);
    void handlePosition(const std::string& body);
    void handleMargin(const std::string& body);
    void handleScreen(const std::string& body);
    void handleClose(const std::string& body);
    
    // Config Page
    void handleConfigPage(SOCKET clientSocket);
    void handleStyleCss(SOCKET clientSocket);
    void handleGetConfig(SOCKET clientSocket);
    void handleUpdateConfig(SOCKET clientSocket, const std::string& body);
    void handleResetConfig(SOCKET clientSocket);
    void applyConfigToRenderer();
    
    std::wstring utf8ToWide(const std::string& str);
};
