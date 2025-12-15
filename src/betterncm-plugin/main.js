"use strict";

/**
 * @module 常量定义
 * @description 定义插件使用的常量和枚举值，包括对齐方式、字体权重、字体样式等
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

const WindowsEnum = {
    // 窗口对齐方式
    WindowAlignment: {
        WindowAlignmentAdaptive: 0, // 自适应
        WindowAlignmentLeft: 1,     // 左对齐
        WindowAlignmentCenter: 2,   // 居中
        WindowAlignmentRight: 3     // 右对齐
    },
    // DirectWrite 文本对齐方式
    DWRITE_TEXT_ALIGNMENT: {
        DWRITE_TEXT_ALIGNMENT_LEADING: 0,   // 左对齐
        DWRITE_TEXT_ALIGNMENT_TRAILING: 1,  // 右对齐
        DWRITE_TEXT_ALIGNMENT_CENTER: 2,    // 居中
        DWRITE_TEXT_ALIGNMENT_JUSTIFIED: 3  // 两端对齐
    },
    // DirectWrite 字体粗细
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
    // DirectWrite 字体样式
    DWRITE_FONT_STYLE: {
        DWRITE_FONT_STYLE_NORMAL: 0,    // 正常
        DWRITE_FONT_STYLE_OBLIQUE: 1,   // 倾斜
        DWRITE_FONT_STYLE_ITALIC: 2     // 斜体
    }
};


/**
 * @module 工具函数
 * @description 提供通用的工具函数，如对象扁平化、防抖、延时等
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

const Utils = {
    /**
     * 扁平化对象
     * @description 将嵌套的对象转换为单层对象，键名使用前缀拼接
     * @param {Object} obj - 需要扁平化的对象
     * @param {string} [prefix=''] - 键名前缀
     * @returns {Object} 扁平化后的对象
     */
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
    
    /**
     * 防抖函数
     * @description 限制函数在一定时间内只能执行一次
     * @param {Function} func - 需要执行的函数
     * @param {number} wait - 等待时间（毫秒）
     * @returns {Function} 包装后的函数
     */
    debounce: (func, wait) => {
        let timeout;
        return function(...args) {
            const context = this;
            clearTimeout(timeout);
            timeout = setTimeout(() => func.apply(context, args), wait);
        };
    },

    /**
     * 延时函数
     * @description 返回一个Promise，在指定时间后resolve
     * @param {number} ms - 延时时间（毫秒）
     * @returns {Promise} Promise对象
     */
    delay: (ms) => {
        return new Promise(resolve => setTimeout(resolve, ms));
    }
};


/**
 * @module 配置管理
 * @description 定义插件的默认配置，并提供配置的读取和保存功能
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

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
        },
        "karaoke": false
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
    /**
     * 获取配置
     * @description 获取指定名称的配置，如果不存在则返回默认配置
     * @param {string} name - 配置名称
     * @returns {Object} 配置对象
     */
    get: name => Object.assign({}, defaultConfig[name], plugin.getConfig(name, defaultConfig[name])),
    
    /**
     * 保存配置
     * @description 保存指定名称的配置
     * @param {string} name - 配置名称
     * @param {Object} value - 配置值
     */
    set: (name, value) => plugin.setConfig(name, value)
};


/**
 * @module API 通信模块
 * @description 封装与任务栏歌词程序的 HTTP 通信逻辑
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-15
 */

class TaskbarLyricsAPI {
    constructor() {
        // 使用固定端口，实现独立部署
        this.port = 27232; 
        this.callbacks = {
            onError: [],
            onOnline: [],
            onOffline: []
        };
        this.isBackendOnline = false;
        this.checkInterval = null;
        this.startHealthCheck();
    }

    startHealthCheck() {
        const check = async () => {
            try {
                // 使用 OPTIONS 请求作为轻量级心跳/健康检查
                const response = await fetch(`http://127.0.0.1:${this.port}/taskbar/lyrics/lyrics`, {
                    method: 'OPTIONS'
                });
                if (response.ok) {
                    if (!this.isBackendOnline) {
                        console.log("Taskbar Lyrics: Backend is online");
                        this.callbacks.onOnline.forEach(cb => cb());
                    }
                    this.isBackendOnline = true;
                } else {
                    if (this.isBackendOnline) {
                        console.log("Taskbar Lyrics: Backend is offline (Response not OK)");
                        this.callbacks.onOffline.forEach(cb => cb());
                    }
                    this.isBackendOnline = false;
                }
            } catch (e) {
                // console.error("Taskbar Lyrics: Health check failed", e);
                if (this.isBackendOnline) {
                    console.log("Taskbar Lyrics: Backend is offline (Error)", e);
                    this.callbacks.onOffline.forEach(cb => cb());
                }
                this.isBackendOnline = false;
            }
        };

        check(); // 立即检查一次
        this.checkInterval = setInterval(check, 2000); // 每2秒检查一次
    }

    /**
     * 发送请求
     * @description 发送数据到任务栏歌词程序
     * @param {string} path - API 路径
     * @param {Object} params - 请求参数
     */
    async fetch(path, params) {
        if (!this.isBackendOnline) {
            // 如果后端离线，直接忽略请求，避免大量报错
            return;
        }

        const payload = params;
        // HTTP 模式下，URL 通过请求行传递，body 中不需要 url 字段，但保留也不会出错
        
        try {
            const response = await fetch(`http://127.0.0.1:${this.port}/taskbar${path}`, {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify(payload)
            });
            
            if (!response.ok) {
                throw new Error(`HTTP error! status: ${response.status}`);
            }
        } catch (e) {
            // 如果请求失败，可能后端刚刚离线
            this.isBackendOnline = false;
            this.callbacks.onError.forEach(cb => cb(e));
        }
    }

    // API Methods
    
    /**
     * 设置字体
     * @param {Object} params - 字体配置
     */
    font(params) { this.fetch("/font/font", params); }

    /**
     * 设置颜色
     * @param {Object} params - 颜色配置
     */
    color(params) { this.fetch("/font/color", params); }

    /**
     * 设置样式
     * @param {Object} params - 样式配置
     */
    style(params) { this.fetch("/font/style", params); }

    /**
     * 设置大小
     * @param {Object} params - 大小配置
     */
    size(params) { this.fetch("/font/size", params); }

    /**
     * 发送歌词
     * @param {Object} params - 歌词数据
     */
    lyrics(params) { this.fetch("/lyrics/lyrics", params); }

    /**
     * 设置对齐
     * @param {Object} params - 对齐配置
     */
    align(params) { this.fetch("/lyrics/align", params); }

    /**
     * 设置窗口位置
     * @param {Object} params - 位置配置
     */
    windowPosition(params) { this.fetch("/window/position", params); }

    /**
     * 设置窗口边距
     * @param {Object} params - 边距配置
     */
    windowMargin(params) { this.fetch("/window/margin", params); }

    /**
     * 设置显示屏幕
     * @param {Object} params - 屏幕配置
     */
    windowScreen(params) { this.fetch("/window/screen", params); }

    /**
     * 关闭程序
     * @param {Object} params - 参数
     */
    close(params) { this.fetch("/close", params); }

    /**
     * 监听事件
     * @description 注册事件回调
     * @param {string} event - 事件名称 (onError)
     * @param {Function} callback - 回调函数
     */
    on(event, callback) {
        if (this.callbacks[event]) {
            this.callbacks[event].push(callback);
        }
    }
}

const apiInstance = new TaskbarLyricsAPI();


/**
 * @module 歌词管理
 * @description 负责歌词的获取、解析、处理和发送
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

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
        this.boundOnLyricsUpdate = this.onLyricsUpdate.bind(this);
    }

    /**
     * 监听内置歌词变化
     * @description 使用 MutationObserver 监听网易云音乐内置歌词DOM的变化
     */
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

    /**
     * 歌曲加载处理
     * @description 获取当前播放歌曲信息，初始化歌词
     */
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
        this.currentLyricsText = { "basic": name, "extra": artistName };

        const config = ConfigManager.get("lyrics");
        if (config["retrieval_method"]["value"] == "2") {
            // 集成 RefinedNowPlaying 插件
            if (window.currentLyrics && window.currentLyrics.hash.includes(this.musicId)) {
                this.parsedLyric = window.currentLyrics.lyrics;
            } else {
                this.parsedLyric = null;
            }
        } else {
            // 使用 LibLyric 获取歌词
            const lyricData = await this.liblyric.getLyricData(this.musicId);
            this.parsedLyric = this.liblyric.parseLyric(
                lyricData?.lrc?.lyric ?? "",
                lyricData?.tlyric?.lyric ?? "",
                lyricData?.romalrc?.lyric ?? "",
                lyricData?.yrc?.lyric ?? ""
            );

            if (lyricData?.yrc?.lyric) {
                const yrcLines = this.parseYrc(lyricData.yrc.lyric);
                if (yrcLines.length > 0) {
                    for (const line of this.parsedLyric) {
                        const yrcLine = yrcLines.find(l => Math.abs(l.time - line.time) < 100);
                        if (yrcLine) {
                            line.dynamicLyric = yrcLine.dynamicLyric;
                            if (yrcLine.duration > 0) line.duration = yrcLine.duration;
                        }
                    }
                }
            }
        }

        this.processParsedLyric();
        this.currentIndex = 0;
    }

    onLyricsUpdate(e) {
        const lyrics = e.detail;
        if (lyrics && lyrics.hash && lyrics.hash.includes(this.musicId)) {
            this.parsedLyric = lyrics.lyrics;
            this.processParsedLyric();
            this.currentIndex = 0;
        }
    }

    processParsedLyric() {
        if (this.parsedLyric) {
            this.parsedLyric = this.parsedLyric.filter(item => item.originalLyric != "");

            // 纯音乐检查：如果只有一行且时间为0且有持续时间，视为纯音乐
            if (
                (this.parsedLyric.length == 1)
                && (this.parsedLyric[0].time == 0)
                && (this.parsedLyric[0].duration != 0)
            ) {
                this.parsedLyric = [];
            }
        }
    }

    parseYrc(lyric) {
        const result = [];
        const yrcLineRegexp = /^\[(?<time>[0-9]+),(?<duration>[0-9]+)\](?<line>.*)/;
        const yrcWordTimeRegexp = /^\((?<time>[0-9]+),(?<duration>[0-9]+),(?<flag>[0-9]+)\)(?<word>[^\(]*)/;

        for (const line of lyric.trim().split("\n")) {
            let tmp = line.trim();
            const lineMatches = tmp.match(yrcLineRegexp);
            if (lineMatches) {
                const time = parseInt(lineMatches.groups?.time || "0");
                const duration = parseInt(lineMatches.groups?.duration || "0");
                tmp = lineMatches.groups?.line || "";
                const words = [];
                
                while (tmp.length > 0) {
                    const wordMatches = tmp.match(yrcWordTimeRegexp);
                    if (wordMatches) {
                        const wordTime = parseInt(wordMatches.groups?.time || "0");
                        const wordDuration = parseInt(wordMatches.groups?.duration || "0");
                        const flag = parseInt(wordMatches.groups?.flag || "0");
                        const word = wordMatches.groups?.word.trimStart();
                        
                        const splitedWords = word
                            ?.split(/\s+/)
                            .filter((v) => v.trim().length > 0);
                        
                        if (splitedWords && splitedWords.length > 0) {
                            const splitedDuration = wordDuration / splitedWords.length;
                            splitedWords.forEach((subWord, i) => {
                                let finalWord = subWord;
                                if (i === splitedWords.length - 1) {
                                     if (/\s/.test((word ?? '')[(word ?? '').length - 1])) {
                                         finalWord = `${subWord.trimStart()} `;
                                     } else {
                                         finalWord = subWord.trimStart();
                                     }
                                } else if (i === 0) {
                                     if (/\s/.test((word ?? '')[0])) {
                                          finalWord = ` ${subWord.trimStart()}`;
                                     } else {
                                          finalWord = subWord.trimStart();
                                     }
                                } else {
                                     finalWord = `${subWord.trimStart()} `;
                                }

                                words.push({
                                    time: wordTime + i * splitedDuration,
                                    duration: splitedDuration,
                                    flag,
                                    word: finalWord
                                });
                            });
                        }
                        
                        tmp = tmp.slice((wordMatches.index || 0) + wordMatches[0].length);
                    } else {
                        break;
                    }
                }
                result.push({
                    time,
                    duration,
                    dynamicLyric: words
                });
            }
        }
        return result;
    }

    /**
     * 播放进度更新
     * @description 根据当前播放时间更新歌词显示
     * @param {Object} _ - 事件对象（未使用）
     * @param {number} time - 当前播放时间（秒）
     */
    async playProgress(_, time) {
        const adjust = Number(ConfigManager.get("effect")["adjust"]);
        if (!this.parsedLyric) return;

        const currentTime = (time + adjust) * 1000;
        let nextIndex = this.parsedLyric.findIndex(item => item.time > currentTime);
        nextIndex = (nextIndex <= -1) ? this.parsedLyric.length : nextIndex;

        const config = ConfigManager.get("lyrics");
        const isLineChanged = nextIndex != this.currentIndex;

        if (isLineChanged) {
            const currentLyric = this.parsedLyric[nextIndex - 1] ?? "";
            const nextLyric = this.parsedLyric[nextIndex] ?? "";

            const lyrics = {
                "basic": currentLyric?.originalLyric ?? "",
                "extra": currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? ""
            };

            this.processExtraShow(lyrics, currentLyric, nextLyric);
            this.currentLyricsText = lyrics;
            this.currentIndex = nextIndex;
        }

        if (config.karaoke) {
            if (!this.currentLyricsText) return;

            const currentLineIdx = this.currentIndex - 1;
            const currentLyric = this.parsedLyric[currentLineIdx];
            
            let basicProgress = 0;
            if (currentLyric) {
                const startTime = currentLyric.time;
                let duration = currentLyric.duration;
                
                // Fallback duration calculation if missing or 0
                if (!duration || duration <= 0) {
                     const nextLine = this.parsedLyric[currentLineIdx + 1];
                     if (nextLine) {
                         duration = nextLine.time - startTime;
                     }
                }

                if (duration > 0) {
                    if (currentLyric.dynamicLyric) {
                        let passedLen = 0;
                        let totalLen = 0;
                        let currentWordProgress = 0;

                        for (const word of currentLyric.dynamicLyric) {
                             totalLen += word.word.length;
                             const wordEndTime = word.time + word.duration;
                             
                             if (currentTime >= wordEndTime) {
                                 passedLen += word.word.length;
                             } else if (currentTime >= word.time) {
                                 const wp = (currentTime - word.time) / word.duration;
                                 currentWordProgress = Math.max(0, Math.min(1, wp)) * word.word.length;
                             }
                        }
                        
                        if (totalLen > 0) {
                            basicProgress = (passedLen + currentWordProgress) / totalLen;
                        } else {
                             const p = (currentTime - startTime) / duration;
                             basicProgress = Math.max(0, Math.min(1, p));
                        }
                    } else {
                        const p = (currentTime - startTime) / duration;
                        basicProgress = Math.max(0, Math.min(1, p));
                    }
                } else {
                    basicProgress = (currentTime >= startTime) ? 1 : 0;
                }
            }

            let extraProgress = basicProgress;
            const extraShowValue = ConfigManager.get("effect")["extra_show"]["value"];
            
            if (extraShowValue == 1) { // Next Line or Swap
                const nextLinePos = ConfigManager.get("effect")["next_line_lyrics_position"]["value"];
                if (nextLinePos == 0) { // Extra = Next
                    extraProgress = -1;
                } else if (nextLinePos == 1) { // Basic = Next, Extra = Current
                    extraProgress = basicProgress;
                    basicProgress = -1;
                } else if (nextLinePos == 2) { // Rotate
                    if (this.currentLine == 1) {
                        // Basic=Curr, Extra=Next
                        extraProgress = -1;
                    } else {
                        // Basic=Next, Extra=Curr
                        extraProgress = basicProgress;
                        basicProgress = -1;
                    }
                }
            } else if (extraShowValue == 0) {
                extraProgress = -1;
            } else {
                // For Case 2 (Translation) & 3 (Romaji)
                // If extra lyric is effectively "Next Line", disable karaoke for it (-1)
                // If extra lyric is Translation/Romaji of current line, keep karaoke (basicProgress)
                
                const extraText = this.currentLyricsText.extra;
                const isTranslation = currentLyric?.translatedLyric && extraText === currentLyric.translatedLyric;
                const isRomaji = currentLyric?.romanLyric && extraText === currentLyric.romanLyric;
                
                if (isTranslation || isRomaji) {
                    extraProgress = basicProgress;
                } else {
                    // Likely fallback to next line or empty
                    extraProgress = -1;
                }
            }

            apiInstance.lyrics({
                ...this.currentLyricsText,
                basic_progress: basicProgress,
                extra_progress: extraProgress
            });
        } else if (isLineChanged) {
            apiInstance.lyrics({
                ...this.currentLyricsText,
                basic_progress: -1,
                extra_progress: -1
            });
        }
    }

    /**
     * 处理额外歌词显示
     * @description 根据配置决定额外歌词（副歌词）显示的内容
     * @param {Object} lyrics - 歌词对象
     * @param {Object} currentLyric - 当前歌词行
     * @param {Object} nextLyric - 下一行歌词
     */
    processExtraShow(lyrics, currentLyric, nextLyric) {
        const extraShowValue = ConfigManager.get("effect")["extra_show"]["value"];
        
        switch (extraShowValue) {
            case 0: // 不显示副歌词
                lyrics.extra = "";
                break;

            case 1: // 下一句歌词或交换显示
                const nextLinePos = ConfigManager.get("effect")["next_line_lyrics_position"]["value"];
                switch (nextLinePos) {
                    case 0: // 副歌词显示下一句
                        lyrics.extra = nextLyric?.originalLyric ?? "";
                        break;
                    case 1: // 主歌词显示下一句，副歌词显示当前句
                        lyrics.basic = nextLyric?.originalLyric ?? "";
                        lyrics.extra = currentLyric?.originalLyric ?? "";
                        break;
                    case 2: // 轮流显示
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

            case 2: // 翻译或下一句原始内容
                lyrics.extra = currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? "";
                break;

            case 3: // 罗马音或翻译或下一句原始内容
                lyrics.extra = currentLyric?.romanLyric 
                    ?? currentLyric?.translatedLyric 
                    ?? nextLyric?.originalLyric 
                    ?? "";
                break;
        }
    }

    /**
     * 启动歌词处理
     * @description 根据配置的获取方式启动相应的歌词处理逻辑
     */
    start() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0: // 监听内置歌词
                this.watchLyricsChange();
                break;
            case 1: // LibLyric
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                const playingSong = betterncm.ncm.getPlayingSong();
                if (playingSong && playingSong.data && playingSong.data.id != this.musicId) {
                    this.playLoad();
                }
                break;
            case 2: // RefinedNowPlaying
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                document.addEventListener('lyrics-updated', this.boundOnLyricsUpdate);
                break;
        }
    }

    /**
     * 停止歌词处理
     * @description 清理监听器和回调
     */
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
                if (method == 2) {
                    document.removeEventListener('lyrics-updated', this.boundOnLyricsUpdate);
                }
                break;
        }
    }
}

const lyricManager = new LyricManager();


/**
 * @module 后端管理
 * @description 负责后端程序的启动、关闭和重启，以及配置的初始应用
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

class BackendManager {
    constructor() {
        this.pluginPath = "";
    }

    /**
     * 设置插件路径
     * @param {string} path - 插件根目录路径
     */
    setPluginPath(path) {
        this.pluginPath = path;
    }

    /**
     * 启动后端程序
     * @description (已废弃) 后端现独立运行，不再由插件自动启动
     */
    async start() {
        console.log("Taskbar Lyrics: Backend start logic removed. Expecting independent backend.");
        
        // 监听后端上线事件，一旦连接成功立即同步配置
        apiInstance.on('onOnline', () => {
            console.log("Taskbar Lyrics: Backend online, applying config...");
            this.applyConfig();
        });

        // 如果当前已经在线（极少情况，因为 start 在初始化时调用），直接应用
        if (apiInstance.isBackendOnline) {
            this.applyConfig();
        }
    }

    /**
     * 应用所有配置
     * @description 将当前所有配置发送给后端程序
     */
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

    /**
     * 关闭后端程序
     * @description 发送关闭指令
     */
    async close() {
        apiInstance.close({});
    }

    /**
     * 重启后端程序
     * @description (已废弃)
     * @returns {boolean} 总是返回 false
     */
    async restart() {
        console.log("Taskbar Lyrics: Backend restart logic removed.");
        return false;
    }
}

const backendManager = new BackendManager();


/**
 * @module 视图管理
 * @description 负责插件配置界面的渲染、事件绑定和设置更新
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

class ConfigView {
    constructor() {
        this.root = document.createElement("div");
        this.root.style.overflow = "hidden";
        this.root.style.height = "100%";
        this.root.style.width = "100%";
        this.pluginPath = "";
    }

    /**
     * 设置插件路径
     * @param {string} path - 插件根目录路径
     */
    setPluginPath(path) {
        this.pluginPath = path;
    }

    /**
     * 获取根元素
     * @returns {HTMLElement} 插件配置界面的根元素
     */
    getElement() {
        return this.root;
    }

    /**
     * 初始化视图
     * @description 加载 HTML 和 CSS，初始化 Tab 切换和全局事件，绑定设置项
     */
    async init() {
        // 加载 HTML 模板
        const path = `${this.pluginPath}/assets/config.html`;
        const text = await betterncm.fs.readFileText(path);
        const parser = new DOMParser();
        const dom = parser.parseFromString(text, "text/html");
        const element = dom.querySelector("#taskbar-lyrics-dom");
        this.root.appendChild(element);

        // 加载 CSS 样式
        const cssPath = `${this.pluginPath}/assets/style.css`;

        const cssText = await betterncm.fs.readFileText(cssPath);
        const style = document.createElement("style");
        style.textContent = cssText;
        this.root.appendChild(style);

        this.initTabs();
        this.initGlobalEvents();
        this.bindSettings();
    }

    /**
     * 初始化 Tab 切换逻辑
     * @description 绑定 Tab 按钮点击事件，切换显示内容
     */
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

    /**
     * 初始化全局事件
     * @description 处理全局性的交互事件
     */
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

    /**
     * 绑定设置项
     * @description 为各个设置项绑定事件监听，处理配置的读取、修改和重置
     */
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
                $(".karaoke-switch").checked = defaultConfig.lyrics.karaoke;
                lyricManager.stop();
                ConfigManager.set("lyrics", undefined);
                lyricManager.start();
            }
        };
        $(".content.lyrics .lyrics-settings .reset").addEventListener("click", lyricsSettings.reset);
        $(".lyrics-switch").addEventListener("change", e => e.target.checked ? backendManager.start() : backendManager.close());
        
        $(".karaoke-switch").addEventListener("change", (e) => {
             const config = ConfigManager.get("lyrics");
             config["karaoke"] = e.target.checked;
             ConfigManager.set("lyrics", config);
        });
        $(".karaoke-switch").checked = ConfigManager.get("lyrics")["karaoke"];

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


/**
 * @module 插件入口
 * @description 插件的入口文件，负责初始化各个模块，处理生命周期事件，集成后端管理和视图逻辑
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

plugin.onConfig(tools => configView.getElement());

plugin.onLoad(async () => {
    // 初始化后端管理器
    backendManager.setPluginPath(plugin.pluginPath);
    
    // 初始化视图
    configView.setPluginPath(plugin.pluginPath);
    await configView.init();

    // 设置重启逻辑
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

    // 插件卸载时关闭后端
    addEventListener("beforeunload", async () => {
        await backendManager.close();
    });

    // 启动后端
    await backendManager.start();

    // 启动歌词获取
    lyricManager.start();
});


