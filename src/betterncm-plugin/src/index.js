"use strict";

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
