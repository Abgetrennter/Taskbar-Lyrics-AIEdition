# Taskbar-Lyrics 项目深度分析报告

本文档是对 `Taskbar-Lyrics` 代码库的全面分析，旨在帮助开发者理解其架构、功能实现、接口定义及维护方式。

## 1. 代码结构分析

该项目采用前后端分离的混合架构，由 **BetterNCM 插件 (前端)** 和 **Windows 原生程序 (后端)** 组成。

### 1.1 目录结构概览

```text
e:\Code\Taskbar-Lyrics-1.x.x
├── .github/workflows/   # CI/CD 自动化构建脚本
├── dist/                # 构建产物（用户最终使用的文件）
├── src/
│   ├── betterncm-plugin/ # [前端] BetterNCM 插件源码 (JS/HTML/CSS)
│   │   ├── base.js       # 基础配置与 API 封装
│   │   ├── lyric.js      # 歌词获取与处理逻辑
│   │   ├── config.html   # 设置界面
│   │   └── ...
│   └── taskbar-lyrics/   # [后端] C++ 原生程序源码
│       ├── NetworkServer.* # HTTP 服务器实现
│       ├── CreateWindow.*  # 窗口创建与管理
│       ├── RenderWindow.*  # Direct2D 渲染逻辑
│       └── TaskbarLyrics.cpp # 程序入口
└── *.sln, *.vcxproj      # Visual Studio 工程文件
```

### 1.2 模块划分与依赖

| 模块 | 语言 | 职责 | 关键依赖 |
| :--- | :--- | :--- | :--- |
| **Frontend (Plugin)** | JavaScript | 1. 从网易云音乐获取歌词<br>2. 提供用户配置界面<br>3. 发送数据给后端 | BetterNCM API |
| **Backend (Native)** | C++ (Win32) | 1. 运行本地 HTTP 服务器接收数据<br>2. 创建透明任务栏窗口<br>3. 使用 Direct2D 渲染歌词 | Windows API, Direct2D, DirectWrite, cpp-httplib, nlohmann-json |

---

## 2. 核心功能解析

### 2.1 歌词获取 (前端)
位于 `src/betterncm-plugin/lyric.js`。插件支持三种歌词源模式：
1.  **DOM 监听**: 使用 `MutationObserver` 实时监听网易云音乐界面 (`#x-g-mn .m-lyric`) 的 DOM 变化。这是最实时的方案。
2.  **LibLyric**: 调用 BetterNCM 社区库 `liblyric` 解析歌词文件。
3.  **RefinedNowPlaying**: 兼容另一个插件的歌词数据源。

### 2.2 跨进程通信
前端与后端通过 **本地 HTTP (Localhost)** 进行通信。
- **服务端**: C++ 程序启动一个 HTTP Server，监听端口 `BETTERNCM_API_PORT - 2`。
- **客户端**: JS 插件通过 `fetch` 发送 POST 请求传输 JSON 数据。

### 2.3 窗口嵌入与渲染 (后端)
- **嵌入任务栏**: 使用 `SetParent` API 将窗口父节点设置为任务栏窗口 (`Shell_TrayWnd`)，实现“嵌入”效果。
- **透明背景**: 使用 `WS_EX_LAYERED | WS_EX_TRANSPARENT` 窗口样式实现背景透明和鼠标穿透。
- **高性能渲染**: 使用 **Direct2D** 和 **DirectWrite** 进行文本绘制，支持硬件加速和抗锯齿，确保在不同分辨率下的显示效果。

---

## 3. 接口文档

后端服务监听地址：`http://127.0.0.1:<PORT>` (PORT 默认为 BetterNCM 端口 - 2)
所有接口均使用 **POST** 方法，请求体为 **JSON** 格式。

### 3.1 歌词控制

#### 发送歌词 `/taskbar/lyrics/lyrics`
更新当前显示的歌词内容。
- **请求参数**:
  ```json
  {
    "basic": "主歌词内容（如：原语言）",
    "extra": "副歌词内容（如：翻译）"
  }
  ```

#### 歌词对齐 `/taskbar/lyrics/align`
设置歌词的文本对齐方式。
- **请求参数**:
  ```json
  {
    "basic": 0, // 主歌词对齐方式 (0:左, 1:右, 2:中, 3:两端)
    "extra": 0  // 副歌词对齐方式
  }
  ```
  *注：值对应 DirectWrite 的 `DWRITE_TEXT_ALIGNMENT` 枚举。*

### 3.2 样式设置

#### 设置字体 `/taskbar/font/font`
- **请求参数**:
  ```json
  {
    "font_family": "Microsoft YaHei UI" // 字体名称
  }
  ```

#### 设置颜色 `/taskbar/font/color`
- **请求参数**:
  ```json
  {
    "basic": {
      "light": { "hex_color": 0x000000, "opacity": 1.0 }, // 浅色模式
      "dark": { "hex_color": 0xFFFFFF, "opacity": 1.0 }   // 深色模式
    },
    "extra": { ... } // 结构同 basic
  }
  ```
  *注：颜色值为 16 进制整数 (如 0xFF0000 表示红色)。*

#### 设置样式 `/taskbar/font/style`
- **请求参数**:
  ```json
  {
    "basic": {
      "weight": { "value": 400 }, // 字重 (400: Normal, 700: Bold)
      "slope": 0,                 // 倾斜 (0: Normal, 1: Oblique, 2: Italic)
      "underline": false,         // 下划线
      "strikethrough": false      // 删除线
    },
    "extra": { ... }
  }
  ```

### 3.3 窗口控制

#### 窗口位置 `/taskbar/window/position`
- **请求参数**:
  ```json
  {
    "position": { "value": 0 } // 0: 自适应, 1: 左, 2: 中, 3: 右
  }
  ```

#### 窗口边距 `/taskbar/window/margin`
- **请求参数**:
  ```json
  {
    "left": 0,
    "right": 0
  }
  ```

#### 设置父窗口 `/taskbar/window/screen`
- **请求参数**:
  ```json
  {
    "parent_taskbar": { "value": "Shell_TrayWnd" } // 任务栏窗口类名
  }
  ```

#### 关闭程序 `/taskbar/close`
关闭 C++ 后端程序。无特殊参数。

---

## 4. 数据流说明

系统内部的数据流是一个单向的管道：

1.  **Trigger (事件触发)**: 用户在网易云音乐切歌或进度改变。
2.  **Capture (捕获)**: `betterncm-plugin/lyric.js` 捕获事件，从 DOM 或 API 获取当前时间点的歌词文本。
3.  **Process (处理)**: 插件根据配置（如是否显示翻译、下一句位置）格式化歌词对象。
4.  **Transport (传输)**: `betterncm-plugin/base.js` 将 JSON 数据 POST 到本地 HTTP 服务器。
5.  **Receive (接收)**: `taskbar-lyrics/NetworkServer.cpp` 接收请求，解析 JSON，更新 `RenderWindow` 对象的成员变量。
6.  **Render (渲染)**: `NetworkServer` 发送 `WM_PAINT` 消息 -> `RenderWindow.cpp` 触发重绘 -> Direct2D 将新文本画在屏幕上。

---

## 5. 部署与构建指南

### 5.1 环境要求
- **操作系统**: Windows 10 / 11 (推荐 Windows 11)
- **编译环境**: Visual Studio 2022 (支持 C++17 或更高)
- **包管理器**: vcpkg

### 5.2 构建步骤
1.  **安装依赖库**:
    使用 vcpkg 安装必要的 C++ 库：
    ```powershell
    vcpkg install cpp-httplib:x86-windows
    vcpkg install nlohmann-json:x86-windows
    vcpkg integrate install
    ```

2.  **编译项目**:
    打开 `Taskbar Lyrics.sln`，选择 **Release** 配置和 **x86** 平台，点击生成。

3.  **打包**:
    编译生成的 `taskbar-lyrics.exe` 需要与 `src/betterncm-plugin/` 下的所有文件放在同一目录（即 `dist/` 目录结构）。

4.  **安装**:
    将 `dist` 文件夹复制到 BetterNCM 的插件目录下即可。

---

## 6. 维护说明与已知问题

### 6.1 已知问题
- **端口冲突风险**: 后端监听端口通过 `BETTERNCM_API_PORT - 2` 计算得出。如果该端口被其他程序占用，会导致启动失败。
- **任务栏兼容性**: 程序强依赖于 Windows 任务栏的窗口类名 (`Shell_TrayWnd`) 和结构。如果 Windows 更新更改了任务栏实现（如 Windows 11 的某些早期版本或未来更新），可能会导致无法嵌入。
- **高分屏适配**: 虽然使用了 Direct2D，但在不同 DPI 设置的多显示器环境下，窗口位置计算可能需要额外校准。

### 6.2 优化建议
- **心跳机制**: 目前只有前端调后端。建议增加后端对前端的心跳检测，如果前端（网易云）关闭，后端应自动退出以节省资源。
- **错误处理**: 增强 HTTP 请求的错误处理机制，当后端未启动时，前端应尝试唤起后端或提示用户。
- **WebSocket 升级**: 考虑将 HTTP POST 轮询/推送改为 WebSocket 长连接，可以减少 TCP 握手开销，提高实时性。
