"use strict";

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
