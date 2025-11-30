"use strict";


plugin.onLoad(async () => {
    const TaskbarLyricsPort = BETTERNCM_API_PORT - 2;

    const flattenObject = (obj, prefix = '') => {
        return Object.keys(obj).reduce((acc, k) => {
            const pre = prefix.length ? prefix + '_' : '';
            if (typeof obj[k] === 'object' && obj[k] !== null && !Array.isArray(obj[k]))
                Object.assign(acc, flattenObject(obj[k], pre + k));
            else
                acc[pre + k] = obj[k];
            return acc;
        }, {});
    }

    let socket = null;
    let heartbeatInterval = null;
    let reconnectTimeout = null;
    let retryCount = 0;
    let messageQueue = [];

    const connectWebSocket = () => {
        if (socket && (socket.readyState === WebSocket.OPEN || socket.readyState === WebSocket.CONNECTING)) {
            return;
        }

        socket = new WebSocket(`ws://127.0.0.1:${TaskbarLyricsPort}`);

        socket.onopen = () => {
            console.log("Taskbar Lyrics: WebSocket connected");
            retryCount = 0;
            if (reconnectTimeout) clearTimeout(reconnectTimeout);
            
            // Flush queue
            while (messageQueue.length > 0) {
                const msg = messageQueue.shift();
                socket.send(msg);
            }

            // Start heartbeat
            if (heartbeatInterval) clearInterval(heartbeatInterval);
            heartbeatInterval = setInterval(() => {
                if (socket && socket.readyState === WebSocket.OPEN) {
                    socket.send(JSON.stringify({ url: "/taskbar/heartbeat" }));
                }
            }, 5000);
        };

        socket.onclose = () => {
            console.log("Taskbar Lyrics: WebSocket closed, reconnecting in 3s...");
            if (heartbeatInterval) clearInterval(heartbeatInterval);
            
            retryCount++;
            if (retryCount === 5) {
                // Notify user if backend seems down
                if (typeof channel !== 'undefined' && channel.call) {
                    channel.call(
                        "trayicon.popBalloon",
                        () => { },
                        [{
                            title: "任务栏歌词",
                            text: "无法连接到任务栏歌词后端程序。\n请检查程序是否运行。",
                            icon: "path",
                            hasSound: false,
                            delayTime: 3000
                        }]
                    );
                }
            }

            reconnectTimeout = setTimeout(connectWebSocket, 3000);
        };

        socket.onerror = (err) => {
            console.error("Taskbar Lyrics: WebSocket error", err);
            socket.close();
        };
    };

    // Initial connection
    connectWebSocket();

    const TaskbarLyricsFetch = (path, params) => {
        const payload = flattenObject(params);
        payload.url = "/taskbar" + path;
        const msg = JSON.stringify(payload);
        
        if (socket && socket.readyState === WebSocket.OPEN) {
            socket.send(msg);
        } else {
            // If it's a lyric update, remove previous lyric updates from queue to avoid buildup
            if (payload.url === "/taskbar/lyrics/lyrics") {
                messageQueue = messageQueue.filter(m => !m.includes('"/taskbar/lyrics/lyrics"'));
            }
            
            messageQueue.push(msg);
            
            // Cap queue size just in case
            if (messageQueue.length > 100) messageQueue.shift();

            // If not connected, try to reconnect (throttled by connectWebSocket logic)
            connectWebSocket();
        }
    };

    const TaskbarLyricsAPI = {
        // 字体设置
        font: {
            font: params => TaskbarLyricsFetch("/font/font", params),
            color: params => TaskbarLyricsFetch("/font/color", params),
            style: params => TaskbarLyricsFetch("/font/style", params),
            size: params => TaskbarLyricsFetch("/font/size", params),
        },

        // 歌词设置
        lyrics: {
            lyrics: params => TaskbarLyricsFetch("/lyrics/lyrics", params),
            align: params => TaskbarLyricsFetch("/lyrics/align", params),
        },

        // 窗口设置
        window: {
            position: params => TaskbarLyricsFetch("/window/position", params),
            margin: params => TaskbarLyricsFetch("/window/margin", params),
            screen: params => TaskbarLyricsFetch("/window/screen", params),
        },

        // 关闭
        close: params => TaskbarLyricsFetch("/close", params)
    };


    // 对应Windows的枚举
    const WindowsEnum = {
        WindowAlignment: {
            WindowAlignmentAdaptive: 0,
            WindowAlignmentLeft: 1,
            WindowAlignmentCenter: 2,
            WindowAlignmentRight: 3
        },
        DWRITE_TEXT_ALIGNMENT: {
            DWRITE_TEXT_ALIGNMENT_LEADING: 0,
            DWRITE_TEXT_ALIGNMENT_TRAILING: 1,
            DWRITE_TEXT_ALIGNMENT_CENTER: 2,
            DWRITE_TEXT_ALIGNMENT_JUSTIFIED: 3
        },
        DWRITE_FONT_WEIGHT: {
            DWRITE_FONT_WEIGHT_THIN: 100,
            DWRITE_FONT_WEIGHT_EXTRA_LIGHT: 200,
            DWRITE_FONT_WEIGHT_ULTRA_LIGHT: 200,
            DWRITE_FONT_WEIGHT_LIGHT: 300,
            DWRITE_FONT_WEIGHT_SEMI_LIGHT: 350,
            DWRITE_FONT_WEIGHT_NORMAL: 400,
            DWRITE_FONT_WEIGHT_REGULAR: 400,
            DWRITE_FONT_WEIGHT_MEDIUM: 500,
            DWRITE_FONT_WEIGHT_DEMI_BOLD: 600,
            DWRITE_FONT_WEIGHT_SEMI_BOLD: 600,
            DWRITE_FONT_WEIGHT_BOLD: 700,
            DWRITE_FONT_WEIGHT_EXTRA_BOLD: 800,
            DWRITE_FONT_WEIGHT_ULTRA_BOLD: 800,
            DWRITE_FONT_WEIGHT_BLACK: 900,
            DWRITE_FONT_WEIGHT_HEAVY: 900,
            DWRITE_FONT_WEIGHT_EXTRA_BLACK: 950,
            DWRITE_FONT_WEIGHT_ULTRA_BLACK: 950
        },
        DWRITE_FONT_STYLE: {
            DWRITE_FONT_STYLE_NORMAL: 0,
            DWRITE_FONT_STYLE_OBLIQUE: 1,
            DWRITE_FONT_STYLE_ITALIC: 2
        }
    };


    // 默认的配置
    const defaultConfig = {
        "font": {
            "font_family": "Microsoft YaHei UI"
        },
        "size": {
            "basic": 20,
            "extra": 15
        },
        "color": {
            "basic": {
                "light": {
                    "hex_color": 0x000000,
                    "opacity": 1.0
                },
                "dark": {
                    "hex_color": 0xFFFFFF,
                    "opacity": 1.0
                }
            },
            "extra": {
                "light": {
                    "hex_color": 0x000000,
                    "opacity": 1.0
                },
                "dark": {
                    "hex_color": 0xFFFFFF,
                    "opacity": 1.0
                }
            }
        },
        "style": {
            "basic": {
                "weight": {
                    "value": WindowsEnum.DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_NORMAL,
                    "textContent": "Normal (400)"
                },
                "slope": WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL,
                "underline": false,
                "strikethrough": false
            },
            "extra": {
                "weight": {
                    "value": WindowsEnum.DWRITE_FONT_WEIGHT.DWRITE_FONT_WEIGHT_NORMAL,
                    "textContent": "Normal (400)"
                },
                "slope": WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL,
                "underline": false,
                "strikethrough": false
            }
        },
        "lyrics": {
            "retrieval_method": {
                "value": 1,
                "textContent": "使用LibLyric解析获取歌词",
            }
        },
        "effect": {
            "next_line_lyrics_position": {
                "value": 0,
                "textContent": "副歌词，下句歌词显示在这"
            },
            "extra_show": {
                "value": 2,
                "textContent": "当前翻译，没则用上个选项"
            },
            "adjust": 0.0
        },
        "align": {
            "basic": WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING,
            "extra": WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING
        },
        "position": {
            "position": {
                "value": WindowsEnum.WindowAlignment.WindowAlignmentAdaptive,
                "textContent": "自动，自适应选择左或右"
            }
        },
        "margin": {
            "left": 0,
            "right": 0
        },
        "screen": {
            "parent_taskbar": {
                "value": "Shell_TrayWnd",
                "textContent": "主屏幕任务栏"
            }
        }
    };


    const pluginConfig = {
        get: name => Object.assign({}, defaultConfig[name], plugin.getConfig(name, defaultConfig[name])),
        set: (name, value) => plugin.setConfig(name, value)
    };


    this.base = {
        TaskbarLyricsPort,
        TaskbarLyricsAPI,
        WindowsEnum,
        defaultConfig,
        pluginConfig
    };
});
