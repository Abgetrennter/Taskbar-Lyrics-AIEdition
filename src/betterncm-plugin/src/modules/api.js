"use strict";

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

    connect() {
        if (this.socket && (this.socket.readyState === WebSocket.OPEN || this.socket.readyState === WebSocket.CONNECTING)) {
            return;
        }

        this.socket = new WebSocket(`ws://127.0.0.1:${this.port}`);

        this.socket.onopen = () => {
            console.log("Taskbar Lyrics: WebSocket connected");
            this.retryCount = 0;
            if (this.reconnectTimeout) clearTimeout(this.reconnectTimeout);

            // Flush queue
            while (this.messageQueue.length > 0) {
                const msg = this.messageQueue.shift();
                this.socket.send(msg);
            }

            // Start heartbeat
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

            this.reconnectTimeout = setTimeout(() => this.connect(), 3000);
        };

        this.socket.onerror = (err) => {
            console.error("Taskbar Lyrics: WebSocket error", err);
            this.socket.close();
            this.callbacks.onError.forEach(cb => cb(err));
        };
    }

    fetch(path, params) {
        const payload = Utils.flattenObject(params);
        payload.url = "/taskbar" + path;
        const msg = JSON.stringify(payload);

        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.send(msg);
        } else {
            // If it's a lyric update, remove previous lyric updates from queue to avoid buildup
            if (payload.url === "/taskbar/lyrics/lyrics") {
                this.messageQueue = this.messageQueue.filter(m => !m.includes('"/taskbar/lyrics/lyrics"'));
            }

            this.messageQueue.push(msg);

            if (this.messageQueue.length > 100) this.messageQueue.shift();

            this.connect();
        }
    }

    // API Methods
    font(params) { this.fetch("/font/font", params); }
    color(params) { this.fetch("/font/color", params); }
    style(params) { this.fetch("/font/style", params); }
    size(params) { this.fetch("/font/size", params); }
    lyrics(params) { this.fetch("/lyrics/lyrics", params); }
    align(params) { this.fetch("/lyrics/align", params); }
    windowPosition(params) { this.fetch("/window/position", params); }
    windowMargin(params) { this.fetch("/window/margin", params); }
    windowScreen(params) { this.fetch("/window/screen", params); }
    close(params) { this.fetch("/close", params); }

    on(event, callback) {
        if (this.callbacks[event]) {
            this.callbacks[event].push(callback);
        }
    }
}

const apiInstance = new TaskbarLyricsAPI();
