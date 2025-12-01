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
     * @description 复制并运行后端可执行文件
     */
    async start() {
        const dataPath = (await betterncm.app.getDataPath()).replace("/", "\\");
        
        let pluginPath = this.pluginPath;
        if (!pluginPath) {
            pluginPath = plugin.pluginPath;
        }
        
        // 规范化路径
        pluginPath = pluginPath.replace("/./", "\\").replace("/", "\\");

        console.log(`Taskbar Lyrics: Starting backend. DataPath: ${dataPath}, PluginPath: ${pluginPath}`);

        const taskkill = `taskkill /F /IM "任务栏歌词.exe"`;
        const xcopy = `xcopy /C /D /Y "${pluginPath}\\任务栏歌词.exe" "${dataPath}"`;
        const exec = `"${dataPath}\\任务栏歌词.exe" ${BETTERNCM_API_PORT - 2}`;
        const cmd = `${taskkill} & ${xcopy} & ${exec}`;

        try {
            await betterncm.app.exec(`cmd /S /C ${cmd}`, false, false);
            // 启动后应用配置
            this.applyConfig();
            lyricManager.start();
        } catch (e) {
            console.error("Taskbar Lyrics: Failed to start backend", e);
            throw e;
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
     * @description 发送关闭指令并停止歌词处理
     */
    async close() {
        apiInstance.close({});
        lyricManager.stop();
    }

    /**
     * 重启后端程序
     * @description 尝试重新启动后端程序
     * @returns {boolean} 重启是否成功
     */
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
