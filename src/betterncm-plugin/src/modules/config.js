"use strict";

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
    set: (name, value) => plugin.setConfig(name, value),

    /**
     * 从后端同步配置
     * @description 将后端返回的配置合并到本地，仅覆盖前后端共有的配置项
     * @param {Object} backendConfig - 后端返回的配置对象
     */
    syncFromBackend: (backendConfig) => {
        if (!backendConfig) return;
        const sharedKeys = ["font", "color", "size", "style", "lyrics", "effect", "position", "margin", "align", "screen"];
        for (const key of sharedKeys) {
            if (backendConfig[key] !== undefined) {
                plugin.setConfig(key, backendConfig[key]);
            }
        }
    }
};
