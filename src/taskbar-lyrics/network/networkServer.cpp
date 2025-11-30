#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "networkServer.hpp"
#include "../utils/jsonUtils.hpp"
#include <sstream>
#include <algorithm>
#include <wincrypt.h>
#include <chrono>

#pragma comment(lib, "Advapi32.lib")

static long long GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

NetworkServer::NetworkServer(LyricsWindow* window, unsigned short port)
{
    m_window = window;
    m_isRunning = true;
    m_lastHeartbeatTime = GetCurrentTimestamp();
    m_serverThread = new std::thread(&NetworkServer::listenThreadFunc, this, port);
    m_heartbeatThread = new std::thread(&NetworkServer::heartbeatFunc, this);
}

NetworkServer::~NetworkServer()
{
    m_isRunning = false;
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
    }
    m_serverThread = nullptr;

    if (m_heartbeatThread && m_heartbeatThread->joinable()) {
        m_heartbeatThread->join();
        delete m_heartbeatThread;
    }
    m_heartbeatThread = nullptr;

    WSACleanup();
}

void NetworkServer::heartbeatFunc()
{
    while (m_isRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        long long now = GetCurrentTimestamp();
        
        static bool initialized = false;
        if (!initialized) {
             if (now - m_lastHeartbeatTime > 30000) initialized = true;
             else continue;
        }

        if (now - m_lastHeartbeatTime > 10000) {
            SendMessage(m_window->windowHandle, WM_CLOSE, NULL, NULL);
            break;
        }
    }
}

void NetworkServer::listenThreadFunc(unsigned short port)
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;

    m_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(port);

    if (bind(m_listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(m_listenSocket);
        WSACleanup();
        return;
    }

    if (listen(m_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(m_listenSocket);
        WSACleanup();
        return;
    }

    while (m_isRunning) {
        SOCKET clientSocket = accept(m_listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (m_isRunning) continue; 
            else break;
        }
        
        handleConnection(clientSocket);
    }
}

void NetworkServer::handleConnection(SOCKET clientSocket)
{
    char buffer[8192]; 
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        
        m_lastHeartbeatTime = GetCurrentTimestamp();

        if (isWebSocketRequest(request)) {
            webSocketHandshake(clientSocket, request);
            
            std::string message;
            while (m_isRunning) {
                if (!readWebSocketFrame(clientSocket, message)) {
                    break; 
                }
                
                m_lastHeartbeatTime = GetCurrentTimestamp();

                if (message.empty()) continue;

                std::string url = JsonUtils::getString(message, "url");
                std::string body = message; 

                if (url == "/taskbar/font/font") handleFont(body);
                else if (url == "/taskbar/font/color") handleColor(body);
                else if (url == "/taskbar/font/style") handleStyle(body);
                else if (url == "/taskbar/font/size") handleSize(body);
                else if (url == "/taskbar/lyrics/lyrics") handleLyrics(body);
                else if (url == "/taskbar/lyrics/align") handleAlign(body);
                else if (url == "/taskbar/window/position") handlePosition(body);
                else if (url == "/taskbar/window/margin") handleMargin(body);
                else if (url == "/taskbar/window/screen") handleScreen(body);
                else if (url == "/taskbar/close") {
                    handleClose(body);
                    break;
                }
            }
            closesocket(clientSocket);
            return;
        }

        // HTTP Handling
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headers = request.substr(0, headerEnd);
            std::string body = request.substr(headerEnd + 4);
            
            size_t firstSpace = headers.find(' ');
            size_t secondSpace = headers.find(' ', firstSpace + 1);
            if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
                std::string url = headers.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                
                if (url == "/taskbar/font/font") handleFont(body);
                else if (url == "/taskbar/font/color") handleColor(body);
                else if (url == "/taskbar/font/style") handleStyle(body);
                else if (url == "/taskbar/font/size") handleSize(body);
                else if (url == "/taskbar/lyrics/lyrics") handleLyrics(body);
                else if (url == "/taskbar/lyrics/align") handleAlign(body);
                else if (url == "/taskbar/window/position") handlePosition(body);
                else if (url == "/taskbar/window/margin") handleMargin(body);
                else if (url == "/taskbar/window/screen") handleScreen(body);
                else if (url == "/taskbar/close") handleClose(body);
            }
        }
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

bool NetworkServer::isWebSocketRequest(const std::string& request) {
    return request.find("Upgrade: websocket") != std::string::npos || 
           request.find("Upgrade: WebSocket") != std::string::npos;
}

std::string NetworkServer::base64Encode(const unsigned char* data, size_t len) {
    static const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (len--) {
        char_array_3[i++] = *(data++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';
    }
    return ret;
}

std::string NetworkServer::calculateWebSocketKey(const std::string& key) {
    std::string magic = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    std::string input = key + magic;

    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    DWORD cbHash = 20;
    BYTE rgbHash[20];

    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash)) {
            CryptHashData(hHash, (BYTE*)input.c_str(), (DWORD)input.length(), 0);
            CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0);
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }

    return base64Encode(rgbHash, 20);
}

void NetworkServer::webSocketHandshake(SOCKET clientSocket, const std::string& request) {
    std::string keyPrefix = "Sec-WebSocket-Key: ";
    size_t keyStart = request.find(keyPrefix);
    if (keyStart == std::string::npos) return;
    
    keyStart += keyPrefix.length();
    size_t keyEnd = request.find("\r\n", keyStart);
    std::string key = request.substr(keyStart, keyEnd - keyStart);
    
    std::string acceptKey = calculateWebSocketKey(key);
    
    std::stringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";
    
    std::string resStr = response.str();
    send(clientSocket, resStr.c_str(), (int)resStr.length(), 0);
}

bool NetworkServer::readWebSocketFrame(SOCKET clientSocket, std::string& outMessage) {
    unsigned char buffer[2];
    int totalRead = 0;
    while (totalRead < 2) {
        int r = recv(clientSocket, (char*)buffer + totalRead, 2 - totalRead, 0);
        if (r <= 0) return false;
        totalRead += r;
    }

    bool fin = (buffer[0] & 0x80) != 0;
    int opcode = buffer[0] & 0x0f;
    
    if (opcode == 8) return false; // Close frame

    bool masked = (buffer[1] & 0x80) != 0;
    unsigned long long payloadLen = buffer[1] & 0x7f;

    if (payloadLen == 126) {
        unsigned char lenBytes[2];
        totalRead = 0;
        while (totalRead < 2) {
            int r = recv(clientSocket, (char*)lenBytes + totalRead, 2 - totalRead, 0);
            if (r <= 0) return false;
            totalRead += r;
        }
        payloadLen = (lenBytes[0] << 8) | lenBytes[1];
    } else if (payloadLen == 127) {
        unsigned char lenBytes[8];
        totalRead = 0;
        while (totalRead < 8) {
            int r = recv(clientSocket, (char*)lenBytes + totalRead, 8 - totalRead, 0);
            if (r <= 0) return false;
            totalRead += r;
        }
        unsigned long long realLen = 0;
        for(int i=0; i<8; i++) {
            realLen = (realLen << 8) | lenBytes[i];
        }
        payloadLen = realLen;
    }

    unsigned char maskingKey[4];
    if (masked) {
        totalRead = 0;
        while (totalRead < 4) {
            int r = recv(clientSocket, (char*)maskingKey + totalRead, 4 - totalRead, 0);
            if (r <= 0) return false;
            totalRead += r;
        }
    }

    if (payloadLen > 0) {
        if (payloadLen > 16 * 1024 * 1024) return false;

        size_t safeLen = (size_t)payloadLen;
        std::vector<char> payload(safeLen);
        int totalReceived = 0;
        while (totalReceived < (int)safeLen) {
            int r = recv(clientSocket, payload.data() + totalReceived, (int)(safeLen - totalReceived), 0);
            if (r <= 0) return false;
            totalReceived += r;
        }

        if (masked) {
            for (size_t i = 0; i < safeLen; i++) {
                payload[i] = payload[i] ^ maskingKey[i % 4];
            }
        }
        outMessage.assign(payload.begin(), payload.end());
    } else {
        outMessage = "";
    }

    return true;
}

std::wstring NetworkServer::utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void NetworkServer::handleFont(const std::string& body) {
    m_window->renderer->fontFamily = utf8ToWide(JsonUtils::getString(body, "font_family"));
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleColor(const std::string& body) {
    m_window->renderer->basicLightColor = D2D1::ColorF(
        JsonUtils::getHex(body, "basic_light_hex_color"),
        JsonUtils::getFloat(body, "basic_light_opacity")
    );
    m_window->renderer->basicDarkColor = D2D1::ColorF(
        JsonUtils::getHex(body, "basic_dark_hex_color"),
        JsonUtils::getFloat(body, "basic_dark_opacity")
    );
    m_window->renderer->extraLightColor = D2D1::ColorF(
        JsonUtils::getHex(body, "extra_light_hex_color"),
        JsonUtils::getFloat(body, "extra_light_opacity")
    );
    m_window->renderer->extraDarkColor = D2D1::ColorF(
        JsonUtils::getHex(body, "extra_dark_hex_color"),
        JsonUtils::getFloat(body, "extra_dark_opacity")
    );
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleStyle(const std::string& body) {
    m_window->renderer->basicFontWeight = (DWRITE_FONT_WEIGHT)JsonUtils::getInt(body, "basic_weight_value");
    m_window->renderer->basicFontStyle = (DWRITE_FONT_STYLE)JsonUtils::getInt(body, "basic_slope");
    m_window->renderer->basicUnderline = JsonUtils::getBool(body, "basic_underline");
    m_window->renderer->basicStrikethrough = JsonUtils::getBool(body, "basic_strikethrough");
    
    m_window->renderer->extraFontWeight = (DWRITE_FONT_WEIGHT)JsonUtils::getInt(body, "extra_weight_value");
    m_window->renderer->extraFontStyle = (DWRITE_FONT_STYLE)JsonUtils::getInt(body, "extra_slope");
    m_window->renderer->extraUnderline = JsonUtils::getBool(body, "extra_underline");
    m_window->renderer->extraStrikethrough = JsonUtils::getBool(body, "extra_strikethrough");

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleSize(const std::string& body) {
    float basic = JsonUtils::getFloat(body, "basic");
    float extra = JsonUtils::getFloat(body, "extra");

    if (basic > 0) {
        m_window->renderer->basicFontSize = basic;
    }
    
    if (extra > 0) {
        m_window->renderer->extraFontSize = extra;
        m_window->renderer->basicFontSizeDoubleLine = extra;
    }

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleLyrics(const std::string& body) {
    m_window->renderer->basicLyrics = utf8ToWide(JsonUtils::getString(body, "basic"));
    m_window->renderer->extraLyrics = utf8ToWide(JsonUtils::getString(body, "extra"));
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleAlign(const std::string& body) {
    m_window->renderer->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)JsonUtils::getInt(body, "basic");
    m_window->renderer->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)JsonUtils::getInt(body, "extra");
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handlePosition(const std::string& body) {
    m_window->renderer->windowAlignment = (WindowAlignment)JsonUtils::getInt(body, "position_value");
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleMargin(const std::string& body) {
    m_window->renderer->leftMargin = JsonUtils::getInt(body, "left");
    m_window->renderer->rightMargin = JsonUtils::getInt(body, "right");
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleScreen(const std::string& body) {
    // Not implemented in original code but present in switch
}

void NetworkServer::handleClose(const std::string& body) {
    // Implemented via SendMessage in heartbeat or loop break
    SendMessage(m_window->windowHandle, WM_CLOSE, NULL, NULL);
}
