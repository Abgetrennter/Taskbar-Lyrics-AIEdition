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

private:
    LyricsWindow* m_window = nullptr;
    bool m_isRunning = false;
    long long m_lastHeartbeatTime = 0;
    
    std::thread* m_serverThread = nullptr;
    std::thread* m_heartbeatThread = nullptr;
    SOCKET m_listenSocket = INVALID_SOCKET;

    void listenThreadFunc(unsigned short port);
    void heartbeatFunc();
    void handleConnection(SOCKET clientSocket);
    
    // WebSocket Helpers
    bool isWebSocketRequest(const std::string& request);
    std::string base64Encode(const unsigned char* data, size_t len);
    std::string calculateWebSocketKey(const std::string& key);
    void webSocketHandshake(SOCKET clientSocket, const std::string& request);
    bool readWebSocketFrame(SOCKET clientSocket, std::string& outMessage);

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

    // Encoding conversion
    std::wstring utf8ToWide(const std::string& str);
};
