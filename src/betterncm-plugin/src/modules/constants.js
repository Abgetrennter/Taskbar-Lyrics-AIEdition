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
