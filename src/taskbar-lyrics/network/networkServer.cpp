#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "networkServer.hpp"
#include "../utils/json.hpp"
#include "../utils/logger.hpp"
#include "../utils/configManager.hpp"
#include <sstream>
#include <fstream>
#include <algorithm>
#include <wincrypt.h>
#include <chrono>

#pragma comment(lib, "Advapi32.lib")

using json = nlohmann::json;

static long long GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
}

static unsigned int ParseHexColor(const std::string& str) {
    std::string val = str;
    if (val.empty()) return 0;
    if (val[0] == '#') val = val.substr(1);
    try {
        return std::stoul(val, nullptr, 16);
    } catch (...) {
        return 0;
    }
}

NetworkServer::NetworkServer(LyricsWindow* window, unsigned short port)
{
    m_window = window;
    m_port = port;
    m_isRunning = false;
    start();
}

NetworkServer::~NetworkServer()
{
    stop();
    WSACleanup();
}

void NetworkServer::start()
{
    if (m_isRunning) return;
    m_isRunning = true;
    m_serverThread = new std::thread(&NetworkServer::listenThreadFunc, this, m_port);
}

void NetworkServer::stop()
{
    if (!m_isRunning) return;
    m_isRunning = false;
    
    // Close socket to break accept()
    if (m_listenSocket != INVALID_SOCKET) {
        closesocket(m_listenSocket);
        m_listenSocket = INVALID_SOCKET;
    }

    if (m_serverThread && m_serverThread->joinable()) {
        m_serverThread->join();
        delete m_serverThread;
    }
    m_serverThread = nullptr;
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

        size_t headerEnd = request.find("\r\n\r\n");
        if (headerEnd != std::string::npos) {
            std::string headers = request.substr(0, headerEnd);
            std::string body;

            // Parse Method
            size_t firstSpace = headers.find(' ');
            std::string method = "";
            if (firstSpace != std::string::npos) {
                method = headers.substr(0, firstSpace);
            }

            // Parse URL
            std::string url = "";
            size_t secondSpace = headers.find(' ', firstSpace + 1);
            if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
                url = headers.substr(firstSpace + 1, secondSpace - firstSpace - 1);
            }

            // Handle OPTIONS for CORS
            if (method == "OPTIONS") {
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Access-Control-Allow-Origin: *\r\n"
                                       "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                                       "Access-Control-Allow-Headers: Content-Type\r\n"
                                       "Content-Length: 0\r\n"
                                       "Connection: close\r\n\r\n";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
                closesocket(clientSocket);
                return;
            }
            
            // Check for Content-Length
             int contentLength = 0;
             std::string headersLower = headers;
             std::transform(headersLower.begin(), headersLower.end(), headersLower.begin(), ::tolower);
             
             size_t clPosLower = headersLower.find("content-length: ");
             if (clPosLower != std::string::npos) {
                 size_t clEnd = headersLower.find("\r\n", clPosLower);
                 std::string clStr = headersLower.substr(clPosLower + 16, clEnd - (clPosLower + 16));
                 try {
                     contentLength = std::stoi(clStr);
                 } catch (...) {
                     contentLength = 0;
                 }
             }

             if (contentLength > 0) {
                 std::string initialBody = request.substr(headerEnd + 4);
                 if (initialBody.length() < static_cast<size_t>(contentLength)) {
                     // Need to read more
                     int remaining = contentLength - (int)initialBody.length();
                     std::vector<char> extraBuffer(remaining + 1);
                     int totalExtraRead = 0;
                     while (totalExtraRead < remaining) {
                         int r = recv(clientSocket, extraBuffer.data() + totalExtraRead, remaining - totalExtraRead, 0);
                         if (r <= 0) break;
                         totalExtraRead += r;
                     }
                     extraBuffer[totalExtraRead] = '\0';
                     body = initialBody + std::string(extraBuffer.data(), totalExtraRead);
                 } else {
                     body = initialBody.substr(0, contentLength);
                 }
             } else {
                 body = request.substr(headerEnd + 4);
             }

            if (!url.empty()) {
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
                else if (url == "/taskbar/hitokoto") handleHitokoto(body);
                else if (url == "/config") { handleConfigPage(clientSocket); return; }
                else if (url == "/style.css") { handleStyleCss(clientSocket); return; }
                else if (url == "/api/config") {
                    if (method == "GET") handleGetConfig(clientSocket);
                    else if (method == "POST") handleUpdateConfig(clientSocket, body);
                    return; 
                }
                else if (url == "/api/reset") {
                     handleResetConfig(clientSocket);
                     return;
                }
                else {
                    Logger::Info("Unknown URL: %s", url.c_str());
                    std::string res = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
                    send(clientSocket, res.c_str(), (int)res.length(), 0);
                    closesocket(clientSocket);
                    return;
                }

                // For taskbar/* endpoints, we successfully handled it (or ignored it), so send 200 OK
                std::string response = "HTTP/1.1 200 OK\r\n"
                                       "Access-Control-Allow-Origin: *\r\n"
                                       "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                                       "Access-Control-Allow-Headers: Content-Type\r\n"
                                       "Content-Length: 0\r\n"
                                       "Connection: close\r\n\r\n";
                send(clientSocket, response.c_str(), (int)response.length(), 0);
                closesocket(clientSocket);
                return;
            }
        }
    }

    // If we get here (e.g. malformed request), close socket
    closesocket(clientSocket);
}

std::wstring NetworkServer::utf8ToWide(const std::string& str) {
    if (str.empty()) return L"";
    int size_needed = MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), NULL, 0);
    std::wstring wstrTo(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, &str[0], (int)str.size(), &wstrTo[0], size_needed);
    return wstrTo;
}

void NetworkServer::handleFont(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string font = j.value("font_family", "Microsoft YaHei");
        m_window->renderer->fontFamily = utf8ToWide(font);
        
        ConfigManager::getInstance().GetConfigMutable().font.fontFamily = font;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleColor(const std::string& body) {
    try {
        json j = json::parse(body);
        
        auto basic = j.value("basic", json::object());
        auto basicLight = basic.value("light", json::object());
        auto basicDark = basic.value("dark", json::object());

        auto extra = j.value("extra", json::object());
        auto extraLight = extra.value("light", json::object());
        auto extraDark = extra.value("dark", json::object());

        unsigned int bLHexVal = basicLight.value("hex_color", 0xFFFFFF);
        float bLOpacity = basicLight.value("opacity", 1.0f);
        
        unsigned int bDHexVal = basicDark.value("hex_color", 0x000000);
        float bDOpacity = basicDark.value("opacity", 1.0f);
        
        unsigned int eLHexVal = extraLight.value("hex_color", 0xCCCCCC);
        float eLOpacity = extraLight.value("opacity", 1.0f);
        
        unsigned int eDHexVal = extraDark.value("hex_color", 0x333333);
        float eDOpacity = extraDark.value("opacity", 1.0f);

        m_window->renderer->basicLightColor = D2D1::ColorF(bLHexVal, bLOpacity);
        m_window->renderer->basicDarkColor = D2D1::ColorF(bDHexVal, bDOpacity);
        m_window->renderer->extraLightColor = D2D1::ColorF(eLHexVal, eLOpacity);
        m_window->renderer->extraDarkColor = D2D1::ColorF(eDHexVal, eDOpacity);
        
        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.color.basic.light.hexColor = bLHexVal;
        c.color.basic.light.opacity = bLOpacity;
        c.color.basic.dark.hexColor = bDHexVal;
        c.color.basic.dark.opacity = bDOpacity;
        c.color.extra.light.hexColor = eLHexVal;
        c.color.extra.light.opacity = eLOpacity;
        c.color.extra.dark.hexColor = eDHexVal;
        c.color.extra.dark.opacity = eDOpacity;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleStyle(const std::string& body) {
    try {
        json j = json::parse(body);
        
        auto basic = j.value("basic", json::object());
        auto basicWeightObj = basic.value("weight", json::object());
        
        auto extra = j.value("extra", json::object());
        auto extraWeightObj = extra.value("weight", json::object());

        int basicWeight = basicWeightObj.value("value", 400);
        int basicSlope = basic.value("slope", 0);
        bool basicUnderline = basic.value("underline", false);
        bool basicStrikethrough = basic.value("strikethrough", false);
        
        int extraWeight = extraWeightObj.value("value", 400);
        int extraSlope = extra.value("slope", 0);
        bool extraUnderline = extra.value("underline", false);
        bool extraStrikethrough = extra.value("strikethrough", false);

        m_window->renderer->basicFontWeight = (DWRITE_FONT_WEIGHT)basicWeight;
        m_window->renderer->basicFontStyle = (DWRITE_FONT_STYLE)basicSlope;
        m_window->renderer->basicUnderline = basicUnderline;
        m_window->renderer->basicStrikethrough = basicStrikethrough;
        
        m_window->renderer->extraFontWeight = (DWRITE_FONT_WEIGHT)extraWeight;
        m_window->renderer->extraFontStyle = (DWRITE_FONT_STYLE)extraSlope;
        m_window->renderer->extraUnderline = extraUnderline;
        m_window->renderer->extraStrikethrough = extraStrikethrough;

        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.style.basic.weightValue = basicWeight;
        c.style.basic.slope = basicSlope;
        c.style.basic.underline = basicUnderline;
        c.style.basic.strikethrough = basicStrikethrough;
        c.style.extra.weightValue = extraWeight;
        c.style.extra.slope = extraSlope;
        c.style.extra.underline = extraUnderline;
        c.style.extra.strikethrough = extraStrikethrough;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleSize(const std::string& body) {
    try {
        json j = json::parse(body);
        float basic = j.value("basic", 0.0f);
        float extra = j.value("extra", 0.0f);

        auto& c = ConfigManager::getInstance().GetConfigMutable();

        if (basic > 0) {
            m_window->renderer->basicFontSize = basic;
            m_window->renderer->basicFontSizeDoubleLine = basic;
            c.size.basic = basic;
        }
        
        if (extra > 0) {
            m_window->renderer->extraFontSize = extra;
            c.size.extra = extra;
        }
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleLyrics(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string basic = j.value("basic", "");
        std::string extra = j.value("extra", "");
        
        m_window->renderer->basicLyrics = utf8ToWide(basic);
        m_window->renderer->extraLyrics = utf8ToWide(extra);
        
        m_window->renderer->lastLyricsUpdateTimestamp = GetCurrentTimestamp();
        m_window->renderer->isShowingHitokoto = false;

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleAlign(const std::string& body) {
    try {
        json j = json::parse(body);
        int basicAlign = j.value("basic", 0);
        int extraAlign = j.value("extra", 0);

        m_window->renderer->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)basicAlign;
        m_window->renderer->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)extraAlign;
        
        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.align.basic = basicAlign;
        c.align.extra = extraAlign;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handlePosition(const std::string& body) {
    try {
        json j = json::parse(body);
        auto posObj = j.value("position", json::object());
        int value = posObj.value("value", 0);
        std::string text = posObj.value("textContent", "");
        
        m_window->renderer->windowAlignment = (WindowAlignment)value;
        
        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.position.value = value;
        c.position.textContent = text;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleMargin(const std::string& body) {
    try {
        json j = json::parse(body);
        int left = j.value("left", 0);
        int right = j.value("right", 0);

        m_window->renderer->leftMargin = left;
        m_window->renderer->rightMargin = right;
        
        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.margin.left = left;
        c.margin.right = right;
        ConfigManager::getInstance().Save();

        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleScreen(const std::string& body) {
    try {
        json j = json::parse(body);
        auto parentTaskbar = j.value("parent_taskbar", json::object());
        std::string value = parentTaskbar.value("value", "Shell_TrayWnd");
        std::string text = parentTaskbar.value("textContent", "");
        
        m_window->renderer->updateParentTaskbar(value);
        
        auto& c = ConfigManager::getInstance().GetConfigMutable();
        c.screen.parentTaskbarValue = value;
        c.screen.parentTaskbarText = text;
        ConfigManager::getInstance().Save();
        
        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    } catch (...) {}
}

void NetworkServer::handleClose(const std::string& body) {
    m_window->renderer->basicLyrics = L"";
    m_window->renderer->extraLyrics = L"";
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleHitokoto(const std::string& body) {
    try {
        json j = json::parse(body);
        std::string jsonPath = j.value("hitokoto_json_path", "");
        int interval = j.value("hitokoto_interval", 30);

        auto& c = ConfigManager::getInstance().GetConfigMutable();
        if (!jsonPath.empty()) {
            c.hitokoto.jsonPath = jsonPath;
        }
        if (interval > 0) {
            c.hitokoto.interval = interval;
        }
        ConfigManager::getInstance().Save();
    } catch (...) {}
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return "";
    std::stringstream buffer;
    buffer << f.rdbuf();
    return buffer.str();
}

std::string findAsset(const std::string& name) {
    // Try absolute path known from context (Development)
    std::string sourcePath = "e:\\Code\\Taskbar-Lyrics-1.x.x\\src\\betterncm-plugin\\assets\\" + name;
    if (std::ifstream(sourcePath).good()) return sourcePath;
    
    // Try relative to exe (Deployment)
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    std::string exePath(path);
    std::string dir = exePath.substr(0, exePath.find_last_of("\\"));
    std::string assetPath = dir + "\\assets\\" + name; 
    if (std::ifstream(assetPath).good()) return assetPath;

    // Try one level up
    std::string upOne = dir + "\\..\\" + name;
    if (std::ifstream(upOne).good()) return upOne;

    return "";
}

void NetworkServer::handleConfigPage(SOCKET clientSocket) {
    std::string htmlPath = findAsset("config.html");
    std::string htmlContent = readFile(htmlPath);

    if (htmlContent.empty()) {
        std::string res = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\nConfig file not found in assets.";
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    std::string fullHtml = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Taskbar Lyrics Settings</title>";
    fullHtml += "<style>";
    fullHtml += "body { background: #1a1a2e; color: #e0e0e0; padding: 20px; font-family: 'Microsoft YaHei UI', sans-serif; }";
    fullHtml += "#taskbar-lyrics-dom .tab_box { display: flex; gap: 4px; border-bottom: 1px solid #333; padding-bottom: 4px; }";
    fullHtml += "#taskbar-lyrics-dom .tab_button { cursor: pointer; padding: 8px 16px; border: none; background: none; color: #aaa; font-size: 14px; border-radius: 4px 4px 0 0; }";
    fullHtml += "#taskbar-lyrics-dom .tab_button:hover { color: #fff; background: rgba(255,255,255,0.05); }";
    fullHtml += "#taskbar-lyrics-dom .tab_button.active { color: #fff; background: #0078D7; font-weight: bold; }";
    fullHtml += "#taskbar-lyrics-dom .content_box { margin-top: 16px; }";
    fullHtml += "#taskbar-lyrics-dom .content { display: none; }";
    fullHtml += "#taskbar-lyrics-dom .content.show { display: block; }";
    fullHtml += "#taskbar-lyrics-dom .item_container { display: flex; align-items: center; gap: 10px; margin: 8px 0; }";
    fullHtml += "#taskbar-lyrics-dom .text_container { display: flex; flex-direction: column; align-items: flex-start; gap: 4px; }";
    fullHtml += "#taskbar-lyrics-dom .text_container p { margin: 4px 0; }";
    fullHtml += "#taskbar-lyrics-dom h1 { display: flex; align-items: center; gap: 10px; margin: 12px 0; }";
    fullHtml += "#taskbar-lyrics-dom h1 strong { font-size: 16px; }";
    fullHtml += "#taskbar-lyrics-dom hr { border: none; border-top: 1px solid #333; margin: 16px 0; }";
    fullHtml += "#taskbar-lyrics-dom input.u-txt { background: #2a2a3e; border: 1px solid #444; color: #e0e0e0; border-radius: 4px; padding: 4px 8px; }";
    fullHtml += "#taskbar-lyrics-dom input.u-txt:focus { outline: none; border-color: #0078D7; }";
    fullHtml += "input.color-input { width: 60px; height: 28px; padding: 2px; cursor: pointer; }";
    fullHtml += "input.number-input { width: 120px; }";
    fullHtml += "button { cursor: pointer; padding: 6px 14px; border: 1px solid #555; border-radius: 4px; background: #2a2a3e; color: #e0e0e0; font-size: 13px; }";
    fullHtml += "button:hover { background: #3a3a4e; }";
    fullHtml += ".u-ibtn5 { background: #0078D7; border-color: #0078D7; color: #fff; }";
    fullHtml += ".u-ibtn5:hover { background: #1a8ae8; }";
    fullHtml += ".switch-btn { width: 44px; height: 22px; appearance: none; border-radius: 20px; border: 1px solid #555; background: #333; position: relative; cursor: pointer; }";
    fullHtml += ".switch-btn::before { content: ''; position: absolute; top: 2px; left: 2px; width: 16px; height: 16px; border-radius: 50%; background: #888; transition: all 0.2s; }";
    fullHtml += ".switch-btn:checked { background: #0078D7; border-color: #0078D7; }";
    fullHtml += ".switch-btn:checked::before { left: calc(100% - 18px); background: #fff; }";
    fullHtml += ".u-select { display: inline-block; position: relative; min-width: 200px; }";
    fullHtml += ".u-select .value { padding: 6px 10px; background: #2a2a3e; border: 1px solid #444; border-radius: 4px; cursor: pointer; }";
    fullHtml += ".u-select .sltwrap { display: none; position: absolute; top: 100%; left: 0; right: 0; background: #2a2a3e; border: 1px solid #444; border-radius: 0 0 4px 4px; z-index: 100; max-height: 200px; overflow-y: auto; }";
    fullHtml += ".u-select .option { padding: 6px 10px; cursor: pointer; list-style: none; }";
    fullHtml += ".u-select .option:hover { background: #3a3a4e; }";
    fullHtml += ".slope-active { background: #0078D7 !important; color: #fff !important; border-color: #0078D7 !important; }";
    fullHtml += "</style>";
    fullHtml += "</head><body>";
    fullHtml += htmlContent;

    // Inject JS
    fullHtml += "<script>";
    fullHtml += R"js(
        // --- Helpers ---
        const $ = s => document.querySelector(s);
        const $$ = s => document.querySelectorAll(s);
        const hexToColor = n => '#' + (n || 0).toString(16).padStart(6, '0');
        const colorToHex = c => parseInt(c.substring(1), 16);

        // --- Tabs ---
        const tabs = $$('.tab_button');
        const contents = $$('.content');
        tabs.forEach((tab, i) => {
            tab.addEventListener('click', () => {
                tabs.forEach(t => t.classList.remove('active'));
                contents.forEach(c => c.classList.remove('show'));
                tab.classList.add('active');
                if (contents[i]) contents[i].classList.add('show');
            });
        });

        // --- Custom Select ---
        document.addEventListener('click', () => {
            $$('.sltwrap').forEach(w => w.style.display = 'none');
        });
        $$('.u-select').forEach(sel => {
            const val = sel.querySelector('.value');
            const wrap = sel.querySelector('.sltwrap');
            sel.addEventListener('click', e => {
                e.stopPropagation();
                $$('.sltwrap').forEach(w => { if (w !== wrap) w.style.display = 'none'; });
                wrap.style.display = wrap.style.display === 'block' ? 'none' : 'block';
            });
            sel.querySelectorAll('.option').forEach(opt => {
                opt.addEventListener('click', e => {
                    e.stopPropagation();
                    val.textContent = opt.textContent;
                    sel.dataset.value = opt.dataset.value;
                    wrap.style.display = 'none';
                });
            });
        });

        // --- Slope buttons ---
        const slopeMap = { normal: 0, oblique: 1, italic: 2 };
        const slopeRev = { 0: 'normal', 1: 'oblique', 2: 'italic' };
        let selSlope = { basic: 0, extra: 0 };

        ['basic', 'extra'].forEach(type => {
            ['normal', 'oblique', 'italic'].forEach(slope => {
                const btn = $(`.${type}-${slope}`);
                if (btn) btn.addEventListener('click', () => {
                    [`${type}-normal`, `${type}-oblique`, `${type}-italic`].forEach(c => {
                        const el = $(`.${c}`); if (el) el.classList.remove('slope-active');
                    });
                    btn.classList.add('slope-active');
                    selSlope[type] = slopeMap[slope];
                });
            });
        });

        // --- Align buttons ---
        const alignMap = { left: 0, center: 2, right: 1 };
        const alignRev = { 0: 'left', 1: 'right', 2: 'center' };
        let selAlign = { basic: 0, extra: 0 };

        ['basic', 'extra'].forEach(type => {
            ['left', 'center', 'right'].forEach(align => {
                const btn = $(`.${type}-align-${align}`);
                if (btn) btn.addEventListener('click', () => {
                    [`${type}-align-left`, `${type}-align-center`, `${type}-align-right`].forEach(c => {
                        const el = $(`.${c}`); if (el) el.classList.remove('slope-active');
                    });
                    btn.classList.add('slope-active');
                    selAlign[type] = alignMap[align];
                });
            });
        });

        // --- Load config from backend ---
        function loadConfig() {
            fetch('/api/config').then(r => r.json()).then(d => {
                if (!d) return;

                // Font
                const fontEl = $('.font-family');
                if (fontEl) fontEl.value = d.font?.font_family || '';

                // Color
                const setColor = (sel, val) => { const el = $(sel); if (el) el.value = val; };
                if (d.color) {
                    setColor('.basic-light-color', hexToColor(d.color.basic?.light?.hex_color));
                    setColor('.basic-light-opacity', d.color.basic?.light?.opacity);
                    setColor('.basic-dark-color', hexToColor(d.color.basic?.dark?.hex_color));
                    setColor('.basic-dark-opacity', d.color.basic?.dark?.opacity);
                    setColor('.extra-light-color', hexToColor(d.color.extra?.light?.hex_color));
                    setColor('.extra-light-opacity', d.color.extra?.light?.opacity);
                    setColor('.extra-dark-color', hexToColor(d.color.extra?.dark?.hex_color));
                    setColor('.extra-dark-opacity', d.color.extra?.dark?.opacity);
                }

                // Size
                if (d.size) {
                    setColor('.basic-size', d.size.basic);
                    setColor('.extra-size', d.size.extra);
                }

                // Style
                if (d.style) {
                    const bw = $('.basic-weight');
                    if (bw) { bw.dataset.value = d.style.basic?.weight?.value; bw.querySelector('.value').textContent = d.style.basic?.weight?.textContent || ''; }
                    const bs = slopeRev[d.style.basic?.slope];
                    if (bs) $(`.basic-${bs}`)?.classList.add('slope-active');
                    selSlope.basic = d.style.basic?.slope || 0;
                    setColor('.basic-underline', d.style.basic?.underline);
                    const bu = $('.basic-underline'); if (bu) bu.checked = !!d.style.basic?.underline;
                    const bst = $('.basic-strikethrough'); if (bst) bst.checked = !!d.style.basic?.strikethrough;

                    const ew = $('.extra-weight');
                    if (ew) { ew.dataset.value = d.style.extra?.weight?.value; ew.querySelector('.value').textContent = d.style.extra?.weight?.textContent || ''; }
                    const es = slopeRev[d.style.extra?.slope];
                    if (es) $(`.extra-${es}`)?.classList.add('slope-active');
                    selSlope.extra = d.style.extra?.slope || 0;
                    const eu = $('.extra-underline'); if (eu) eu.checked = !!d.style.extra?.underline;
                    const est = $('.extra-strikethrough'); if (est) est.checked = !!d.style.extra?.strikethrough;
                }

                // Lyrics
                if (d.lyrics) {
                    const rm = $('.retrieval-method');
                    if (rm) { rm.dataset.value = d.lyrics.retrieval_method?.value; rm.querySelector('.value').textContent = d.lyrics.retrieval_method?.textContent || ''; }
                    const k = $('.karaoke-switch'); if (k) k.checked = !!d.lyrics.karaoke;
                }

                // Effect
                if (d.effect) {
                    const nlp = $('.next-line-lyrics-position');
                    if (nlp) { nlp.dataset.value = d.effect.next_line_lyrics_position?.value; nlp.querySelector('.value').textContent = d.effect.next_line_lyrics_position?.textContent || ''; }
                    const es2 = $('.extra-show');
                    if (es2) { es2.dataset.value = d.effect.extra_show?.value; es2.querySelector('.value').textContent = d.effect.extra_show?.textContent || ''; }
                    setColor('.adjust', d.effect.adjust);
                }

                // Align
                if (d.align) {
                    selAlign.basic = d.align.basic ?? 0;
                    selAlign.extra = d.align.extra ?? 0;
                    const ba = alignRev[d.align.basic]; if (ba) $(`.basic-align-${ba}`)?.classList.add('slope-active');
                    const ea = alignRev[d.align.extra]; if (ea) $(`.extra-align-${ea}`)?.classList.add('slope-active');
                }

                // Position
                if (d.position?.position) {
                    const wp = $('.window-position');
                    if (wp) { wp.dataset.value = d.position.position.value; wp.querySelector('.value').textContent = d.position.position.textContent || ''; }
                }

                // Margin
                if (d.margin) {
                    setColor('.margin-settings .left', d.margin.left);
                    setColor('.margin-settings .right', d.margin.right);
                }

                // Screen
                if (d.screen?.parent_taskbar) {
                    const pt = $('.parent-taskbar');
                    if (pt) { pt.dataset.value = d.screen.parent_taskbar.value; pt.querySelector('.value').textContent = d.screen.parent_taskbar.textContent || ''; }
                }

                // Hitokoto
                if (d.hitokoto) {
                    setColor('.hitokoto-json-path', d.hitokoto.hitokoto_json_path || '');
                    setColor('.hitokoto-interval', d.hitokoto.hitokoto_interval || 30);
                }
            });
        }
        loadConfig();

        // --- Build full config from current UI state ---
        function buildConfig() {
            return {
                font: { font_family: $('.font-family')?.value || '' },
                color: {
                    basic: {
                        light: { hex_color: colorToHex($('.basic-light-color')?.value || '#000000'), opacity: parseFloat($('.basic-light-opacity')?.value || 1) },
                        dark: { hex_color: colorToHex($('.basic-dark-color')?.value || '#ffffff'), opacity: parseFloat($('.basic-dark-opacity')?.value || 1) }
                    },
                    extra: {
                        light: { hex_color: colorToHex($('.extra-light-color')?.value || '#000000'), opacity: parseFloat($('.extra-light-opacity')?.value || 1) },
                        dark: { hex_color: colorToHex($('.extra-dark-color')?.value || '#ffffff'), opacity: parseFloat($('.extra-dark-opacity')?.value || 1) }
                    }
                },
                size: {
                    basic: parseFloat($('.basic-size')?.value || 20),
                    extra: parseFloat($('.extra-size')?.value || 16)
                },
                style: {
                    basic: {
                        weight: { value: parseInt($('.basic-weight')?.dataset.value || 400), textContent: $('.basic-weight .value')?.textContent || '' },
                        slope: selSlope.basic, underline: !!$('.basic-underline')?.checked, strikethrough: !!$('.basic-strikethrough')?.checked
                    },
                    extra: {
                        weight: { value: parseInt($('.extra-weight')?.dataset.value || 400), textContent: $('.extra-weight .value')?.textContent || '' },
                        slope: selSlope.extra, underline: !!$('.extra-underline')?.checked, strikethrough: !!$('.extra-strikethrough')?.checked
                    }
                },
                lyrics: {
                    retrieval_method: { value: parseInt($('.retrieval-method')?.dataset.value || 1), textContent: $('.retrieval-method .value')?.textContent || '' },
                    karaoke: !!$('.karaoke-switch')?.checked
                },
                effect: {
                    next_line_lyrics_position: { value: parseInt($('.next-line-lyrics-position')?.dataset.value || 0), textContent: $('.next-line-lyrics-position .value')?.textContent || '' },
                    extra_show: { value: parseInt($('.extra-show')?.dataset.value || 2), textContent: $('.extra-show .value')?.textContent || '' },
                    adjust: parseFloat($('.adjust')?.value || 0)
                },
                align: { basic: selAlign.basic, extra: selAlign.extra },
                position: { position: { value: parseInt($('.window-position')?.dataset.value || 0), textContent: $('.window-position .value')?.textContent || '' } },
                margin: { left: parseInt($('.margin-settings .left')?.value || 0), right: parseInt($('.margin-settings .right')?.value || 0) },
                screen: { parent_taskbar: { value: $('.parent-taskbar')?.dataset.value || 'Shell_TrayWnd', textContent: $('.parent-taskbar .value')?.textContent || '' } }
            };
        }

        // --- Apply buttons: save then reload ---
        $$('.apply').forEach(btn => {
            btn.addEventListener('click', () => {
                const config = buildConfig();
                fetch('/api/config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(config)
                }).then(() => {
                    // Also send hitokoto separately for backward compat
                    fetch('/taskbar/hitokoto', {
                        method: 'POST',
                        headers: { 'Content-Type': 'application/json' },
                        body: JSON.stringify({
                            hitokoto_json_path: $('.hitokoto-json-path')?.value || '',
                            hitokoto_interval: parseInt($('.hitokoto-interval')?.value || 30)
                        })
                    });
                    alert('保存成功');
                });
            });
        });

        // --- Reset buttons ---
        $$('.reset').forEach(btn => {
            btn.addEventListener('click', () => {
                if (confirm('确定要恢复默认设置吗？')) {
                    fetch('/api/reset', { method: 'POST' }).then(() => {
                        alert('已重置');
                        loadConfig();
                    });
                }
            });
        });
    )js";
    fullHtml += "</script></body></html>";

    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Content-Type: text/html; charset=utf-8\r\n"
                           "Content-Length: " + std::to_string(fullHtml.length()) + "\r\n\r\n" + fullHtml;
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

void NetworkServer::handleStyleCss(SOCKET clientSocket) {
    std::string cssPath = findAsset("style.css");
    std::string cssContent = readFile(cssPath);
    
    if (cssContent.empty()) {
        std::string res = "HTTP/1.1 404 Not Found\r\n\r\n";
        send(clientSocket, res.c_str(), (int)res.length(), 0);
        closesocket(clientSocket);
        return;
    }

    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/css\r\nContent-Length: " + std::to_string(cssContent.length()) + "\r\n\r\n" + cssContent;
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

void NetworkServer::handleGetConfig(SOCKET clientSocket) {
    std::string json = ConfigManager::getInstance().GetJSON();
    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: Content-Type\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: " + std::to_string(json.length()) + "\r\n\r\n" + json;
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

void NetworkServer::handleUpdateConfig(SOCKET clientSocket, const std::string& body) {
    ConfigManager::getInstance().UpdateFromJSON(body);
    ConfigManager::getInstance().Save();
    
    // Apply to renderer
    const auto& config = ConfigManager::getInstance().GetConfig();
    if (m_window && m_window->renderer) {
        auto r = m_window->renderer;
        r->fontFamily = utf8ToWide(config.font.fontFamily);
        
        r->basicFontSize = config.size.basic;
        r->extraFontSize = config.size.extra;
        r->basicFontSizeDoubleLine = config.size.basic;

        r->basicLightColor = D2D1::ColorF(config.color.basic.light.hexColor, config.color.basic.light.opacity);
        r->basicDarkColor = D2D1::ColorF(config.color.basic.dark.hexColor, config.color.basic.dark.opacity);
        r->extraLightColor = D2D1::ColorF(config.color.extra.light.hexColor, config.color.extra.light.opacity);
        r->extraDarkColor = D2D1::ColorF(config.color.extra.dark.hexColor, config.color.extra.dark.opacity);
        
        r->basicFontWeight = (DWRITE_FONT_WEIGHT)config.style.basic.weightValue;
        r->basicFontStyle = (DWRITE_FONT_STYLE)config.style.basic.slope;
        r->basicUnderline = config.style.basic.underline;
        r->basicStrikethrough = config.style.basic.strikethrough;

        r->extraFontWeight = (DWRITE_FONT_WEIGHT)config.style.extra.weightValue;
        r->extraFontStyle = (DWRITE_FONT_STYLE)config.style.extra.slope;
        r->extraUnderline = config.style.extra.underline;
        r->extraStrikethrough = config.style.extra.strikethrough;
        
        r->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.basic;
        r->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.extra;
        
        r->windowAlignment = (WindowAlignment)config.position.value;
        r->leftMargin = config.margin.left;
        r->rightMargin = config.margin.right;
        
        r->updateParentTaskbar(config.screen.parentTaskbarValue);
        
        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    }

    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: Content-Type\r\n"
                           "Content-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

void NetworkServer::handleResetConfig(SOCKET clientSocket) {
    ConfigManager::getInstance().Reset();
    
    // Apply default config
    const auto& config = ConfigManager::getInstance().GetConfig();
    if (m_window && m_window->renderer) {
        auto r = m_window->renderer;
        r->fontFamily = utf8ToWide(config.font.fontFamily);
        
        r->basicFontSize = config.size.basic;
        r->extraFontSize = config.size.extra;
        r->basicFontSizeDoubleLine = config.size.basic;

        r->basicLightColor = D2D1::ColorF(config.color.basic.light.hexColor, config.color.basic.light.opacity);
        r->basicDarkColor = D2D1::ColorF(config.color.basic.dark.hexColor, config.color.basic.dark.opacity);
        r->extraLightColor = D2D1::ColorF(config.color.extra.light.hexColor, config.color.extra.light.opacity);
        r->extraDarkColor = D2D1::ColorF(config.color.extra.dark.hexColor, config.color.extra.dark.opacity);
        
        r->basicFontWeight = (DWRITE_FONT_WEIGHT)config.style.basic.weightValue;
        r->basicFontStyle = (DWRITE_FONT_STYLE)config.style.basic.slope;
        r->basicUnderline = config.style.basic.underline;
        r->basicStrikethrough = config.style.basic.strikethrough;

        r->extraFontWeight = (DWRITE_FONT_WEIGHT)config.style.extra.weightValue;
        r->extraFontStyle = (DWRITE_FONT_STYLE)config.style.extra.slope;
        r->extraUnderline = config.style.extra.underline;
        r->extraStrikethrough = config.style.extra.strikethrough;
        
        r->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.basic;
        r->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.extra;
        
        r->windowAlignment = (WindowAlignment)config.position.value;
        r->leftMargin = config.margin.left;
        r->rightMargin = config.margin.right;
        
        r->updateParentTaskbar(config.screen.parentTaskbarValue);
        
        PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
    }
    
    std::string response = "HTTP/1.1 200 OK\r\n"
                           "Access-Control-Allow-Origin: *\r\n"
                           "Access-Control-Allow-Methods: POST, GET, OPTIONS\r\n"
                           "Access-Control-Allow-Headers: Content-Type\r\n"
                           "Content-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}
