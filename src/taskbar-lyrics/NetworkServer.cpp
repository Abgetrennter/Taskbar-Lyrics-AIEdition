#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "NetworkServer.hpp"
#include "CreateWindow.hpp"
#include <d2d1.h>
#include <sstream>
#include <vector>
#include <algorithm>

网络服务器类::网络服务器类(
    任务栏窗口类* 任务栏窗口,
    unsigned short 端口
) {
    this->任务栏窗口 = 任务栏窗口;
    this->运行中 = true;
    this->网络服务器_线程 = new std::thread(&网络服务器类::监听线程函数, this, 端口);
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
    
    WSACleanup();
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
        this->处理连接(clientSocket);
    }
}

void 网络服务器类::处理连接(SOCKET clientSocket) {
    char buffer[8192]; // 8KB buffer, 足够容纳大多数请求
    int bytesRead = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
    if (bytesRead > 0) {
        buffer[bytesRead] = '\0';
        std::string request(buffer);
        
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
                else if (url == "/taskbar/lyrics/lyrics") 歌词(body);
                else if (url == "/taskbar/lyrics/align") 对齐(body);
                else if (url == "/taskbar/window/position") 位置(body);
                else if (url == "/taskbar/window/margin") 边距(body);
                else if (url == "/taskbar/window/screen") 屏幕(body);
                else if (url == "/taskbar/close") 关闭(body);
            }
        }
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
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
