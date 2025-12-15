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
            c.size.basic = basic;
        }
        
        if (extra > 0) {
            m_window->renderer->extraFontSize = extra;
            m_window->renderer->basicFontSizeDoubleLine = extra;
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
    fullHtml += "<link rel='stylesheet' href='/style.css'>";
    fullHtml += "<style>body{background:#fff; padding:20px; font-family: 'Microsoft YaHei UI', sans-serif;} .tab_box button { cursor: pointer; padding: 10px; } .content_box { margin-top: 20px; } .content { display: none; } .content.show { display: block; } .active { font-weight: bold; border-bottom: 2px solid #0078D7; }</style>";
    fullHtml += "</head><body>";
    fullHtml += htmlContent;
    
    // Inject JS
    fullHtml += "<script>";
    fullHtml += R"(
        // Tabs
        const tabs = document.querySelectorAll('.tab_button');
        const contents = document.querySelectorAll('.content');
        tabs.forEach((tab, index) => {
            tab.addEventListener('click', () => {
                tabs.forEach(t => t.classList.remove('active'));
                contents.forEach(c => c.classList.remove('show'));
                tab.classList.add('active');
                if(contents[index]) contents[index].classList.add('show');
            });
        });

        function hexToColor(hex) {
            return '#' + (hex || 0).toString(16).padStart(6, '0');
        }
        
        function colorToHex(color) {
            return parseInt(color.substring(1), 16);
        }

        // Custom Select
        document.querySelectorAll('.u-select').forEach(sel => {
            const val = sel.querySelector('.value');
            const wrap = sel.querySelector('.sltwrap');
            const opts = sel.querySelectorAll('.option');
            
            sel.addEventListener('click', (e) => {
                document.querySelectorAll('.sltwrap').forEach(w => {
                    if(w !== wrap) w.style.display = 'none';
                });
                wrap.style.display = wrap.style.display === 'block' ? 'none' : 'block';
                e.stopPropagation();
            });
            
            opts.forEach(opt => {
                opt.addEventListener('click', (e) => {
                    e.stopPropagation();
                    val.textContent = opt.textContent;
                    sel.dataset.value = opt.dataset.value;
                    wrap.style.display = 'none';
                });
            });
        });

        document.addEventListener('click', () => {
            document.querySelectorAll('.sltwrap').forEach(w => w.style.display = 'none');
        });

        // Style Buttons Logic
        const slopeMap = { 'normal': 0, 'oblique': 1, 'italic': 2 };
        const slopeRev = { 0: 'normal', 1: 'oblique', 2: 'italic' };
        
        let selSlope = { basic: 0, extra: 0 };
        
        ['basic', 'extra'].forEach(type => {
            ['normal', 'oblique', 'italic'].forEach(slope => {
                const btn = document.querySelector(`.${type}-${slope}`);
                if(btn) {
                    btn.addEventListener('click', () => {
                        // Reset style for group
                        [`${type}-normal`, `${type}-oblique`, `${type}-italic`].forEach(c => {
                            document.querySelector(`.${c}`).style.backgroundColor = '';
                            document.querySelector(`.${c}`).style.color = '';
                        });
                        // Set active
                        btn.style.backgroundColor = '#0078D7';
                        btn.style.color = '#fff';
                        selSlope[type] = slopeMap[slope];
                    });
                }
            });
        });

        // Alignment Buttons Logic
        const alignMap = { 'left': 1, 'center': 2, 'right': 3 };
        const alignRev = { 1: 'left', 2: 'center', 3: 'right' };
        let selAlign = { basic: 1, extra: 1 };
        
        ['basic', 'extra'].forEach(type => {
            ['left', 'center', 'right'].forEach(align => {
                const btn = document.querySelector(`.${type}-align-${align}`);
                if(btn) {
                    btn.addEventListener('click', () => {
                        [`${type}-align-left`, `${type}-align-center`, `${type}-align-right`].forEach(c => {
                            document.querySelector(`.${c}`).style.backgroundColor = '';
                            document.querySelector(`.${c}`).style.color = '';
                        });
                        btn.style.backgroundColor = '#0078D7';
                        btn.style.color = '#fff';
                        selAlign[type] = alignMap[align];
                    });
                }
            });
        });

        // Load Config
        fetch('/api/config')
        .then(res => res.json())
        .then(data => {
            if(data) {
                // Font
                document.querySelector('.font-family').value = data.font.font_family;
                
                // Color
                document.querySelector('.basic-light-color').value = hexToColor(data.color.basic.light.hex_color);
                document.querySelector('.basic-light-opacity').value = data.color.basic.light.opacity;
                document.querySelector('.basic-dark-color').value = hexToColor(data.color.basic.dark.hex_color);
                document.querySelector('.basic-dark-opacity').value = data.color.basic.dark.opacity;
                document.querySelector('.extra-light-color').value = hexToColor(data.color.extra.light.hex_color);
                document.querySelector('.extra-light-opacity').value = data.color.extra.light.opacity;
                document.querySelector('.extra-dark-color').value = hexToColor(data.color.extra.dark.hex_color);
                document.querySelector('.extra-dark-opacity').value = data.color.extra.dark.opacity;
                
                // Size
                document.querySelector('.basic-size').value = data.size.basic;
                document.querySelector('.extra-size').value = data.size.extra;

                // Style
                document.querySelector('.basic-weight').dataset.value = data.style.basic.weight.value;
                document.querySelector('.basic-weight .value').textContent = data.style.basic.weight.textContent;
                
                // Slope
                const bs = slopeRev[data.style.basic.slope];
                if(bs) document.querySelector(`.basic-${bs}`).click();
                
                document.querySelector('.basic-underline').checked = data.style.basic.underline;
                document.querySelector('.basic-strikethrough').checked = data.style.basic.strikethrough;

                document.querySelector('.extra-weight').dataset.value = data.style.extra.weight.value;
                document.querySelector('.extra-weight .value').textContent = data.style.extra.weight.textContent;
                
                const es = slopeRev[data.style.extra.slope];
                if(es) document.querySelector(`.extra-${es}`).click();
                
                document.querySelector('.extra-underline').checked = data.style.extra.underline;
                document.querySelector('.extra-strikethrough').checked = data.style.extra.strikethrough;

                // Align
                const ba = alignRev[data.align.basic];
                if(ba) document.querySelector(`.basic-align-${ba}`).click();
                const ea = alignRev[data.align.extra];
                if(ea) document.querySelector(`.extra-align-${ea}`).click();

                // Position
                document.querySelector('.position-select').dataset.value = data.position.position.value;
                document.querySelector('.position-select .value').textContent = data.position.position.textContent;

                // Margin
                document.querySelector('.margin-left').value = data.margin.left;
                document.querySelector('.margin-right').value = data.margin.right;
                
                // Screen
                document.querySelector('.screen-select').dataset.value = data.screen.parent_taskbar.value;
                document.querySelector('.screen-select .value').textContent = data.screen.parent_taskbar.textContent;
                
                // Hitokoto
                if (data.hitokoto) {
                    document.querySelector('.hitokoto-json-path').value = data.hitokoto.hitokoto_json_path || "";
                    document.querySelector('.hitokoto-interval').value = data.hitokoto.hitokoto_interval || 30;
                }
            }
        });

        // Save Config
        document.querySelectorAll('.apply').forEach(btn => {
            btn.addEventListener('click', () => {
                const config = {
                    font: {
                        font_family: document.querySelector('.font-family').value
                    },
                color: {
                    basic: {
                        light: {
                            hex_color: colorToHex(document.querySelector('.basic-light-color').value),
                            opacity: parseFloat(document.querySelector('.basic-light-opacity').value)
                        },
                        dark: {
                            hex_color: colorToHex(document.querySelector('.basic-dark-color').value),
                            opacity: parseFloat(document.querySelector('.basic-dark-opacity').value)
                        }
                    },
                    extra: {
                        light: {
                            hex_color: colorToHex(document.querySelector('.extra-light-color').value),
                            opacity: parseFloat(document.querySelector('.extra-light-opacity').value)
                        },
                        dark: {
                            hex_color: colorToHex(document.querySelector('.extra-dark-color').value),
                            opacity: parseFloat(document.querySelector('.extra-dark-opacity').value)
                        }
                    }
                },
                size: {
                    basic: parseFloat(document.querySelector('.basic-size').value),
                    extra: parseFloat(document.querySelector('.extra-size').value)
                },
                style: {
                    basic: {
                        weight: {
                            value: parseInt(document.querySelector('.basic-weight').dataset.value || 400),
                            textContent: document.querySelector('.basic-weight .value').textContent
                        },
                        slope: selSlope.basic,
                        underline: document.querySelector('.basic-underline').checked,
                        strikethrough: document.querySelector('.basic-strikethrough').checked
                    },
                    extra: {
                        weight: {
                            value: parseInt(document.querySelector('.extra-weight').dataset.value || 400),
                            textContent: document.querySelector('.extra-weight .value').textContent
                        },
                        slope: selSlope.extra,
                        underline: document.querySelector('.extra-underline').checked,
                        strikethrough: document.querySelector('.extra-strikethrough').checked
                    }
                },
                align: {
                    basic: selAlign.basic,
                    extra: selAlign.extra
                },
                position: {
                    position: {
                        value: parseInt(document.querySelector('.position-select').dataset.value || 0),
                        textContent: document.querySelector('.position-select .value').textContent
                    }
                },
                margin: {
                    left: parseInt(document.querySelector('.margin-left').value),
                    right: parseInt(document.querySelector('.margin-right').value)
                },
                screen: {
                    parent_taskbar: {
                        value: document.querySelector('.screen-select').dataset.value,
                        textContent: document.querySelector('.screen-select .value').textContent
                    }
                }
            };

            fetch('/api/config', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(config)
            }).then(() => {
                alert('保存成功');
            });

            // Send Hitokoto Config
            const hitokotoConfig = {
                hitokoto_json_path: document.querySelector('.hitokoto-json-path').value,
                hitokoto_interval: parseInt(document.querySelector('.hitokoto-interval').value)
            };
            
            fetch('/taskbar/hitokoto', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(hitokotoConfig)
            });
        });
    });

        // Reset Config
        document.querySelectorAll('.reset').forEach(btn => {
            btn.addEventListener('click', () => {
                if(confirm('确定要恢复默认设置吗？')) {
                    fetch('/api/reset', { method: 'POST' })
                    .then(() => {
                        alert('已重置，请刷新页面');
                        location.reload();
                    });
                }
            });
        });
    )";
    fullHtml += "</script></body></html>";
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: " + std::to_string(fullHtml.length()) + "\r\n\r\n" + fullHtml;
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
    std::string response = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: " + std::to_string(json.length()) + "\r\n\r\n" + json;
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
        r->basicFontSizeDoubleLine = config.size.extra;

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

    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
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
        r->basicFontSizeDoubleLine = config.size.extra;

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
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}
