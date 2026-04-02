# Taskbar-Lyrics 项目深度分析报告

本文档是对 `Taskbar-Lyrics` 代码库的全面分析，旨在帮助开发者理解其架构、功能实现、接口定义及维护方式。

## 1. 代码结构分析

该项目采用前后端分离的解耦架构，由 **BetterNCM 插件 (前端)** 和 **Windows 原生程序 (后端)** 组成。两者独立运行，通过 HTTP API 进行通信。

### 1.1 目录结构概览

```text
e:\Code\Taskbar-Lyrics-1.x.x
├── .github/workflows/   # CI/CD 自动化构建脚本
├── dist/                # 构建产物（用户最终使用的文件）
├── src/
│   ├── betterncm-plugin/ # [前端] BetterNCM 插件源码 (JS/HTML/CSS)
│   │   ├── base.js       # 基础配置与 HTTP 通信封装
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
| **Frontend (Plugin)** | JavaScript | 1. 从网易云音乐获取歌词<br>2. 提供用户配置界面<br>3. 通过 HTTP API 发送数据到后端 | BetterNCM API |
| **Backend (Native)** | C++ (Win32) | 1. 独立运行，提供 HTTP API 接口<br>2. 创建透明任务栏窗口<br>3. 使用 Direct2D 渲染歌词<br>4. **不再自动退出，需手动管理生命周期** | Windows API, Direct2D, DirectWrite, WinSock2 |

---

## 2. 核心功能解析

### 2.1 歌词获取 (前端)
位于 `src/betterncm-plugin/lyric.js`。插件支持三种歌词源模式：
1.  **DOM 监听**: 使用 `MutationObserver` 实时监听网易云音乐界面 (`#x-g-mn .m-lyric`) 的 DOM 变化。这是最实时的方案。
2.  **LibLyric**: 调用 BetterNCM 社区库 `liblyric` 解析歌词文件。
3.  **RefinedNowPlaying**: 兼容另一个插件的歌词数据源。

### 2.2 跨进程通信
前端与后端通过 **HTTP RESTful API** 进行通信。
- **服务端**: C++ 程序启动一个 TCP Server，处理 HTTP POST 请求，默认监听端口 `27232`。
- **客户端**: JS 插件通过 `fetch` API 发送 POST 请求。
- **去耦合**: 移除了 WebSocket 长连接、心跳检测和自动退出机制。后端服务需独立启动。

### 2.3 窗口嵌入与渲染 (后端)
- **嵌入任务栏**: 使用 `SetParent` API 将窗口父节点设置为任务栏窗口 (`Shell_TrayWnd`)，实现“嵌入”效果。
- **透明背景**: 使用 `WS_EX_LAYERED | WS_EX_TRANSPARENT` 窗口样式实现背景透明和鼠标穿透。
- **高性能渲染**: 使用 **Direct2D** 和 **DirectWrite** 进行文本绘制，支持硬件加速和抗锯齿，确保在不同分辨率下的显示效果。

---

## 3. 接口文档

后端服务监听地址：`http://127.0.0.1:<PORT>` (PORT 默认为 27232，可通过命令行参数覆盖)
通信方式：HTTP POST
数据格式：JSON Body

### 3.1 歌词控制

#### 发送歌词
更新当前显示的歌词内容。
- **Endpoint**: `/taskbar/lyrics/lyrics`
- **Method**: POST
- **Body**:
  ```json
  {
    "basic": "主歌词内容（如：原语言）",
    "extra": "副歌词内容（如：翻译）"
  }
  ```

#### 歌词对齐
设置歌词的文本对齐方式。
- **Endpoint**: `/taskbar/lyrics/align`
- **Method**: POST
- **Body**:
  ```json
  {
    "basic": 0, // 主歌词对齐方式 (0:左, 1:右, 2:中, 3:两端)
    "extra": 0  // 副歌词对齐方式
  }
  ```
  *注：值对应 DirectWrite 的 `DWRITE_TEXT_ALIGNMENT` 枚举。*

### 3.2 样式设置

#### 设置字体
- **Endpoint**: `/taskbar/font/font`
- **Method**: POST
- **Body**:
  ```json
  {
    "font_family": "Microsoft YaHei UI" // 字体名称
  }
  ```

#### 设置颜色
- **Endpoint**: `/taskbar/font/color`
- **Method**: POST
- **Body**:
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

#### 设置样式
- **Endpoint**: `/taskbar/font/style`
- **Method**: POST
- **Body**:
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

#### 设置大小
- **Endpoint**: `/taskbar/font/size`
- **Method**: POST
- **Body**:
  ```json
  {
    "basic": 20.0, // 主歌词字体大小 (单位: 像素)
    "extra": 15.0  // 副歌词字体大小
  }
  ```

### 3.3 窗口控制

#### 窗口位置
- **Endpoint**: `/taskbar/window/position`
- **Method**: POST
- **Body**:
  ```json
  {
    "position": { "value": 0 } // 0: 自适应, 1: 左, 2: 中, 3: 右
  }
  ```

#### 窗口边距
- **Endpoint**: `/taskbar/window/margin`
- **Method**: POST
- **Body**:
  ```json
  {
    "left": 0,
    "right": 0
  }
  ```

#### 设置父窗口
- **Endpoint**: `/taskbar/window/screen`
- **Method**: POST
- **Body**:
  ```json
  {
    "parent_taskbar": { "value": "Shell_TrayWnd" } // 任务栏窗口类名
  }
  ```

#### 关闭程序
关闭 C++ 后端程序。
- **Endpoint**: `/taskbar/close`
- **Method**: POST
- **Body**: `{}`

---

## 4. 数据流说明

系统内部的数据流是一个单向的管道：

1.  **Trigger (事件触发)**: 用户在网易云音乐切歌或进度改变。
2.  **Capture (捕获)**: `betterncm-plugin/lyric.js` 捕获事件，从 DOM 或 API 获取当前时间点的歌词文本。
3.  **Process (处理)**: 插件根据配置（如是否显示翻译、下一句位置）格式化歌词对象。
4.  **Transport (传输)**: `betterncm-plugin/api.js` 将 JSON 数据通过 HTTP POST 发送到本地服务器。
5.  **Receive (接收)**: `taskbar-lyrics/NetworkServer.cpp` 接收 HTTP 请求，解析 JSON，更新 `RenderWindow` 对象的成员变量。
6.  **Render (渲染)**: `NetworkServer` 发送 `WM_PAINT` 消息 -> `RenderWindow.cpp` 触发重绘 -> Direct2D 将新文本画在屏幕上。

---

## 5. 部署与构建指南

### 5.1 环境要求
- **操作系统**: Windows 10 / 11 (推荐 Windows 11)
- **编译环境**: Visual Studio 2022 (支持 C++17 或更高)

### 5.2 构建步骤
1.  **依赖说明**:
    本项目已移除 `cpp-httplib` 和 `nlohmann-json` 的外部依赖，改用原生 WinSock2 和简易 JSON 解析。

2.  **编译项目**:
    打开 `Taskbar Lyrics.sln`，选择 **Release** 配置和 **x86** 平台，点击生成。

3.  **部署**:
    - 前端插件：安装到 BetterNCM 插件目录。
    - 后端程序：可独立部署在任意位置，需手动启动或配置为系统服务。

---

## 6. 维护说明与已知问题

### 6.1 已知问题
- **端口冲突风险**: 默认端口 `27232`。如果被占用，需通过命令行参数更改。
- **任务栏兼容性**: 程序强依赖于 Windows 任务栏的窗口类名 (`Shell_TrayWnd`) 和结构。
- **高分屏适配**: 虽然使用了 Direct2D，但在不同 DPI 设置的多显示器环境下，窗口位置计算可能需要额外校准。

### 6.2 优化建议
- **渲染性能优化**: 目前每次收到消息（包括进度更新）都会触发全窗口重绘。可以引入脏矩形渲染或去抖动机制，仅重绘变化的区域，降低 GPU 占用。
- **多显示器/高 DPI 支持**: 完善在多显示器、不同 DPI 缩放比下的窗口位置计算和字体渲染逻辑。
- **配置持久化**: 考虑在后端增加简单的配置文件读写能力，以便在无前端连接时也能记住上次的窗口位置和样式。
- **国际化 (i18n)**: 为前端配置界面和后端提示信息添加多语言支持。
