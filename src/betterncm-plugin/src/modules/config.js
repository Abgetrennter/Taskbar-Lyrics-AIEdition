"use strict";

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
