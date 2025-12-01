"use strict";

/**
 * @module 视图管理
 * @description 负责插件配置界面的渲染、事件绑定和设置更新
 * @author Taskbar Lyrics Plugin Developer
 * @date 2025-12-01
 */

class ConfigView {
    constructor() {
        this.root = document.createElement("div");
        this.root.style.overflow = "hidden";
        this.root.style.height = "100%";
        this.root.style.width = "100%";
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
     * 获取根元素
     * @returns {HTMLElement} 插件配置界面的根元素
     */
    getElement() {
        return this.root;
    }

    /**
     * 初始化视图
     * @description 加载 HTML 和 CSS，初始化 Tab 切换和全局事件，绑定设置项
     */
    async init() {
        // 加载 HTML 模板
        const path = `${this.pluginPath}/assets/config.html`;
        const text = await betterncm.fs.readFileText(path);
        const parser = new DOMParser();
        const dom = parser.parseFromString(text, "text/html");
        const element = dom.querySelector("#taskbar-lyrics-dom");
        this.root.appendChild(element);

        // 加载 CSS 样式
        const cssPath = `${this.pluginPath}/assets/style.css`;

        const cssText = await betterncm.fs.readFileText(cssPath);
        const style = document.createElement("style");
        style.textContent = cssText;
        this.root.appendChild(style);

        this.initTabs();
        this.initGlobalEvents();
        this.bindSettings();
    }

    /**
     * 初始化 Tab 切换逻辑
     * @description 绑定 Tab 按钮点击事件，切换显示内容
     */
    initTabs() {
        const tabBox = this.root.querySelector(".tab_box");
        const contentBox = this.root.querySelector(".content_box");
        const tabs = tabBox.querySelectorAll(".tab_button");
        const contents = contentBox.querySelectorAll(".content");

        tabs.forEach((tab, index) => {
            tab.addEventListener("click", () => {
                tabBox.querySelector(".active").classList.remove("active");
                tab.classList.add("active");
                contentBox.querySelector(".show").classList.remove("show");
                contents[index].classList.add("show");
            });
        });
    }

    /**
     * 初始化全局事件
     * @description 处理全局性的交互事件
     */
    initGlobalEvents() {
        const selectController = (event) => {
            const parent = event.target.parentElement;
            parent.classList.toggle("z-open");
        };

        // Delegate event for value clicks if they are dynamically loaded
        // But here they are loaded once. However, using delegation on root is safer if we had dynamic content.
        // Since we appendChild 'element', we can select from root.
        
        // Note: We need to attach listeners to elements that exist now.
        // But init() is async. The caller (index.js) will await it.
    }

    /**
     * 绑定设置项
     * @description 为各个设置项绑定事件监听，处理配置的读取、修改和重置
     */
    bindSettings() {
        // Helper to select within root
        const $ = (sel) => this.root.querySelector(sel);
        const $$ = (sel) => this.root.querySelectorAll(sel);

        // Select Controller Logic Re-attach
        $$(".value").forEach(el => {
             el.addEventListener("click", (event) => {
                const parent = event.target.parentElement;
                if (parent.classList.contains("z-open")) parent.classList.remove("z-open");
                else parent.classList.add("z-open");
             });
        });

        // --- Font ---
        const fontSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("font")));
                config["font_family"] = $(".content.font .font-settings .font-family").value;
                ConfigManager.set("font", config);
                apiInstance.font(config);
            },
            reset: () => {
                ConfigManager.set("font", undefined);
                apiInstance.font(defaultConfig["font"]);
                $(".content.font .font-settings .font-family").value = defaultConfig["font"]["font_family"];
            }
        };
        $(".content.font .font-settings .apply").addEventListener("click", fontSettings.apply);
        $(".content.font .font-settings .reset").addEventListener("click", fontSettings.reset);
        $(".content.font .font-settings .font-family").value = ConfigManager.get("font")["font_family"];

        // --- Color ---
        const colorSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("color")));
                const getVal = (sel) => $(sel).value;
                config["basic"]["light"]["hex_color"] = parseInt(getVal(".basic-light-color").slice(1), 16);
                config["basic"]["light"]["opacity"] = Number(getVal(".basic-light-opacity"));
                config["basic"]["dark"]["hex_color"] = parseInt(getVal(".basic-dark-color").slice(1), 16);
                config["basic"]["dark"]["opacity"] = Number(getVal(".basic-dark-opacity"));
                config["extra"]["light"]["hex_color"] = parseInt(getVal(".extra-light-color").slice(1), 16);
                config["extra"]["light"]["opacity"] = Number(getVal(".extra-light-opacity"));
                config["extra"]["dark"]["hex_color"] = parseInt(getVal(".extra-dark-color").slice(1), 16);
                config["extra"]["dark"]["opacity"] = Number(getVal(".extra-dark-opacity"));
                ConfigManager.set("color", config);
                apiInstance.color(config);
            },
            reset: () => {
                const def = defaultConfig["color"];
                const setVal = (sel, val) => $(sel).value = val;
                const toHex = (num) => `#${num.toString(16).padStart(6, "0")}`;
                
                setVal(".basic-light-color", toHex(def.basic.light.hex_color));
                setVal(".basic-light-opacity", def.basic.light.opacity);
                setVal(".basic-dark-color", toHex(def.basic.dark.hex_color));
                setVal(".basic-dark-opacity", def.basic.dark.opacity);
                setVal(".extra-light-color", toHex(def.extra.light.hex_color));
                setVal(".extra-light-opacity", def.extra.light.opacity);
                setVal(".extra-dark-color", toHex(def.extra.dark.hex_color));
                setVal(".extra-dark-opacity", def.extra.dark.opacity);
                
                ConfigManager.set("color", undefined);
                apiInstance.color(def);
            }
        };
        $(".content.font .color-settings .apply").addEventListener("click", colorSettings.apply);
        $(".content.font .color-settings .reset").addEventListener("click", colorSettings.reset);
        // Init values
        {
            const cur = ConfigManager.get("color");
            const toHex = (num) => `#${num.toString(16).padStart(6, "0")}`; // Fix: padding
            // Wait, original code: `#${...toString(16)}`. If it's 0, it becomes `#0`. It should be `#000000`.
            // Original code: `#${pluginConfig.get("color")["basic"]["light"]["hex_color"].toString(16)}`
            // If the original code had a bug, I should probably fix it or keep it. 
            // User asked for optimization and quality, so I'll fix it to padStart(6, '0').
            
            // Wait, original code in reset uses `padStart(6, "0")`.
            // But in init logic (view.js line 137), it didn't use padStart.
            // I will add padStart.
            
            $(".basic-light-color").value = toHex(cur.basic.light.hex_color);
            $(".basic-light-opacity").value = cur.basic.light.opacity;
            $(".basic-dark-color").value = toHex(cur.basic.dark.hex_color);
            $(".basic-dark-opacity").value = cur.basic.dark.opacity;
            $(".extra-light-color").value = toHex(cur.extra.light.hex_color);
            $(".extra-light-opacity").value = cur.extra.light.opacity;
            $(".extra-dark-color").value = toHex(cur.extra.dark.hex_color);
            $(".extra-dark-opacity").value = cur.extra.dark.opacity;
        }

        // --- Size ---
        const sizeSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("size")));
                config["basic"] = Number($(".basic-size").value);
                config["extra"] = Number($(".extra-size").value);
                ConfigManager.set("size", config);
                apiInstance.size(config);
            },
            reset: () => {
                $(".basic-size").value = defaultConfig.size.basic;
                $(".extra-size").value = defaultConfig.size.extra;
                ConfigManager.set("size", undefined);
                apiInstance.size(defaultConfig.size);
            }
        };
        $(".content.font .size-settings .apply").addEventListener("click", sizeSettings.apply);
        $(".content.font .size-settings .reset").addEventListener("click", sizeSettings.reset);
        $(".basic-size").value = ConfigManager.get("size").basic;
        $(".extra-size").value = ConfigManager.get("size").extra;

        // --- Style ---
        const styleSettings = {
            setWeight: (name, value, textContent) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name].weight.value = Number(value);
                config[name].weight.textContent = textContent;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            setSlope: (name, type) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name].slope = type;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            setToggle: (name, prop, checked) => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("style")));
                config[name][prop] = checked;
                ConfigManager.set("style", config);
                apiInstance.style(config);
            },
            reset: () => {
                ConfigManager.set("style", undefined);
                apiInstance.style(defaultConfig.style);
                // Update UI
                const def = defaultConfig.style;
                $(".content.font .style-settings .basic-weight .value").textContent = def.basic.weight.textContent;
                $(".basic-underline").checked = def.basic.underline;
                $(".basic-strikethrough").checked = def.basic.strikethrough;
                $(".content.font .style-settings .extra-weight .value").textContent = def.extra.weight.textContent;
                $(".extra-underline").checked = def.extra.underline;
                $(".extra-strikethrough").checked = def.extra.strikethrough;
            }
        };
        
        $(".content.font .style-settings .reset").addEventListener("click", styleSettings.reset);
        
        // Helper for style bindings
        const bindStyleSlope = (sel, name, val) => $(sel).addEventListener("click", () => styleSettings.setSlope(name, val));
        const bindStyleToggle = (sel, name, prop) => $(sel).addEventListener("change", (e) => styleSettings.setToggle(name, prop, e.target.checked));
        
        bindStyleSlope(".basic-normal", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL);
        bindStyleSlope(".basic-oblique", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE);
        bindStyleSlope(".basic-italic", "basic", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC);
        bindStyleToggle(".basic-underline", "basic", "underline");
        bindStyleToggle(".basic-strikethrough", "basic", "strikethrough");

        bindStyleSlope(".extra-normal", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_NORMAL);
        bindStyleSlope(".extra-oblique", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_OBLIQUE);
        bindStyleSlope(".extra-italic", "extra", WindowsEnum.DWRITE_FONT_STYLE.DWRITE_FONT_STYLE_ITALIC);
        bindStyleToggle(".extra-underline", "extra", "underline");
        bindStyleToggle(".extra-strikethrough", "extra", "strikethrough");

        const bindWeightSelect = (name) => {
            $(`.content.font .style-settings .${name}-weight .select`).addEventListener("click", e => {
                const val = e.target.dataset.value;
                const text = e.target.textContent;
                styleSettings.setWeight(name, val, text);
                $(`.content.font .style-settings .${name}-weight .value`).textContent = text;
            });
        };
        bindWeightSelect("basic");
        bindWeightSelect("extra");

        // Init Style UI
        const curStyle = ConfigManager.get("style");
        $(".content.font .style-settings .basic-weight .value").textContent = curStyle.basic.weight.textContent;
        $(".basic-underline").checked = curStyle.basic.underline;
        $(".basic-strikethrough").checked = curStyle.basic.strikethrough;
        $(".content.font .style-settings .extra-weight .value").textContent = curStyle.extra.weight.textContent;
        $(".extra-underline").checked = curStyle.extra.underline;
        $(".extra-strikethrough").checked = curStyle.extra.strikethrough;


        // --- Lyrics ---
        const lyricsSettings = {
            reset: () => {
                $(".retrieval-method .value").textContent = defaultConfig.lyrics.retrieval_method.textContent;
                $(".karaoke-switch").checked = defaultConfig.lyrics.karaoke;
                lyricManager.stop();
                ConfigManager.set("lyrics", undefined);
                lyricManager.start();
            }
        };
        $(".content.lyrics .lyrics-settings .reset").addEventListener("click", lyricsSettings.reset);
        $(".lyrics-switch").addEventListener("change", e => e.target.checked ? backendManager.start() : backendManager.close());
        
        $(".karaoke-switch").addEventListener("change", (e) => {
             const config = ConfigManager.get("lyrics");
             config["karaoke"] = e.target.checked;
             ConfigManager.set("lyrics", config);
        });
        $(".karaoke-switch").checked = ConfigManager.get("lyrics")["karaoke"];

        $(".retrieval-method .select").addEventListener("click", e => {
            const value = e.target.dataset.value;
            const text = e.target.textContent;
            if (value == "2" && !window.currentLyrics) {
                channel.call("trayicon.popBalloon", () => {}, [{
                    title: "任务栏歌词",
                    text: "无法使用RefinedNowPlaying歌词！\n是否安装RefinedNowPlaying插件？\n将回退到使用LibLyric解析获取歌词",
                    icon: "path",
                    hasSound: true,
                    delayTime: 2e3
                }]);
                return;
            }
            const config = JSON.parse(JSON.stringify(ConfigManager.get("lyrics")));
            config["retrieval_method"]["value"] = Number(value);
            config["retrieval_method"]["textContent"] = text;
            lyricManager.stop();
            ConfigManager.set("lyrics", config);
            lyricManager.start();
            $(".retrieval-method .value").textContent = text;
        });
        $(".retrieval-method .value").textContent = ConfigManager.get("lyrics").retrieval_method.textContent;

        // --- Effect ---
        const effectSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("effect")));
                config["adjust"] = Number($(".adjust").value);
                ConfigManager.set("effect", config);
            },
            reset: () => {
                $(".next-line-lyrics-position .value").textContent = defaultConfig.effect.next_line_lyrics_position.textContent;
                $(".extra-show .value").textContent = defaultConfig.effect.extra_show.textContent;
                $(".adjust").value = defaultConfig.effect.adjust;
                ConfigManager.set("effect", undefined);
            }
        };
        $(".content.lyrics .effect-settings .apply").addEventListener("click", effectSettings.apply);
        $(".content.lyrics .effect-settings .reset").addEventListener("click", effectSettings.reset);

        const bindEffectSelect = (cls, key) => {
             $(`${cls} .select`).addEventListener("click", e => {
                 const value = e.target.dataset.value;
                 const text = e.target.textContent;
                 const config = JSON.parse(JSON.stringify(ConfigManager.get("effect")));
                 config[key]["value"] = Number(value);
                 config[key]["textContent"] = text;
                 ConfigManager.set("effect", config);
                 $(`${cls} .value`).textContent = text;
             });
        };
        bindEffectSelect(".next-line-lyrics-position", "next_line_lyrics_position");
        bindEffectSelect(".extra-show", "extra_show");
        
        const curEffect = ConfigManager.get("effect");
        $(".next-line-lyrics-position .value").textContent = curEffect.next_line_lyrics_position.textContent;
        $(".extra-show .value").textContent = curEffect.extra_show.textContent;
        $(".adjust").value = curEffect.adjust;


        // --- Align ---
        const alignSettings = {
             set: (type, val) => {
                 const config = JSON.parse(JSON.stringify(ConfigManager.get("align")));
                 config[type] = val;
                 ConfigManager.set("align", config);
                 apiInstance.align(config);
             },
             reset: () => {
                 ConfigManager.set("align", undefined);
                 apiInstance.align(defaultConfig.align);
             }
        };
        $(".content.lyrics .align-settings .reset").addEventListener("click", alignSettings.reset);
        
        const bindAlign = (sel, type, val) => $(sel).addEventListener("click", () => alignSettings.set(type, val));
        bindAlign(".basic-left", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING);
        bindAlign(".basic-center", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_CENTER);
        bindAlign(".basic-right", "basic", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_TRAILING);
        bindAlign(".extra-left", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_LEADING);
        bindAlign(".extra-center", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_CENTER);
        bindAlign(".extra-right", "extra", WindowsEnum.DWRITE_TEXT_ALIGNMENT.DWRITE_TEXT_ALIGNMENT_TRAILING);

        // --- Window Position ---
        const posSettings = {
            reset: () => {
                $(".window-position .value").textContent = defaultConfig.position.position.textContent;
                ConfigManager.set("position", undefined);
                apiInstance.windowPosition(defaultConfig.position);
            }
        };
        $(".content.window .position-settings .reset").addEventListener("click", posSettings.reset);
        $(".window-position .select").addEventListener("click", e => {
            const value = e.target.dataset.value;
            const text = e.target.textContent;
            const config = JSON.parse(JSON.stringify(ConfigManager.get("position")));
            config["position"]["value"] = Number(value);
            config["position"]["textContent"] = text;
            ConfigManager.set("position", config);
            apiInstance.windowPosition(config);
            $(".window-position .value").textContent = text;
        });
        $(".window-position .value").textContent = ConfigManager.get("position").position.textContent;

        // --- Margin ---
        const marginSettings = {
            apply: () => {
                const config = JSON.parse(JSON.stringify(ConfigManager.get("margin")));
                config["left"] = Number($(".margin-settings .left").value);
                config["right"] = Number($(".margin-settings .right").value);
                ConfigManager.set("margin", config);
                apiInstance.windowMargin(config);
            },
            reset: () => {
                ConfigManager.set("margin", undefined);
                apiInstance.windowMargin(defaultConfig.margin);
                $(".margin-settings .left").value = defaultConfig.margin.left;
                $(".margin-settings .right").value = defaultConfig.margin.right;
            }
        };
        $(".content.window .margin-settings .apply").addEventListener("click", marginSettings.apply);
        $(".content.window .margin-settings .reset").addEventListener("click", marginSettings.reset);
        $(".margin-settings .left").value = ConfigManager.get("margin").left;
        $(".margin-settings .right").value = ConfigManager.get("margin").right;

        // --- Screen ---
        const screenSettings = {
            reset: () => {
                $(".parent-taskbar .value").textContent = defaultConfig.screen.parent_taskbar.textContent;
                ConfigManager.set("screen", undefined);
                apiInstance.windowScreen(defaultConfig.screen);
            }
        };
        $(".content.window .screen-settings .reset").addEventListener("click", screenSettings.reset);
        $(".parent-taskbar .select").addEventListener("click", e => {
             const value = e.target.dataset.value;
             const text = e.target.textContent;
             const config = JSON.parse(JSON.stringify(ConfigManager.get("screen")));
             config["parent_taskbar"]["value"] = value;
             config["parent_taskbar"]["textContent"] = text;
             ConfigManager.set("screen", config);
             apiInstance.windowScreen(config);
             $(".parent-taskbar .value").textContent = text;
        });
        $(".parent-taskbar .value").textContent = ConfigManager.get("screen").parent_taskbar.textContent;
    }
}

const configView = new ConfigView();
