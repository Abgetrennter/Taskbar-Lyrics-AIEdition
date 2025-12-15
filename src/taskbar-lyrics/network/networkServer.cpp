#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include "networkServer.hpp"
#include "../utils/jsonUtils.hpp"
#include "../utils/logger.hpp"
#include "../utils/configManager.hpp"
#include <sstream>
#include <fstream>
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
    std::string font = JsonUtils::getString(body, "font_family");
    m_window->renderer->fontFamily = utf8ToWide(font);
    
    ConfigManager::getInstance().GetConfigMutable().font.fontFamily = font;
    ConfigManager::getInstance().Save();

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
    
    auto& c = ConfigManager::getInstance().GetConfigMutable();
    c.color.basic.light.hexColor = JsonUtils::getHex(body, "basic_light_hex_color");
    c.color.basic.light.opacity = JsonUtils::getFloat(body, "basic_light_opacity");
    c.color.basic.dark.hexColor = JsonUtils::getHex(body, "basic_dark_hex_color");
    c.color.basic.dark.opacity = JsonUtils::getFloat(body, "basic_dark_opacity");
    c.color.extra.light.hexColor = JsonUtils::getHex(body, "extra_light_hex_color");
    c.color.extra.light.opacity = JsonUtils::getFloat(body, "extra_light_opacity");
    c.color.extra.dark.hexColor = JsonUtils::getHex(body, "extra_dark_hex_color");
    c.color.extra.dark.opacity = JsonUtils::getFloat(body, "extra_dark_opacity");
    ConfigManager::getInstance().Save();

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleStyle(const std::string& body) {
    int basicWeight = JsonUtils::getInt(body, "basic_weight_value");
    int basicSlope = JsonUtils::getInt(body, "basic_slope");
    bool basicUnderline = JsonUtils::getBool(body, "basic_underline");
    bool basicStrikethrough = JsonUtils::getBool(body, "basic_strikethrough");
    
    int extraWeight = JsonUtils::getInt(body, "extra_weight_value");
    int extraSlope = JsonUtils::getInt(body, "extra_slope");
    bool extraUnderline = JsonUtils::getBool(body, "extra_underline");
    bool extraStrikethrough = JsonUtils::getBool(body, "extra_strikethrough");

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
}

void NetworkServer::handleSize(const std::string& body) {
    float basic = JsonUtils::getFloat(body, "basic");
    float extra = JsonUtils::getFloat(body, "extra");

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
}

void NetworkServer::handleLyrics(const std::string& body) {
    std::wstring basic = utf8ToWide(JsonUtils::getString(body, "basic"));
    std::wstring extra = utf8ToWide(JsonUtils::getString(body, "extra"));
    
    m_window->renderer->basicLyrics = basic;
    m_window->renderer->extraLyrics = extra;
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleAlign(const std::string& body) {
    int basicAlign = JsonUtils::getInt(body, "basic");
    int extraAlign = JsonUtils::getInt(body, "extra");

    m_window->renderer->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)basicAlign;
    m_window->renderer->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)extraAlign;
    
    auto& c = ConfigManager::getInstance().GetConfigMutable();
    c.align.basic = basicAlign;
    c.align.extra = extraAlign;
    ConfigManager::getInstance().Save();

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handlePosition(const std::string& body) {
    int value = JsonUtils::getInt(body, "value");
    std::string text = JsonUtils::getString(body, "textContent");
    
    m_window->renderer->windowAlignment = (WindowAlignment)value;
    
    auto& c = ConfigManager::getInstance().GetConfigMutable();
    c.position.value = value;
    c.position.textContent = text;
    ConfigManager::getInstance().Save();

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleMargin(const std::string& body) {
    int left = JsonUtils::getInt(body, "left");
    int right = JsonUtils::getInt(body, "right");

    m_window->renderer->leftMargin = left;
    m_window->renderer->rightMargin = right;
    
    auto& c = ConfigManager::getInstance().GetConfigMutable();
    c.margin.left = left;
    c.margin.right = right;
    ConfigManager::getInstance().Save();

    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleScreen(const std::string& body) {
    std::string value = JsonUtils::getString(body, "value");
    std::string text = JsonUtils::getString(body, "textContent");
    
    m_window->renderer->updateParentTaskbar(value);
    
    auto& c = ConfigManager::getInstance().GetConfigMutable();
    c.screen.parentTaskbarValue = value;
    c.screen.parentTaskbarText = text;
    ConfigManager::getInstance().Save();
    
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleClose(const std::string& body) {
    m_window->renderer->basicLyrics = L"";
    m_window->renderer->extraLyrics = L"";
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
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

        // Align Buttons Logic
        const alignMap = { 'left': 0, 'center': 2, 'right': 1 }; // DWRITE: Leading=0, Trailing=1, Center=2
        const alignRev = { 0: 'left', 1: 'right', 2: 'center' };

        let selAlign = { basic: 0, extra: 0 };

        ['basic', 'extra'].forEach(type => {
            ['left', 'center', 'right'].forEach(align => {
                const btn = document.querySelector(`.${type}-${align}`);
                if(btn) {
                    btn.addEventListener('click', () => {
                        [`${type}-left`, `${type}-center`, `${type}-right`].forEach(c => {
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

        function fetchConfig() {
            fetch('/api/config').then(res => res.json()).then(config => {
                if(config.font) document.querySelector('.font-family').value = config.font.font_family || '';
                
                if(config.color) {
                    if(config.color.basic) {
                        document.querySelector('.basic-light-color').value = hexToColor(config.color.basic.light.hex_color);
                        document.querySelector('.basic-light-opacity').value = config.color.basic.light.opacity || 1;
                        document.querySelector('.basic-dark-color').value = hexToColor(config.color.basic.dark.hex_color);
                        document.querySelector('.basic-dark-opacity').value = config.color.basic.dark.opacity || 1;
                    }
                    if(config.color.extra) {
                        document.querySelector('.extra-light-color').value = hexToColor(config.color.extra.light.hex_color);
                        document.querySelector('.extra-light-opacity').value = config.color.extra.light.opacity || 1;
                        document.querySelector('.extra-dark-color').value = hexToColor(config.color.extra.dark.hex_color);
                        document.querySelector('.extra-dark-opacity').value = config.color.extra.dark.opacity || 1;
                    }
                }

                if(config.size) {
                    document.querySelector('.basic-size').value = config.size.basic || 20;
                    document.querySelector('.extra-size').value = config.size.extra || 15;
                }

                if(config.margin) {
                    document.querySelector('.left').value = config.margin.left || 0;
                    document.querySelector('.right').value = config.margin.right || 0;
                }

                if(config.style) {
                    ['basic', 'extra'].forEach(type => {
                        if(config.style[type]) {
                            // Weight
                            const wBtn = document.querySelector(`.${type}-weight`);
                            if(wBtn && config.style[type].weight) {
                                wBtn.dataset.value = config.style[type].weight.value;
                                wBtn.querySelector('.value').textContent = config.style[type].weight.textContent;
                            }
                            // Slope
                            const slopeVal = config.style[type].slope || 0;
                            selSlope[type] = slopeVal;
                            const slopeName = slopeRev[slopeVal] || 'normal';
                            const btn = document.querySelector(`.${type}-${slopeName}`);
                            if(btn) {
                                btn.style.backgroundColor = '#0078D7';
                                btn.style.color = '#fff';
                            }
                            // Underline/Strike
                            const u = document.querySelector(`.${type}-underline`);
                            if(u) u.checked = config.style[type].underline;
                            const s = document.querySelector(`.${type}-strikethrough`);
                            if(s) s.checked = config.style[type].strikethrough;
                        }
                    });
                }

                if(config.align) {
                    ['basic', 'extra'].forEach(type => {
                        const val = config.align[type] || 0;
                        selAlign[type] = val;
                        const name = alignRev[val] || 'left';
                        const btn = document.querySelector(`.${type}-${name}`);
                        if(btn) {
                            btn.style.backgroundColor = '#0078D7';
                            btn.style.color = '#fff';
                        }
                    });
                }

                // Position
                if(config.position) {
                    const posSelect = document.querySelector('.position-select');
                    if(posSelect) {
                        posSelect.dataset.value = config.position.value;
                        posSelect.querySelector('.value').textContent = config.position.textContent;
                    }
                }

                // Screen
                if(config.screen) {
                    const screenSelect = document.querySelector('.screen-select');
                    if(screenSelect) {
                        screenSelect.dataset.value = config.screen.parentTaskbarValue;
                        screenSelect.querySelector('.value').textContent = config.screen.parentTaskbarText;
                    }
                }
            });
        }

        function saveConfig() {
            const config = {
                font: { font_family: document.querySelector('.font-family').value },
                size: {
                    basic: parseFloat(document.querySelector('.basic-size').value) || 0,
                    extra: parseFloat(document.querySelector('.extra-size').value) || 0
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
                margin: {
                    left: parseInt(document.querySelector('.left').value) || 0,
                    right: parseInt(document.querySelector('.right').value) || 0
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
                    value: parseInt(document.querySelector('.position-select').dataset.value || 0),
                    textContent: document.querySelector('.position-select .value').textContent
                },
                screen: {
                    parentTaskbarValue: document.querySelector('.screen-select').dataset.value || '',
                    parentTaskbarText: document.querySelector('.screen-select .value').textContent
                }
            };
            
            fetch('/api/config', {
                method: 'POST',
                body: JSON.stringify(config)
            }).then(() => {
                // Flash feedback or similar
            });
        }

        // Apply buttons
        document.querySelectorAll('.apply').forEach(btn => btn.addEventListener('click', saveConfig));
        
        // Auto-save triggers
        // Align & Style buttons trigger save? Maybe better to stick to manual apply where present, but Align/Style don't have Apply buttons.
        // So we should auto-save them.
        
        // Wrap save for listeners
        const autoSave = () => saveConfig();

        // Style/Align/Margin listeners
        document.querySelectorAll('.switch-btn').forEach(b => b.addEventListener('change', autoSave));
        // Margin inputs
        document.querySelector('.left').addEventListener('change', autoSave);
        document.querySelector('.right').addEventListener('change', autoSave);
        
        // Inject auto-save into button clicks for style/align
        const wrapBtnClick = (selector) => {
            const btn = document.querySelector(selector);
            if(btn) {
                const old = btn.onclick; 
                btn.addEventListener('click', () => setTimeout(autoSave, 50));
            }
        };
        // Actually I already added listeners above for UI update, I can just add autoSave() call there.
        // But the loops above are clean. Let's just add global click listener for those buttons?
        // Or re-iterate.
        
        ['basic', 'extra'].forEach(type => {
            ['normal', 'oblique', 'italic'].forEach(s => {
                const b = document.querySelector(`.${type}-${s}`);
                if(b) b.addEventListener('click', autoSave);
            });
            ['left', 'center', 'right'].forEach(a => {
                const b = document.querySelector(`.${type}-${a}`);
                if(b) b.addEventListener('click', autoSave);
            });
            // Weight select options
            document.querySelectorAll(`.${type}-weight .option`).forEach(opt => {
                opt.addEventListener('click', () => setTimeout(autoSave, 50));
            });
        });

        function resetConfig() {
            if(confirm('Are you sure you want to reset all settings to default?')) {
                fetch('/api/reset').then(() => {
                    location.reload();
                });
            }
        }

        // Reset buttons
        document.querySelectorAll('.reset').forEach(btn => btn.addEventListener('click', resetConfig));

        // Position & Screen listeners
        document.querySelectorAll('.position-select .option').forEach(opt => {
            opt.addEventListener('click', () => setTimeout(autoSave, 50));
        });
        document.querySelectorAll('.screen-select .option').forEach(opt => {
            opt.addEventListener('click', () => setTimeout(autoSave, 50));
        });

        fetchConfig();
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
        std::string res = "HTTP/1.1 404 Not Found\r\n\r\nStyle file not found.";
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

void NetworkServer::applyConfigToRenderer() {
    const AppConfig& config = ConfigManager::getInstance().GetConfig();
    
    // Font
    m_window->renderer->fontFamily = utf8ToWide(config.font.fontFamily);
    
    // Size
    if (config.size.basic > 0) m_window->renderer->basicFontSize = config.size.basic;
    if (config.size.extra > 0) {
        m_window->renderer->extraFontSize = config.size.extra;
        m_window->renderer->basicFontSizeDoubleLine = config.size.extra;
    }
    
    // Color
    m_window->renderer->basicLightColor = D2D1::ColorF(config.color.basic.light.hexColor, config.color.basic.light.opacity);
    m_window->renderer->basicDarkColor = D2D1::ColorF(config.color.basic.dark.hexColor, config.color.basic.dark.opacity);
    m_window->renderer->extraLightColor = D2D1::ColorF(config.color.extra.light.hexColor, config.color.extra.light.opacity);
    m_window->renderer->extraDarkColor = D2D1::ColorF(config.color.extra.dark.hexColor, config.color.extra.dark.opacity);

    // Style
    m_window->renderer->basicFontWeight = (DWRITE_FONT_WEIGHT)config.style.basic.weightValue;
    m_window->renderer->basicFontStyle = (DWRITE_FONT_STYLE)config.style.basic.slope;
    m_window->renderer->basicUnderline = config.style.basic.underline;
    m_window->renderer->basicStrikethrough = config.style.basic.strikethrough;

    m_window->renderer->extraFontWeight = (DWRITE_FONT_WEIGHT)config.style.extra.weightValue;
    m_window->renderer->extraFontStyle = (DWRITE_FONT_STYLE)config.style.extra.slope;
    m_window->renderer->extraUnderline = config.style.extra.underline;
    m_window->renderer->extraStrikethrough = config.style.extra.strikethrough;

    // Align
    m_window->renderer->basicTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.basic;
    m_window->renderer->extraTextAlign = (DWRITE_TEXT_ALIGNMENT)config.align.extra;

    // Margin
    m_window->renderer->leftMargin = config.margin.left;
    m_window->renderer->rightMargin = config.margin.right;

    // Position
    m_window->renderer->windowAlignment = (WindowAlignment)config.position.value;

    // Screen
    m_window->renderer->updateParentTaskbar(config.screen.parentTaskbarValue);
    
    PostMessage(m_window->windowHandle, WM_PAINT, NULL, NULL);
}

void NetworkServer::handleUpdateConfig(SOCKET clientSocket, const std::string& body) {
    ConfigManager::getInstance().UpdateFromJSON(body);
    ConfigManager::getInstance().Save();
    
    applyConfigToRenderer();
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}

void NetworkServer::handleResetConfig(SOCKET clientSocket) {
    ConfigManager::getInstance().Reset();
    applyConfigToRenderer();
    
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    send(clientSocket, response.c_str(), (int)response.length(), 0);
    closesocket(clientSocket);
}
