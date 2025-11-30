#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "NetworkServer.hpp"
#include "CreateWindow.hpp"
#include <d2d1.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <wincrypt.h>

#pragma comment(lib, "Advapi32.lib")

// 时间戳获取
static long long GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

网络服务器类::网络服务器类(
    任务栏窗口类* 任务栏窗口,
    unsigned short 端口
) {
    this->任务栏窗口 = 任务栏窗口;
    this->运行中 = true;
    this->上次心跳时间 = GetCurrentTimestamp();
    this->网络服务器_线程 = new std::thread(&网络服务器类::监听线程函数, this, 端口);
    this->心跳检测_线程 = new std::thread(&网络服务器类::心跳检测函数, this);
}

网络服务器类::~网络服务器类()
{
    this->运行中 = false;
    // 尝试连接自己以打破 accept 阻塞，或者直接关闭 socket
    if (this->监听Socket != INVALID_SOCKET) {
        closesocket(this->监听Socket);
        this->监听Socket = INVALID_SOCKET;
    }
    
    if (this->网络服务器_线程 && this->网络服务器_线程->joinable()) {
        this->网络服务器_线程->join();
        delete this->网络服务器_线程;
    }
    this->网络服务器_线程 = nullptr;
    
    if (this->心跳检测_线程 && this->心跳检测_线程->joinable()) {
        this->心跳检测_线程->join();
        delete this->心跳检测_线程;
    }
    this->心跳检测_线程 = nullptr;

    WSACleanup();
}

void 网络服务器类::心跳检测函数() {
    while (this->运行中) {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        long long now = GetCurrentTimestamp();
        // 10秒没有心跳就退出 (WebSocket保持连接也算心跳，或者单独发送心跳包)
        // 初始给予 30 秒缓冲
        static bool initialized = false;
        if (!initialized) {
             if (now - this->上次心跳时间 > 30000) initialized = true;
             else continue;
        }

        if (now - this->上次心跳时间 > 10000) {
            // 超时退出
            SendMessage(this->任务栏窗口->窗口句柄, WM_CLOSE, NULL, NULL);
            break;
        }
    }
}

void 网络服务器类::监听线程函数(unsigned short 端口) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return;

    this->监听Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (this->监听Socket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(端口);

    if (bind(this->监听Socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(this->监听Socket);
        WSACleanup();
        return;
    }

    if (listen(this->监听Socket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(this->监听Socket);
        WSACleanup();
        return;
    }

    while (this->运行中) {
        SOCKET clientSocket = accept(this->监听Socket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            if (this->运行中) continue; 
            else break;
        }
        
        // 设置接收超时，防止 WebSocket 意外断开导致线程卡死
        // DWORD timeout = 5000; 
        // setsockopt(clientSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

        this->处理连接(clientSocket);
    }
}

void 网络服务器类::处理连接(SOCKET clientSocket) {
    char buffer[8192]; 
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        
        // 更新心跳时间
        this->上次心跳时间 = GetCurrentTimestamp();

        // 检查是否为 WebSocket 升级请求
        if (this->是WebSocket请求(request)) {
            this->WebSocket握手(clientSocket, request);
            
            // 进入 WebSocket 消息循环
            std::string message;
            while (this->运行中) {
                if (!this->读取WebSocket帧(clientSocket, message)) {
                    break; // 连接关闭或错误
                }
                
                // 更新心跳
                this->上次心跳时间 = GetCurrentTimestamp();

                // 解析 JSON 消息
                // 假设 WebSocket 发送的是纯 JSON 字符串
                if (message.empty()) continue;

                // 简单的路由分发，这里需要解析 JSON 中的 type 字段或者 url 字段
                // 为了兼容现有的 API，我们可以约定 WebSocket 发送的数据格式为 { "url": "/taskbar/...", "body": { ... } }
                // 或者直接复用现有的解析逻辑，解析 body
                
                std::string url = JsonGetString(message, "url");
                std::string body = message; // 整个 message 作为 body，因为 JsonXXX 函数是在整个 json 串中查找

                if (url == "/taskbar/font/font") 字体(body);
                else if (url == "/taskbar/font/color") 颜色(body);
                else if (url == "/taskbar/font/style") 样式(body);
                else if (url == "/taskbar/font/size") 大小(body);
                else if (url == "/taskbar/lyrics/lyrics") 歌词(body);
                else if (url == "/taskbar/lyrics/align") 对齐(body);
                else if (url == "/taskbar/window/position") 位置(body);
                else if (url == "/taskbar/window/margin") 边距(body);
                else if (url == "/taskbar/window/screen") 屏幕(body);
                else if (url == "/taskbar/close") {
                    关闭(body);
                    break;
                }
                else if (url == "/taskbar/heartbeat") {
                    // 心跳包，仅更新时间
                }
            }
            closesocket(clientSocket);
            return;
        }

        // HTTP 处理逻辑
        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headers = request.substr(0, headerEnd);
            std::string body = request.substr(headerEnd + 4);
            
            // 解析 URL
            size_t firstSpace = headers.find(' ');
            size_t secondSpace = headers.find(' ', firstSpace + 1);
            if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
                std::string url = headers.substr(firstSpace + 1, secondSpace - firstSpace - 1);
                
                if (url == "/taskbar/font/font") 字体(body);
                else if (url == "/taskbar/font/color") 颜色(body);
                else if (url == "/taskbar/font/style") 样式(body);
                else if (url == "/taskbar/font/size") 大小(body);
                else if (url == "/taskbar/lyrics/lyrics") 歌词(body);
                else if (url == "/taskbar/lyrics/align") 对齐(body);
                else if (url == "/taskbar/window/position") 位置(body);
                else if (url == "/taskbar/window/margin") 边距(body);
                else if (url == "/taskbar/window/screen") 屏幕(body);
                else if (url == "/taskbar/close") 关闭(body);
                else if (url == "/taskbar/heartbeat") { /* 心跳 */ }
            }
        }
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

bool 网络服务器类::是WebSocket请求(const std::string& request) {
    return request.find("Upgrade: websocket") != std::string::npos || 
           request.find("Upgrade: WebSocket") != std::string::npos;
}

std::string 网络服务器类::Base64Encode(const unsigned char* data, size_t len) {
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

std::string 网络服务器类::计算WebSocketKey(const std::string& key) {
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

    return Base64Encode(rgbHash, 20);
}

void 网络服务器类::WebSocket握手(SOCKET clientSocket, const std::string& request) {
    std::string keyPrefix = "Sec-WebSocket-Key: ";
    size_t keyStart = request.find(keyPrefix);
    if (keyStart == std::string::npos) return;
    
    keyStart += keyPrefix.length();
    size_t keyEnd = request.find("\r\n", keyStart);
    std::string key = request.substr(keyStart, keyEnd - keyStart);
    
    std::string acceptKey = 计算WebSocketKey(key);
    
    std::stringstream response;
    response << "HTTP/1.1 101 Switching Protocols\r\n";
    response << "Upgrade: websocket\r\n";
    response << "Connection: Upgrade\r\n";
    response << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";
    
    std::string resStr = response.str();
    send(clientSocket, resStr.c_str(), (int)resStr.length(), 0);
}

bool 网络服务器类::读取WebSocket帧(SOCKET clientSocket, std::string& outMessage) {
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
        // 简化处理，假设长度不超过 size_t
        payloadLen = 0; // TODO: handle 64-bit length correctly if needed
        // 这里只取低位，实际上歌词不会这么长
        payloadLen = lenBytes[7]; 
        // Correct logic for small payloads that happen to use 64-bit length (unlikely but possible)
        // A better way is to cast if we are sure it fits
        // For now, let's assume it fits in size_t and is not huge.
        // Let's just take the last 4 bytes if size_t is 32-bit, or all 8 if 64-bit.
        // But for this specific app, payload is JSON, unlikely to exceed 4GB.
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
        // Limit payload size to avoid memory issues and overflow on 32-bit systems
        // 16MB limit should be enough for lyrics/json
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

// 简易 JSON 解析器实现
std::string 网络服务器类::JsonGetString(const std::string& json, const std::string& key) {
    std::string keyPattern = "\"" + key + "\":";
    size_t pos = json.find(keyPattern);
    if (pos == std::string::npos) return "";
    
    pos += keyPattern.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n')) pos++;
    
    if (pos >= json.length() || json[pos] != '"') return "";
    pos++; // skip quote
    
    size_t endPos = pos;
    while (endPos < json.length()) {
        if (json[endPos] == '"' && (endPos == 0 || json[endPos - 1] != '\\')) break;
        endPos++;
    }
    
    // 处理简单的转义字符 (仅处理 \" 和 \\)
    std::string result = json.substr(pos, endPos - pos);
    size_t found = result.find("\\\"");
    while (found != std::string::npos) {
        result.replace(found, 2, "\"");
        found = result.find("\\\"", found + 1);
    }
    return result;
}

int 网络服务器类::JsonGetInt(const std::string& json, const std::string& key) {
    std::string keyPattern = "\"" + key + "\":";
    size_t pos = json.find(keyPattern);
    if (pos == std::string::npos) return 0;
    pos += keyPattern.length();
    while (pos < json.length() && !isdigit(json[pos]) && json[pos] != '-') pos++;
    return std::atoi(json.c_str() + pos);
}

float 网络服务器类::JsonGetFloat(const std::string& json, const std::string& key) {
    std::string keyPattern = "\"" + key + "\":";
    size_t pos = json.find(keyPattern);
    if (pos == std::string::npos) return 0.0f;
    pos += keyPattern.length();
    while (pos < json.length() && !isdigit(json[pos]) && json[pos] != '-' && json[pos] != '.') pos++;
    return (float)std::atof(json.c_str() + pos);
}

bool 网络服务器类::JsonGetBool(const std::string& json, const std::string& key) {
    std::string keyPattern = "\"" + key + "\":";
    size_t pos = json.find(keyPattern);
    if (pos == std::string::npos) return false;
    pos += keyPattern.length();
    while (pos < json.length() && isspace(json[pos])) pos++;
    if (json.substr(pos, 4) == "true") return true;
    return false;
}

unsigned int 网络服务器类::JsonGetHex(const std::string& json, const std::string& key) {
    return (unsigned int)JsonGetInt(json, key);
}

// 业务逻辑
void 网络服务器类::字体(const std::string& body) {
    this->任务栏窗口->呈现窗口->字体名称 = this->字符转换.from_bytes(
        JsonGetString(body, "font_family")
    );
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::颜色(const std::string& body) {
    this->任务栏窗口->呈现窗口->字体颜色_浅色_主歌词 = D2D1::ColorF(
        JsonGetHex(body, "basic_light_hex_color"),
        JsonGetFloat(body, "basic_light_opacity")
    );
    this->任务栏窗口->呈现窗口->字体颜色_深色_主歌词 = D2D1::ColorF(
        JsonGetHex(body, "basic_dark_hex_color"),
        JsonGetFloat(body, "basic_dark_opacity")
    );
    this->任务栏窗口->呈现窗口->字体颜色_浅色_副歌词 = D2D1::ColorF(
        JsonGetHex(body, "extra_light_hex_color"),
        JsonGetFloat(body, "extra_light_opacity")
    );
    this->任务栏窗口->呈现窗口->字体颜色_深色_副歌词 = D2D1::ColorF(
        JsonGetHex(body, "extra_dark_hex_color"),
        JsonGetFloat(body, "extra_dark_opacity")
    );
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::样式(const std::string& body) {
    this->任务栏窗口->呈现窗口->字体样式_主歌词_字重 = (DWRITE_FONT_WEIGHT)JsonGetInt(body, "basic_weight_value");
    this->任务栏窗口->呈现窗口->字体样式_主歌词_斜体 = (DWRITE_FONT_STYLE)JsonGetInt(body, "basic_slope");
    this->任务栏窗口->呈现窗口->字体样式_主歌词_下划线 = JsonGetBool(body, "basic_underline");
    this->任务栏窗口->呈现窗口->字体样式_主歌词_删除线 = JsonGetBool(body, "basic_strikethrough");
    
    this->任务栏窗口->呈现窗口->字体样式_副歌词_字重 = (DWRITE_FONT_WEIGHT)JsonGetInt(body, "extra_weight_value");
    this->任务栏窗口->呈现窗口->字体样式_副歌词_斜体 = (DWRITE_FONT_STYLE)JsonGetInt(body, "extra_slope");
    this->任务栏窗口->呈现窗口->字体样式_副歌词_下划线 = JsonGetBool(body, "extra_underline");
    this->任务栏窗口->呈现窗口->字体样式_副歌词_删除线 = JsonGetBool(body, "extra_strikethrough");

    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::大小(const std::string& body) {
    float basic = JsonGetFloat(body, "basic");
    float extra = JsonGetFloat(body, "extra");

    if (basic > 0) {
        this->任务栏窗口->呈现窗口->字体大小_主歌词 = basic;
    }
    
    if (extra > 0) {
        this->任务栏窗口->呈现窗口->字体大小_副歌词 = extra;
        this->任务栏窗口->呈现窗口->字体大小_主歌词_双行 = extra;
    }

    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::歌词(const std::string& body) {
    this->任务栏窗口->呈现窗口->主歌词 = this->字符转换.from_bytes(
        JsonGetString(body, "basic")
    );
    this->任务栏窗口->呈现窗口->副歌词 = this->字符转换.from_bytes(
        JsonGetString(body, "extra")
    );
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::对齐(const std::string& body) {
    this->任务栏窗口->呈现窗口->对齐方式_主歌词 = (DWRITE_TEXT_ALIGNMENT)JsonGetInt(body, "basic");
    this->任务栏窗口->呈现窗口->对齐方式_副歌词 = (DWRITE_TEXT_ALIGNMENT)JsonGetInt(body, "extra");
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::位置(const std::string& body) {
    this->任务栏窗口->呈现窗口->窗口位置 = (WindowAlignment)JsonGetInt(body, "position_value");
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::边距(const std::string& body) {
    this->任务栏窗口->呈现窗口->左边距 = JsonGetInt(body, "left");
    this->任务栏窗口->呈现窗口->右边距 = JsonGetInt(body, "right");
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::屏幕(const std::string& body) {
    std::string parent_taskbar = JsonGetString(body, "parent_taskbar_value");
    
    this->任务栏窗口->呈现窗口->任务栏_句柄 = FindWindow(this->字符转换.from_bytes(parent_taskbar).c_str(), NULL);
    this->任务栏窗口->呈现窗口->开始按钮_句柄 = FindWindowEx(this->任务栏窗口->呈现窗口->任务栏_句柄, NULL, L"Start", NULL);

    GetWindowRect(this->任务栏窗口->呈现窗口->任务栏_句柄, &this->任务栏窗口->呈现窗口->任务栏_矩形);
    GetWindowRect(this->任务栏窗口->呈现窗口->开始按钮_句柄, &this->任务栏窗口->呈现窗口->开始按钮_矩形);

    SetParent(this->任务栏窗口->窗口句柄, this->任务栏窗口->呈现窗口->任务栏_句柄);
    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
}

void 网络服务器类::关闭(const std::string& body) {
    this->任务栏窗口->呈现窗口->主歌词 = L"检测到网易云音乐重载页面";
    this->任务栏窗口->呈现窗口->副歌词 = L"正在尝试关闭任务栏歌词...";

    PostMessage(this->任务栏窗口->窗口句柄, WM_PAINT, NULL, NULL);
    PostMessage(this->任务栏窗口->窗口句柄, WM_CLOSE, NULL, NULL);
}