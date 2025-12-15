"use strict";

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
