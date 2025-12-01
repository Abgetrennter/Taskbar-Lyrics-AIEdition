"use strict";

/**
 * @module API 通信模块
 * @description 封装与任务栏歌词程序的 WebSocket 通信逻辑，包括自动重连、心跳保活和消息队列
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

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

    /**
     * 建立 WebSocket 连接
     * @description 初始化 WebSocket 连接，设置事件监听，处理断线重连
     */
    connect() {
        if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) {
            return;
        }

        this.socket = new WebSocket(`ws://127.0.0.1:${this.port}`);

        this.socket.onopen = () => {
            console.log("Taskbar Lyrics: WebSocket connected");
            this.retryCount = 0;
            if (this.reconnectTimeout) clearTimeout(this.reconnectTimeout);

            // 发送队列中的消息
            while (this.messageQueue.length > 0) {
                const msg = this.messageQueue.shift();
                this.socket.send(msg);
            }

            // 启动心跳检测
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

            // 3秒后尝试重连
            this.reconnectTimeout = setTimeout(() => this.connect(), 3000);
        };

        this.socket.onerror = (err) => {
            console.error("Taskbar Lyrics: WebSocket error", err);
            this.socket.close();
            this.callbacks.onError.forEach(cb => cb(err));
        };
    }

    /**
     * 发送请求
     * @description 发送数据到任务栏歌词程序，如果未连接则加入队列
     * @param {string} path - API 路径
     * @param {Object} params - 请求参数
     */
    fetch(path, params) {
        const payload = Utils.flattenObject(params);
        payload.url = "/taskbar" + path;
        const msg = JSON.stringify(payload);

        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(msg);
        } else {
            // 如果是歌词更新，移除旧的歌词更新消息，避免队列堆积
            if (payload.url === "/taskbar/lyrics/lyrics") {
                this.messageQueue = this.messageQueue.filter(m => !m.includes('"/taskbar/lyrics/lyrics"'));
            }

            this.messageQueue.push(msg);

            // 限制队列长度
            if (this.messageQueue.length > 100) this.messageQueue.shift();

            this.connect();
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
     * @param {string} event - 事件名称 (onOpen, onClose, onError)
     * @param {Function} callback - 回调函数
     */
    on(event, callback) {
        if (this.callbacks[event]) {
            this.callbacks[event].push(callback);
        }
    }
}

const apiInstance = new TaskbarLyricsAPI();
