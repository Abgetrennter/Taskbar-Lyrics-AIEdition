"use strict";

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
