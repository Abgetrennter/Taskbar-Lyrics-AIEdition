"use strict";

/**
 * @module 歌词管理
 * @description 负责歌词的获取、解析、处理和发送
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

class LyricManager {
    constructor() {
        this.observer = null;
        this.parsedLyric = null;
        this.currentIndex = 0;
        this.musicId = 0;
        this.currentLine = 0;
        this.liblyric = loadedPlugins.liblyric;
        
        this.boundPlayLoad = this.playLoad.bind(this);
        this.boundPlayProgress = this.playProgress.bind(this);
    }

    /**
     * 监听内置歌词变化
     * @description 使用 MutationObserver 监听网易云音乐内置歌词DOM的变化
     */
    async watchLyricsChange() {
        const mLyric = await betterncm.utils.waitForElement("#x-g-mn .m-lyric");
        const MutationCallback = mutations => {
            for (const mutation of mutations) {
                let lyrics = {
                    basic: "",
                    extra: ""
                };

                if (mutation.addedNodes[2]) {
                    lyrics.basic = mutation.addedNodes[0].firstChild.textContent;
                    lyrics.extra = mutation.addedNodes[2].firstChild ? mutation.addedNodes[2].firstChild.textContent : "";
                } else {
                    lyrics.basic = mutation.addedNodes[0].textContent;
                }

                apiInstance.lyrics(lyrics);
            }
        }

        this.observer = new MutationObserver(MutationCallback);
        this.observer.observe(mLyric, { childList: true, subtree: true });
    }

    /**
     * 歌曲加载处理
     * @description 获取当前播放歌曲信息，初始化歌词
     */
    async playLoad() {
        const playingSong = betterncm.ncm.getPlayingSong();
        this.musicId = playingSong.data.id ?? 0;
        const name = playingSong.data.name ?? "";
        const artists = playingSong.data.artists ?? "";

        let artistName = "";
        if (Array.isArray(artists)) {
            artists.forEach(item => artistName += ` / ${item.name}`);
            artistName = artistName.slice(3);
        }

        apiInstance.lyrics({
            "basic": name,
            "extra": artistName
        });

        const config = ConfigManager.get("lyrics");
        if ((config["retrieval_method"]["value"] == "2") && window.currentLyrics) {
            // 集成 RefinedNowPlaying 插件
            let retries = 0;
            while (retries < 50) { // 防止无限循环
                if (window.currentLyrics.hash.includes(this.musicId)) {
                    this.parsedLyric = window.currentLyrics.lyrics;
                    break;
                } else {
                    await Utils.delay(100);
                    retries++;
                }
            }
        } else {
            // 使用 LibLyric 获取歌词
            const lyricData = await this.liblyric.getLyricData(this.musicId);
            this.parsedLyric = this.liblyric.parseLyric(
                lyricData?.lrc?.lyric ?? "",
                lyricData?.tlyric?.lyric ?? "",
                lyricData?.romalrc?.lyric ?? ""
            );
        }

        if (this.parsedLyric) {
            this.parsedLyric = this.parsedLyric.filter(item => item.originalLyric != "");

            // 纯音乐检查：如果只有一行且时间为0且有持续时间，视为纯音乐
            if (
                (this.parsedLyric.length == 1)
                && (this.parsedLyric[0].time == 0)
                && (this.parsedLyric[0].duration != 0)
            ) {
                this.parsedLyric = [];
            }
        }

        this.currentIndex = 0;
    }

    /**
     * 播放进度更新
     * @description 根据当前播放时间更新歌词显示
     * @param {Object} _ - 事件对象（未使用）
     * @param {number} time - 当前播放时间（秒）
     */
    async playProgress(_, time) {
        const adjust = Number(ConfigManager.get("effect")["adjust"]);
        if (!this.parsedLyric) return;

        let nextIndex = this.parsedLyric.findIndex(item => item.time > (time + adjust) * 1000);
        nextIndex = (nextIndex <= -1) ? this.parsedLyric.length : nextIndex;

        if (nextIndex != this.currentIndex) {
            const currentLyric = this.parsedLyric[nextIndex - 1] ?? "";
            const nextLyric = this.parsedLyric[nextIndex] ?? "";

            const lyrics = {
                "basic": currentLyric?.originalLyric ?? "",
                "extra": currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? ""
            };

            this.processExtraShow(lyrics, currentLyric, nextLyric);

            apiInstance.lyrics(lyrics);
            this.currentIndex = nextIndex;
        }
    }

    /**
     * 处理额外歌词显示
     * @description 根据配置决定额外歌词（副歌词）显示的内容
     * @param {Object} lyrics - 歌词对象
     * @param {Object} currentLyric - 当前歌词行
     * @param {Object} nextLyric - 下一行歌词
     */
    processExtraShow(lyrics, currentLyric, nextLyric) {
        const extraShowValue = ConfigManager.get("effect")["extra_show"]["value"];
        
        switch (extraShowValue) {
            case 0: // 不显示副歌词
                lyrics.extra = "";
                break;

            case 1: // 下一句歌词或交换显示
                const nextLinePos = ConfigManager.get("effect")["next_line_lyrics_position"]["value"];
                switch (nextLinePos) {
                    case 0: // 副歌词显示下一句
                        lyrics.extra = nextLyric?.originalLyric ?? "";
                        break;
                    case 1: // 主歌词显示下一句，副歌词显示当前句
                        lyrics.basic = nextLyric?.originalLyric ?? "";
                        lyrics.extra = currentLyric?.originalLyric ?? "";
                        break;
                    case 2: // 轮流显示
                        if (this.currentLine == 0) {
                            lyrics.basic = currentLyric?.originalLyric ?? "";
                            lyrics.extra = nextLyric?.originalLyric ?? "";
                            this.currentLine = 1;
                        } else {
                            lyrics.basic = nextLyric?.originalLyric ?? "";
                            lyrics.extra = currentLyric?.originalLyric ?? "";
                            this.currentLine = 0;
                        }
                        break;
                }
                break;

            case 2: // 翻译或下一句原始内容
                lyrics.extra = currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? "";
                break;

            case 3: // 罗马音或翻译或下一句原始内容
                lyrics.extra = currentLyric?.romanLyric 
                    ?? currentLyric?.translatedLyric 
                    ?? nextLyric?.originalLyric 
                    ?? "";
                break;
        }
    }

    /**
     * 启动歌词处理
     * @description 根据配置的获取方式启动相应的歌词处理逻辑
     */
    start() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0: // 监听内置歌词
                this.watchLyricsChange();
                break;
            case 1: // LibLyric
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                const playingSong = betterncm.ncm.getPlayingSong();
                if (playingSong && playingSong.data && playingSong.data.id != this.musicId) {
                    this.playLoad();
                }
                break;
            case 2: // RefinedNowPlaying
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                break;
        }
    }

    /**
     * 停止歌词处理
     * @description 清理监听器和回调
     */
    stop() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0:
                if (this.observer) {
                    this.observer.disconnect();
                    this.observer = null;
                }
                break;
            case 1:
            case 2:
                legacyNativeCmder.removeRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.removeRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                break;
        }
    }
}

const lyricManager = new LyricManager();
