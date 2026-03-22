# TZshot

[中文](#中文) | [English](#english)

---

## 中文

### 开源许可
本项目采用 **GNU General Public License v3.0（GPL-3.0-only）** 开源。  
详见仓库根目录 [LICENSE](./LICENSE)。

### 项目简介
`TZshot` 是一个基于 **Qt 6 + QML + C++** 的截图与贴图工具，支持多屏截图、标注、贴图编辑、OCR、GIF 录制、全局快捷键、系统托盘和 AI 图像编辑。

### 主要功能
- 多屏截图（虚拟桌面坐标、跨屏场景）
- 标注工具：
  - 矩形、圆形、箭头、画笔
  - 文字标注（文本内容、背景开关）
  - 序号标注（支持自动递增）
  - 高亮笔
  - 马赛克（可调强度）
  - 撤销
- 截图结果：
  - 复制到剪贴板
  - 保存到文件
  - 贴图到桌面
- 贴图窗口：
  - 拖拽、透明度调整
  - 缩放、旋转、镜像、1:1 恢复
  - 标注、OCR、右键菜单
  - 完成后复制、另存为/覆盖闭环
- GIF 录制（可调录制区域）
- OCR 识别结果浮窗
- 全局快捷键（可配置并持久化）
- 系统托盘菜单
- 中英文切换（切换后自动重启应用）
- AI 图像编辑（可配置模型与 API Key）

### 技术栈
- C++17
- Qt 6（Core / Gui / Widgets / Quick / Qml / Network / QuickDialogs2 / LinguistTools）
- CMake
- 全局快捷键：
  - Windows: `RegisterHotKey`
  - Linux(X11): `xcb_grab_key`

### 目录结构
```text
TZshot/
├─ src/
│  ├─ main.cpp
│  ├─ model/
│  ├─ viewmodel/
│  ├─ paint_board/
│  ├─ shortcut_key/
│  ├─ ocr/
│  └─ ai_call/
├─ qml/
├─ i18n/
├─ resource/
├─ thirdpart/
└─ CMakeLists.txt
```

### 环境要求
- CMake >= 3.16
- Qt 6.x
- C++17 编译器

Windows:
- MSVC 2019/2022（推荐使用 Qt Creator 对应 Kit）

Linux:
- X11/xcb 相关开发库

### 构建与运行
#### 方式一：Qt Creator（推荐）
1. 打开项目根目录 `CMakeLists.txt`
2. 选择 Qt 6 Kit
3. Configure -> Build -> Run

#### 方式二：命令行
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

运行（Windows）：
```bash
./build/Release/TZshot.exe
```

运行（Linux，路径以实际生成为准）：
```bash
./build/TZshot
```

### OCR 依赖说明（Windows）
- 首次配置/构建时，CMake 会自动从 GitHub 拉取并构建 `Leptonica`、`Tesseract`（输出到项目根目录 `thirdpart/ocr-install`）。
- 项目默认**不自动下载**语言数据文件（`tessdata`）。
- 请手动准备并放置：
  - `thirdpart/ocr-install/share/tessdata/chi_sim.traineddata`
  - `thirdpart/ocr-install/share/tessdata/eng.traineddata`
- 若缺少上述文件，运行时会出现 `Tesseract Init 失败：未找到可用 tessdata`。

### 默认快捷键
- `Alt + A`: 截图
- `Alt + S`: 截图并保存
- `Alt + P`: 贴图到桌面
- `Alt + Q`: 显示/隐藏窗口

### 配置项（QSettings）
- AI
  - `AI/apiKey`
  - `AI/selectedModel`
- 快捷键
  - `Shortcuts/screenshot`
  - `Shortcuts/screenshotSave`
  - `Shortcuts/sticky`
  - `Shortcuts/toggle`
- 通用
  - `App/language`（`zh_CN` / `en`）
  - `ImageSaver/savePath`

### 已知说明
- Linux 全局快捷键目前基于 X11（暂不覆盖 Wayland 原生实现）
- GIF 与部分平台能力仍可继续做跨平台增强

### 贡献与安全提示
- 请勿提交任何真实的 `API Key`、令牌或个人隐私数据。
- 提交前建议自查编码与换行：`UTF-8（无 BOM）` + `LF`。
- 若修改第三方依赖，请同步补充其许可证说明。

---

## English

### License
This project is licensed under **GNU General Public License v3.0 (GPL-3.0-only)**.  
See [LICENSE](./LICENSE) in the repository root.

### Overview
`TZshot` is a screenshot and pin-image utility built with **Qt 6 + QML + C++**.  
It supports multi-screen capture, rich annotations, sticky image editing, OCR, GIF recording, global shortcuts, tray integration, and AI image editing.

### Key Features
- Multi-screen capture (virtual desktop coordinates)
- Annotation tools:
  - Rectangle, circle, arrow, freehand pen
  - Text annotation (text content + background toggle)
  - Number annotation (auto-increment supported)
  - Highlighter
  - Mosaic (adjustable intensity)
  - Undo
- Output actions:
  - Copy to clipboard
  - Save to file
  - Pin to desktop (sticky window)
- Sticky window:
  - Drag, opacity control
  - Zoom, rotate, mirror, reset-to-1:1
  - Annotate, OCR, context menu actions
  - Apply-and-copy / save-as / overwrite workflow
- GIF recording with adjustable capture area
- OCR result window
- Configurable global shortcuts (persisted)
- System tray menu
- Chinese/English UI switching (app restarts after switching)
- AI image editing with configurable model/API key

### Tech Stack
- C++17
- Qt 6 (Core / Gui / Widgets / Quick / Qml / Network / QuickDialogs2 / LinguistTools)
- CMake
- Global shortcuts:
  - Windows: `RegisterHotKey`
  - Linux(X11): `xcb_grab_key`

### Project Structure
```text
TZshot/
├─ src/
├─ qml/
├─ i18n/
├─ resource/
├─ thirdpart/
└─ CMakeLists.txt
```

### Requirements
- CMake >= 3.16
- Qt 6.x
- C++17 compiler

Windows:
- MSVC 2019/2022 (Qt Creator kit recommended)

Linux:
- X11/xcb development libraries

### Build & Run
#### Option 1: Qt Creator (Recommended)
1. Open `CMakeLists.txt` in project root
2. Select a Qt 6 kit
3. Configure -> Build -> Run

#### Option 2: Command Line
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run on Windows:
```bash
./build/Release/TZshot.exe
```

Run on Linux (path may vary by generator):
```bash
./build/TZshot
```

### OCR Notes (Windows)
- On first configure/build, CMake auto-fetches and builds `Leptonica` and `Tesseract` from GitHub into `thirdpart/ocr-install` (project root).
- This project does **not** auto-download language data (`tessdata`).
- Please add these files manually:
  - `thirdpart/ocr-install/share/tessdata/chi_sim.traineddata`
  - `thirdpart/ocr-install/share/tessdata/eng.traineddata`
- If they are missing, OCR will fail at runtime with `Tesseract Init failed: no usable tessdata found`.

### Default Shortcuts
- `Alt + A`: Screenshot
- `Alt + S`: Screenshot and Save
- `Alt + P`: Pin to Desktop
- `Alt + Q`: Show/Hide Window

### Configuration (QSettings)
- AI
  - `AI/apiKey`
  - `AI/selectedModel`
- Shortcuts
  - `Shortcuts/screenshot`
  - `Shortcuts/screenshotSave`
  - `Shortcuts/sticky`
  - `Shortcuts/toggle`
- General
  - `App/language` (`zh_CN` / `en`)
  - `ImageSaver/savePath`

### Notes
- Linux global shortcuts currently rely on X11 (no native Wayland implementation yet)
- GIF and some platform-dependent capabilities can be further improved cross-platform

### Contributing & Security
- Do not commit real API keys, tokens, or private data.
- Keep source files in `UTF-8 (no BOM)` with `LF` line endings.
- When adding/updating third-party dependencies, include their license notices.
