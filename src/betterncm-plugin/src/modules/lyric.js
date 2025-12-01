"use strict";

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
            // RefinedNowPlaying integration
            let retries = 0;
            while (retries < 50) { // Avoid infinite loop
                if (window.currentLyrics.hash.includes(this.musicId)) {
                    this.parsedLyric = window.currentLyrics.lyrics;
                    break;
                } else {
                    await Utils.delay(100);
                    retries++;
                }
            }
        } else {
            const lyricData = await this.liblyric.getLyricData(this.musicId);
            this.parsedLyric = this.liblyric.parseLyric(
                lyricData?.lrc?.lyric ?? "",
                lyricData?.tlyric?.lyric ?? "",
                lyricData?.romalrc?.lyric ?? ""
            );
        }

        if (this.parsedLyric) {
            this.parsedLyric = this.parsedLyric.filter(item => item.originalLyric != "");

            // Pure music check
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

    processExtraShow(lyrics, currentLyric, nextLyric) {
        const extraShowValue = ConfigManager.get("effect")["extra_show"]["value"];
        
        switch (extraShowValue) {
            case 0: // No extra
                lyrics.extra = "";
                break;

            case 1: // Next line or swap
                const nextLinePos = ConfigManager.get("effect")["next_line_lyrics_position"]["value"];
                switch (nextLinePos) {
                    case 0:
                        lyrics.extra = nextLyric?.originalLyric ?? "";
                        break;
                    case 1:
                        lyrics.basic = nextLyric?.originalLyric ?? "";
                        lyrics.extra = currentLyric?.originalLyric ?? "";
                        break;
                    case 2:
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

            case 2: // Translation or next original
                lyrics.extra = currentLyric?.translatedLyric ?? nextLyric?.originalLyric ?? "";
                break;

            case 3: // Roman or Translation or next original
                lyrics.extra = currentLyric?.romanLyric 
                    ?? currentLyric?.translatedLyric 
                    ?? nextLyric?.originalLyric 
                    ?? "";
                break;
        }
    }

    start() {
        const config = ConfigManager.get("lyrics");
        const method = config["retrieval_method"]["value"];

        switch (method) {
            case 0:
                this.watchLyricsChange();
                break;
            case 1:
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                const playingSong = betterncm.ncm.getPlayingSong();
                if (playingSong && playingSong.data && playingSong.data.id != this.musicId) {
                    this.playLoad();
                }
                break;
            case 2:
                legacyNativeCmder.appendRegisterCall("Load", "audioplayer", this.boundPlayLoad);
                legacyNativeCmder.appendRegisterCall("PlayProgress", "audioplayer", this.boundPlayProgress);
                break;
        }
    }

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
