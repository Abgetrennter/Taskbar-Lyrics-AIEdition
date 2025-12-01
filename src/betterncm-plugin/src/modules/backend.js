"use strict";

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
