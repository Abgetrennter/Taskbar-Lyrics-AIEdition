"use strict";


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



const Utils = {
    flattenObject: (obj, prefix = '') => {
        return Object.keys(obj).reduce((acc, k) => {
            const pre = prefix.length ? prefix + '_' : '';
            if (typeof obj[k] === 'object' && obj[k] !== null && !Array.isArray(obj[k]))
                Object.assign(acc, Utils.flattenObject(obj[k], pre + k));
            else
                acc[pre + k] = obj[k];
            return acc;
        }, {});
    },
    
    debounce: (func, wait) => {
        let timeout;
        return function(...args) {
            const context = this;
            clearTimeout(timeout);
            timeout = setTimeout(() => func.apply(context, args), wait);
        };
    },

    delay: (ms) => {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};



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

const ConfigManager = {
    get: name => Object.assign({}, defaultConfig[name], plugin.getConfig(name, defaultConfig[name])),
    set: (name, value) => plugin.setConfig(name, value)
};



class TaskbarLyricsAPI {
    constructor() {
        this.socket = null;
        this.heartbeatInterval = null;
        this.reconnectTimeout = null;
        this.retryCount = 0;
        this.messageQueue = [];
        this.port = BETTERNCM_API_PORT - 2;
        this.callbacks = {
            onOpen: [],
            onClose: [],
            onError: []
        };
    }

    connect() {
        if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) {
            return;
        }

        this.socket = new WebSocket(`ws://127.0.0.1:${this.port}`);

        this.socket.onopen = () => {
            console.log("Taskbar Lyrics: WebSocket connected");
            this.retryCount = 0;
            if (this.reconnectTimeout) clearTimeout(this.reconnectTimeout);

            // Flush queue
            while (this.messageQueue.length > 0) {
                const msg = this.messageQueue.shift();
                this.socket.send(msg);
            }

            // Start heartbeat
            if (this.heartbeatInterval) clearInterval(this.heartbeatInterval);
            this.heartbeatInterval = setInterval(() => {
                if (this.socket && this.socket.readyState === WebSocket.OPEN) {
                    this.socket.send(JSON.stringify({ url: "/taskbar/heartbeat" }));
                }
            }, 5000);
            
            this.callbacks.onOpen.forEach(cb => cb());
        };

        this.socket.onclose = () => {
            console.log("Taskbar Lyrics: WebSocket closed, reconnecting in 3s...");
            if (this.heartbeatInterval) clearInterval(this.heartbeatInterval);

            this.retryCount++;
            
            this.callbacks.onClose.forEach(cb => cb(this.retryCount));

            this.reconnectTimeout = setTimeout(() => this.connect(), 3000);
        };

        this.socket.onerror = (err) => {
            console.error("Taskbar Lyrics: WebSocket error", err);
            this.socket.close();
            this.callbacks.onError.forEach(cb => cb(err));
        };
    }

    fetch(path, params) {
        const payload = Utils.flattenObject(params);
        payload.url = "/taskbar" + path;
        const msg = JSON.stringify(payload);

        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(msg);
        } else {
            // If it's a lyric update, remove previous lyric updates from queue to avoid buildup
            if (payload.url === "/taskbar/lyrics/lyrics") {
                this.messageQueue = this.messageQueue.filter(m => !m.includes('"/taskbar/lyrics/lyrics"'));
            }

            this.messageQueue.push(msg);

            if (this.messageQueue.length > 100) this.messageQueue.shift();

            this.connect();
        }
    }

    // API Methods
    font(params) { this.fetch("/font/font", params); }
    color(params) { this.fetch("/font/color", params); }
    style(params) { this.fetch("/font/style", params); }
    size(params) { this.fetch("/font/size", params); }
    lyrics(params) { this.fetch("/lyrics/lyrics", params); }
    align(params) { this.fetch("/lyrics/align", params); }
    windowPosition(params) { this.fetch("/window/position", params); }
    windowMargin(params) { this.fetch("/window/margin", params); }
    windowScreen(params) { this.fetch("/window/screen", params); }
    close(params) { this.fetch("/close", params); }

    on(event, callback) {
        if (this.callbacks[event]) {
            this.callbacks[event].push(callback);
        }
    }
}

const apiInstance = new TaskbarLyricsAPI();



class LyricManager {
    constructor() {
        this.observer = null;
        this.parsedLyric = null;
        this.currentIndex = 0;
        this.musicId = 0;
        this.currentLine = 0;
        this.liblyric = loadedPlugins.liblyric;
        
        this.boundPlayLoad = this.playLoad.bind(this);
        this.boundPlayProgress = this.playProgress.bind(this);
    }

    async watchLyricsChange() {
        const mLyric = await betterncm.utils.waitForElement("#x-g-mn .m-lyric");
        const MutationCallback = mutations => {
            for (const mutation of mutations) {
                let lyrics = {
                    basic: "",
                    extra: ""
                };

                if (mutation.addedNodes[2]) {
                    lyrics.basic = mutation.addedNodes[0].firstChild.textContent;
                    lyrics.extra = mutation.addedNodes[2].firstChild ? mutation.addedNodes[2].firstChild.textContent : "";
                } else {
                    lyrics.basic = mutation.addedNodes[0].textContent;
                }

                apiInstance.lyrics(lyrics);
            }
        }

        this.observer = new MutationObserver(MutationCallback);
        this.observer.observe(mLyric, { childList: true, subtree: true });
    }

    async playLoad() {
        const playingSong = betterncm.ncm.getPlayingSong();
        this.musicId = playingSong.data.id ?? 0;
        const name = playingSong.data.name ?? "";
        const artists = playingSong.data.artists ?? "";

        let artistName = "";
        if (Array.isArray(artists)) {
            artists.forEach(item => artistName += ` / ${item.name}`);
            artistName = artistName.slice(3);
        }

        apiInstance.lyrics({
            "basic": name,
            "extra": artistName
        });

        const config = ConfigManager.get("lyrics");
        if ((config["retrieval_method"]["value"] == "2") && window.currentLyrics) {
            // RefinedNowPlaying integration
            let retries = 0;
            while (retries < 50) { // Avoid infinite loop
                if (window.currentLyrics.hash.includes(this.musicId)) {
                    this.parsedLyric = window.currentLyrics.lyrics;
                    break;
                } else {
                    await Utils.delay(100);
                    retries++;
                }
            }
        } else {
            const lyricData = await this.liblyric.getLyricData(this.musicId);
            this.parsedLyric = this.liblyric.parseLyric(
                lyricData?.lrc?.lyric ?? "",
                lyricData?.tlyric?.lyric ?? "",
                lyricData?.romalrc?.lyric ?? ""
            );
        }

        if (this.parsedLyric) {
            this.parsedLyric = this.parsedLyric.filter(item => item.originalLyric != "");

            // Pure music check
            if (
                (this.parsedLyric.length == 1)
                && (this.parsedLyric[0].time == 0)
                && (this.parsedLyric[0].duration != 0)
            ) {
                this.parsedLyric = [];
            }
        }

        this.currentIndex = 0;
    }

    async playProgress(_, time) {
        const adjust = Number(ConfigManager.get("effect")["adjust"]);
        if (!this.parsedLyric) return;

        let nextIndex = this.parsedLyric.findIndex(item => item.time > (time + adjust) * 1000);
        nextIndex = (nextIndex <= -1) ? this.parsedLyric.length : nextIndex;

        if (nextIndex != this.currentIndex) {
            const currentLyric = this.parsedLyric[nextIndex - 1] ?? "";
            const nextLyric = this.parsedLyric[nextIndex] ?? "";

            const lyrics = {
                "basic": currentLyric?.originalLyric ?? "",
                "extra": currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? ""
            };

            this.processExtraShow(lyrics, currentLyric, nextLyric);

            apiInstance.lyrics(lyrics);
            this.currentIndex = nextIndex;
        }
    }

    processExtraShow(lyrics, currentLyric, nextLyric) {
        const extraShowValue = ConfigManager.get("effect")["extra_show"]["value"];
        
        switch (extraShowValue) {
            case 0: // No extra
                lyrics.extra = "";
                break;

            case 1: // Next line or swap
                const nextLinePos = ConfigManager.get("effect")["next_line_lyrics_position"]["value"];
                switch (nextLinePos) {
                    case 0:
                        lyrics.extra = nextLyric?.originalLyric ?? "";
                        break;
                    case 1:
                        lyrics.basic = nextLyric?.originalLyric ?? "";
                        lyrics.extra = currentLyric?.originalLyric ?? "";
                        break;
                    case 2:
                        if (this.currentLine == 0) {
                            lyrics.basic = currentLyric?.originalLyric ?? "";
                            lyrics.extra = nextLyric?.originalLyric ?? "";
                            this.currentLine = 1;
                        } else {
                            lyrics.basic = nextLyric?.originalLyric ?? "";
                            lyrics.extra = currentLyric?.originalLyric ?? "";
                            this.currentLine = 0;
                        }
                        break;
                }
                break;

            case 2: // Translation or next original
                lyrics.extra = currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? "";
                break;

            case 3: // Roman or Translation or next original
                lyrics.extra = currentLyric?.romanLyric 
                    ?? currentLyric?.translatedLyric 
                    ?? nextLyric?.originalLyric 
                    ?? "";
                break;
        }
    }

    start() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0:
                this.watchLyricsChange();
                break;
            case 1:
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                const playingSong = betterncm.ncm.getPlayingSong();
                if (playingSong && playingSong.data && playingSong.data.id != this.musicId) {
                    this.playLoad();
                }
                break;
            case 2:
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                break;
        }
    }

    stop() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0:
                if (this.observer) {
                    this.observer.disconnect();
                    this.observer = null;
                }
                break;
            case 1:
            case 2:
                legacyNativeCmder.removeRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.removeRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                break;
        }
    }
}

const lyricManager = new LyricManager();



class BackendManager {
    constructor() {
        this.pluginPath = "";
    }

    setPluginPath(path) {
        this.pluginPath = path;
    }

    async start() {
        const dataPath = (await betterncm.app.getDataPath()).replace("/", "\\");
        
        let pluginPath = this.pluginPath;
        if (!pluginPath) {
            pluginPath = plugin.pluginPath;
        }
        
        // Normalize path
        pluginPath = pluginPath.replace("/./", "\\").replace("/", "\\");

        console.log(`Taskbar Lyrics: Starting backend. DataPath: ${dataPath}, PluginPath: ${pluginPath}`);

        const taskkill = `taskkill /F /IM "任务栏歌词.exe"`;
        const xcopy = `xcopy /C /D /Y "${pluginPath}\\任务栏歌词.exe" "${dataPath}"`;
        const exec = `"${dataPath}\\任务栏歌词.exe" ${BETTERNCM_API_PORT - 2}`;
        const cmd = `${taskkill} & ${xcopy} & ${exec}`;

        try {
            await betterncm.app.exec(`cmd /S /C ${cmd}`, false, false);
            // Give it a moment to start before sending configs? 
            // The original code sends immediately. The socket connection logic handles the queue.
            this.applyConfig();
            lyricManager.start();
        } catch (e) {
            console.error("Taskbar Lyrics: Failed to start backend", e);
            throw e;
        }
    }

    applyConfig() {
        apiInstance.font(ConfigManager.get("font"));
        apiInstance.color(ConfigManager.get("color"));
        apiInstance.style(ConfigManager.get("style"));
        apiInstance.size(ConfigManager.get("size"));
        apiInstance.windowPosition(ConfigManager.get("position"));
        apiInstance.windowMargin(ConfigManager.get("margin"));
        apiInstance.align(ConfigManager.get("align"));
        apiInstance.windowScreen(ConfigManager.get("screen"));
    }

    async close() {
        apiInstance.close({});
        lyricManager.stop();
    }

    async restart() {
        console.log("Taskbar Lyrics: Triggering backend restart...");
        try {
            await this.start();
            console.log("Taskbar Lyrics: Backend restart command executed.");
            return true;
        } catch (e) {
            console.error("Taskbar Lyrics: Backend restart failed", e);
            return false;
        }
    }
}

const backendManager = new BackendManager();



class ConfigView {
    constructor() {
        this.root = document.createElement("div");
        this.root.style.overflow = "hidden";
        this.root.style.height = "100%";
        this.root.style.width = "100%";
        this.pluginPath = "";
    }

    setPluginPath(path) {
        this.pluginPath = path;
    }

    getElement() {
        return this.root;
    }

    async init() {
        // Load HTML
        const path = `${this.pluginPath}/assets/config.html`;
        const text = await betterncm.fs.readFileText(path);
        const parser = new DOMParser();
        const dom = parser.parseFromString(text, "text/html");
        const element = dom.querySelector("#taskbar-lyrics-dom");
        this.root.appendChild(element);

        // Load CSS
        const cssPath = `${this.pluginPath}/assets/style.css`;

        const cssText = await betterncm.fs.readFileText(cssPath);
        const style = document.createElement("style");
        style.textContent = cssText;
        this.root.appendChild(style);

        this.initTabs();
        this.initGlobalEvents();
        this.bindSettings();
    }

    initTabs() {
        const tabBox = this.root.querySelector(".tab_box");
        const contentBox = this.root.querySelector(".content_box");
        const tabs = tabBox.querySelectorAll(".tab_button");
        const contents = contentBox.querySelectorAll(".content");

        tabs.forEach((tab, index) => {
            tab.addEventListener("click", () => {
                tabBox.querySelector(".active").classList.remove("active");
                tab.classList.add("active");
                contentBox.querySelector(".show").classList.remove("show");
                contents[index].classList.add("show");
            });
        });
    }

    initGlobalEvents() {
        const selectController = (event) => {
            const parent = event.target.parentElement;
            parent.classList.toggle("z-open");
        };

        // Delegate event for value clicks if they are dynamically loaded
        // But here they are loaded once. However, using delegation on root is safer if we had dynamic content.
        // Since we appendChild 'element', we can select from root.
        
        // Note: We need to attach listeners to elements that exist now.
        // But init() is async. The caller (index.js) will await it.
    }

    bindSettings() {
        // Helper to select within root
        const $ = (sel) => this.root.querySelector(sel);
        const $$ = (sel) => this.root.querySelectorAll(sel);

        // Select Controller Logic Re-attach
        $$(".value").forEach(el => {
             el.addEventListener("click", (event) => {
                const parent = event.target.parentElement;
                if (parent.classList.contains("z-open")) parent.classList.remove("z-open");
                else parent.classList.add("z-open");
             });
        });

        // --- Font ---
        const fontSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("font")));
                config["font_family"] = $(".content.font .font-settings .font-family").value;
                ConfigManager.set("font", config);
                apiInstance.font(config);
            },
            reset: () => {
                ConfigManager.set("font", undefined);
                apiInstance.font(defaultConfig["font"]);
                $(".content.font .font-settings .font-family").value = defaultConfig["font"]["font_family"];
            }
        };
        $(".content.font .font-settings .apply").addEventListener("click", fontSettings.apply);
        $(".content.font .font-settings .reset").addEventListener("click", fontSettings.reset);
        $(".content.font .font-settings .font-family").value = ConfigManager.get("font")["font_family"];

        // --- Color ---
        const colorSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("color")));
                const getVal = (sel) => $(sel).value;
                config["basic"]["light"]["hex_color"] = parseInt(getVal(".basic-light-color").slice(1), 16);
                config["basic"]["light"]["opacity"] = Number(getVal(".basic-light-opacity"));
                config["basic"]["dark"]["hex_color"] = parseInt(getVal(".basic-dark-color").slice(1), 16);
                config["basic"]["dark"]["opacity"] = Number(getVal(".basic-dark-opacity"));
                config["extra"]["light"]["hex_color"] = parseInt(getVal(".extra-light-color").slice(1), 16);
                config["extra"]["light"]["opacity"] = Number(getVal(".extra-light-opacity"));
                config["extra"]["dark"]["hex_color"] = parseInt(getVal(".extra-dark-color").slice(1), 16);
                config["extra"]["dark"]["opacity"] = Number(getVal(".extra-dark-opacity"));
                ConfigManager.set("color", config);
                apiInstance.color(config);
            },
            reset: () => {
                const def = defaultConfig["color"];
                const setVal = (sel, val) => $(sel).value = val;
                const toHex = (num) => `#${num.toString(16).padStart(6, "0")}`;
                
                setVal(".basic-light-color", toHex(def.basic.light.hex_color));
                setVal(".basic-light-opacity", def.basic.light.opacity);
                setVal(".basic-dark-color", toHex(def.basic.dark.hex_color));
                setVal(".basic-dark-opacity", def.basic.dark.opacity);
                setVal(".extra-light-color", toHex(def.extra.light.hex_color));
                setVal(".extra-light-opacity", def.extra.light.opacity);
                setVal(".extra-dark-color", toHex(def.extra.dark.hex_color));
                setVal(".extra-dark-opacity", def.extra.dark.opacity);
                
                ConfigManager.set("color", undefined);
                apiInstance.color(def);
            }
        };
        $(".content.font .color-settings .apply").addEventListener("click", colorSettings.apply);
        $(".content.font .color-settings .reset").addEventListener("click", colorSettings.reset);
        // Init values
        {
            const cur = ConfigManager.get("color");
            const toHex = (num) => `#${num.toString(16).padStart(6, "0")}`; // Fix: padding
            // Wait, original code: `#${...toString(16)}`. If it's 0, it becomes `#0`. It should be `#000000`.
            // Original code: `#${pluginConfig.get("color")["basic"]["light"]["hex_color"].toString(16)}`
            // If the original code had a bug, I should probably fix it or keep it. 
            // User asked for optimization and quality, so I'll fix it to padStart(6, '0').
            
            // Wait, original code in reset uses `padStart(6, "0")`.
            // But in init logic (view.js line 137), it didn't use padStart.
            // I will add padStart.
            
            $(".basic-light-color").value = toHex(cur.basic.light.hex_color);
            $(".basic-light-opacity").value = cur.basic.light.opacity;
            $(".basic-dark-color").value = toHex(cur.basic.dark.hex_color);
            $(".basic-dark-opacity").value = cur.basic.dark.opacity;
            $(".extra-light-color").value = toHex(cur.extra.light.hex_color);
            $(".extra-light-opacity").value = cur.extra.light.opacity;
            $(".extra-dark-color").value = toHex(cur.extra.dark.hex_color);
            $(".extra-dark-opacity").value = cur.extra.dark.opacity;
        }

        // --- Size ---
        const sizeSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("size")));
                config["basic"] = Number($(".basic-size").value);
                config["extra"] = Number($(".extra-size").value);
                ConfigManager.set("size", config);
                apiInstance.size(config);
            },
            reset: () => {
                $(".basic-size").value = defaultConfig.size.basic;
                $(".extra-size").value = defaultConfig.size.extra;
                ConfigManager.set("size", undefined);
                apiInstance.size(defaultConfig.size);
            }
        };
        $(".content.font .size-settings .apply").addEventListener("click", sizeSettings.apply);
        $(".content.font .size-settings .reset").addEventListener("click", sizeSettings.reset);
        $(".basic-size").value = ConfigManager.get("size").basic;
        $(".extra-size").value = ConfigManager.get("size").extra;

        // --- Style ---
        const styleSettings = {
            setWeight: (name, value, textContent) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name].weight.value = Number(value);
                config[name].weight.textContent = textContent;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            setSlope: (name, type) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name].slope = type;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            setToggle: (name, prop, checked) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name][prop] = checked;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            reset: () => {
                ConfigManager.set("style", undefined);
                apiInstance.style(defaultConfig.style);
                // Update UI
                const def = defaultConfig.style;
                $(".content.font .style-settings .basic-weight .value").textContent = def.basic.weight.textContent;
                $(".basic-underline").checked = def.basic.underline;
                $(".basic-strikethrough").checked = def.basic.strikethrough;
                $(".content.font .style-settings .extra-weight .value").textContent = def.extra.weight.textContent;
                $(".extra-underline").checked = def.extra.underline;
                $(".extra-strikethrough").checked = def.extra.strikethrough;
            }
        };
        
        $(".content.font .style-settings .reset").addEventListener("click", styleSettings.reset);
        
        // Helper for style bindings
        const bindStyleSlope = (sel, name, val) => $(sel).addEventListener("click", () => styleSettings.setSlope(name, val));
        const bindStyleToggle = (sel, name, prop) => $(sel).addEventListener("change", (e) => styleSettings.setToggle(name, prop, e.target.checked));
        
        bindStyleSlope(".basic-normal", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL);
        bindStyleSlope(".basic-oblique", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE);
        bindStyleSlope(".basic-italic", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC);
        bindStyleToggle(".basic-underline", "basic", "underline");
        bindStyleToggle(".basic-strikethrough", "basic", "strikethrough");

        bindStyleSlope(".extra-normal", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL);
        bindStyleSlope(".extra-oblique", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE);
        bindStyleSlope(".extra-italic", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC);
        bindStyleToggle(".extra-underline", "extra", "underline");
        bindStyleToggle(".extra-strikethrough", "extra", "strikethrough");

        const bindWeightSelect = (name) => {
            $(`.content.font .style-settings .${name}-weight .select`).addEventListener("click", e => {
                const val = e.target.dataset.value;
                const text = e.target.textContent;
                styleSettings.setWeight(name, val, text);
                $(`.content.font .style-settings .${name}-weight .value`).textContent = text;
            });
        };
        bindWeightSelect("basic");
        bindWeightSelect("extra");

        // Init Style UI
        const curStyle = ConfigManager.get("style");
        $(".content.font .style-settings .basic-weight .value").textContent = curStyle.basic.weight.textContent;
        $(".basic-underline").checked = curStyle.basic.underline;
        $(".basic-strikethrough").checked = curStyle.basic.strikethrough;
        $(".content.font .style-settings .extra-weight .value").textContent = curStyle.extra.weight.textContent;
        $(".extra-underline").checked = curStyle.extra.underline;
        $(".extra-strikethrough").checked = curStyle.extra.strikethrough;


        // --- Lyrics ---
        const lyricsSettings = {
            reset: () => {
                $(".retrieval-method .value").textContent = defaultConfig.lyrics.retrieval_method.textContent;
                lyricManager.stop();
                ConfigManager.set("lyrics", undefined);
                lyricManager.start();
            }
        };
        $(".content.lyrics .lyrics-settings .reset").addEventListener("click", lyricsSettings.reset);
        $(".lyrics-switch").addEventListener("change", e => e.target.checked ? backendManager.start() : backendManager.close());
        
        $(".retrieval-method .select").addEventListener("click", e => {
            const value = e.target.dataset.value;
            const text = e.target.textContent;
            if (value == "2" && !window.currentLyrics) {
                channel.call("trayicon.popBalloon", () => {}, [{
                    title: "任务栏歌词",
                    text: "无法使用RefinedNowPlaying歌词！\n是否安装RefinedNowPlaying插件？\n将回退到使用LibLyric解析获取歌词",
                    icon: "path",
                    hasSound: true,
                    delayTime: 2e3
                }]);
                return;
            }
            const config = JSON.parse(JSON.stringify(ConfigManager.get("lyrics")));
            config["retrieval_method"]["value"] = Number(value);
            config["retrieval_method"]["textContent"] = text;
            lyricManager.stop();
            ConfigManager.set("lyrics", config);
            lyricManager.start();
            $(".retrieval-method .value").textContent = text;
        });
        $(".retrieval-method .value").textContent = ConfigManager.get("lyrics").retrieval_method.textContent;

        // --- Effect ---
        const effectSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("effect")));
                config["adjust"] = Number($(".adjust").value);
                ConfigManager.set("effect", config);
            },
            reset: () => {
                $(".next-line-lyrics-position .value").textContent = defaultConfig.effect.next_line_lyrics_position.textContent;
                $(".extra-show .value").textContent = defaultConfig.effect.extra_show.textContent;
                $(".adjust").value = defaultConfig.effect.adjust;
                ConfigManager.set("effect", undefined);
            }
        };
        $(".content.lyrics .effect-settings .apply").addEventListener("click", effectSettings.apply);
        $(".content.lyrics .effect-settings .reset").addEventListener("click", effectSettings.reset);

        const bindEffectSelect = (cls, key) => {
             $(`${cls} .select`).addEventListener("click", e => {
                 const value = e.target.dataset.value;
                 const text = e.target.textContent;
                 const config = JSON.parse(JSON.stringify(ConfigManager.get("effect")));
                 config[key]["value"] = Number(value);
                 config[key]["textContent"] = text;
                 ConfigManager.set("effect", config);
                 $(`${cls} .value`).textContent = text;
             });
        };
        bindEffectSelect(".next-line-lyrics-position", "next_line_lyrics_position");
        bindEffectSelect(".extra-show", "extra_show");
        
        const curEffect = ConfigManager.get("effect");
        $(".next-line-lyrics-position .value").textContent = curEffect.next_line_lyrics_position.textContent;
        $(".extra-show .value").textContent = curEffect.extra_show.textContent;
        $(".adjust").value = curEffect.adjust;


        // --- Align ---
        const alignSettings = {
             set: (type, val) => {
                 const config = JSON.parse(JSON.stringify(ConfigManager.get("align")));
                 config[type] = val;
                 ConfigManager.set("align", config);
                 apiInstance.align(config);
             },
             reset: () => {
                 ConfigManager.set("align", undefined);
                 apiInstance.align(defaultConfig.align);
             }
        };
        $(".content.lyrics .align-settings .reset").addEventListener("click", alignSettings.reset);
        
        const bindAlign = (sel, type, val) => $(sel).addEventListener("click", () => alignSettings.set(type, val));
        bindAlign(".basic-left", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING);
        bindAlign(".basic-center", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_CENTER);
        bindAlign(".basic-right", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_TRAILING);
        bindAlign(".extra-left", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING);
        bindAlign(".extra-center", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_CENTER);
        bindAlign(".extra-right", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_TRAILING);

        // --- Window Position ---
        const posSettings = {
            reset: () => {
                $(".window-position .value").textContent = defaultConfig.position.position.textContent;
                ConfigManager.set("position", undefined);
                apiInstance.windowPosition(defaultConfig.position);
            }
        };
        $(".content.window .position-settings .reset").addEventListener("click", posSettings.reset);
        $(".window-position .select").addEventListener("click", e => {
            const value = e.target.dataset.value;
            const text = e.target.textContent;
            const config = JSON.parse(JSON.stringify(ConfigManager.get("position")));
            config["position"]["value"] = Number(value);
            config["position"]["textContent"] = text;
            ConfigManager.set("position", config);
            apiInstance.windowPosition(config);
            $(".window-position .value").textContent = text;
        });
        $(".window-position .value").textContent = ConfigManager.get("position").position.textContent;

        // --- Margin ---
        const marginSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("margin")));
                config["left"] = Number($(".margin-settings .left").value);
                config["right"] = Number($(".margin-settings .right").value);
                ConfigManager.set("margin", config);
                apiInstance.windowMargin(config);
            },
            reset: () => {
                ConfigManager.set("margin", undefined);
                apiInstance.windowMargin(defaultConfig.margin);
                $(".margin-settings .left").value = defaultConfig.margin.left;
                $(".margin-settings .right").value = defaultConfig.margin.right;
            }
        };
        $(".content.window .margin-settings .apply").addEventListener("click", marginSettings.apply);
        $(".content.window .margin-settings .reset").addEventListener("click", marginSettings.reset);
        $(".margin-settings .left").value = ConfigManager.get("margin").left;
        $(".margin-settings .right").value = ConfigManager.get("margin").right;

        // --- Screen ---
        const screenSettings = {
            reset: () => {
                $(".parent-taskbar .value").textContent = defaultConfig.screen.parent_taskbar.textContent;
                ConfigManager.set("screen", undefined);
                apiInstance.windowScreen(defaultConfig.screen);
            }
        };
        $(".content.window .screen-settings .reset").addEventListener("click", screenSettings.reset);
        $(".parent-taskbar .select").addEventListener("click", e => {
             const value = e.target.dataset.value;
             const text = e.target.textContent;
             const config = JSON.parse(JSON.stringify(ConfigManager.get("screen")));
             config["parent_taskbar"]["value"] = value;
             config["parent_taskbar"]["textContent"] = text;
             ConfigManager.set("screen", config);
             apiInstance.windowScreen(config);
             $(".parent-taskbar .value").textContent = text;
        });
        $(".parent-taskbar .value").textContent = ConfigManager.get("screen").parent_taskbar.textContent;
    }
}

const configView = new ConfigView();



plugin.onConfig(tools => configView.getElement());

plugin.onLoad(async () => {
    // Initialize Backend Manager with plugin path
    backendManager.setPluginPath(plugin.pluginPath);
    
    // Initialize View
    configView.setPluginPath(plugin.pluginPath);
    await configView.init();

    // Setup Restart Logic
    apiInstance.on('onClose', async (retryCount) => {
        if (retryCount === 5) {
             if (typeof channel !== 'undefined' && channel.call) {
                channel.call(
                    "trayicon.popBalloon",
                    () => { },
                    [{
                        title: "任务栏歌词",
                        text: "无法连接到任务栏歌词后端程序。\n尝试自动重启...",
                        icon: "path",
                        hasSound: false,
                        delayTime: 3000
                    }]
                );
            }
            
            const success = await backendManager.restart();

            if (typeof channel !== 'undefined' && channel.call) {
                channel.call(
                    "trayicon.popBalloon",
                    () => { },
                    [{
                        title: "任务栏歌词",
                        text: "自动重启" + (success ? "成功" : "失败"),
                        icon: "path",
                        hasSound: false,
                        delayTime: 3000
                    }]
                );
            }
        }
    });

    // Close on unload
    addEventListener("beforeunload", async () => {
        await backendManager.close();
    });

    // Start Backend
    await backendManager.start();
});


