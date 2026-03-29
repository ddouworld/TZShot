# TZshot

[English](./README.md) | [中文](./README_CN.md)

## 功能截图

| 标注贴图 | AI 图像编辑 |
| --- | --- |
| ![标注贴图](docs/images/sticky-annotate.png) | ![AI 图像编辑](docs/images/ai-edit.png) |

| AI 视觉理解 | 长截图 |
| --- | --- |
| ![AI 视觉理解](docs/images/ai-vision.png) | ![长截图](docs/images/long-capture.png) |

| OCR 结果 | GIF 录制 |
| --- | --- |
| ![OCR 结果](docs/images/ocr-result.png) | ![GIF 录制](docs/images/gif-record.png) |

## 开源许可

本项目采用 **GNU General Public License v3.0（GPL-3.0-only）** 开源。  
详见仓库根目录 [LICENSE](./LICENSE)。

## 项目简介

`TZshot` 是一个基于 **Qt 6 Widgets + C++** 的截图与贴图工具，支持多屏截图、框选标注、贴图编辑、长截图、OCR、GIF 录制、全局快捷键、系统托盘、AI 图像编辑和 AI 图像理解。

## 主要功能

- 多屏截图，支持虚拟桌面坐标与跨屏场景
- 截图浮层
  - 框选、拖拽、八方向调整选区
  - 放大镜取色预览
  - 回车执行默认动作，Esc 取消
- 标注工具
  - 矩形、圆形、箭头、画笔
  - 文字标注
  - 序号标注，支持自动递增
  - 高亮、马赛克、模糊
  - 撤销
- 截图结果操作
  - 复制到剪贴板
  - 保存到文件
  - 贴图到桌面
  - OCR 识别
  - 长截图
  - GIF 录制
- 贴图窗口
  - 拖拽、透明度调整
  - 缩放、旋转、镜像、1:1 恢复
  - 标注、OCR、右键菜单
  - AI 图像编辑与 AI 图像理解
- 长截图控制条与预览浮窗
- GIF 录制
- OCR 结果浮窗
- 全局快捷键
- 系统托盘菜单
- 中英文切换

## 技术栈

- C++17
- Qt 6
  - Core / Gui / Widgets / Network / Concurrent / LinguistTools
- CMake
- 全局快捷键
  - Windows: `RegisterHotKey`
  - Linux(X11): `xcb_grab_key`

## 当前架构

- `src/app/`
  - 应用启动、服务组装、Widget 运行时管理
- `src/widgets/`
  - 截图浮层、贴图窗口、设置窗口、OCR 结果窗、关于窗口等界面层
- `src/viewmodel/`
  - 截图、贴图、长截图、GIF、OCR、存储、AI 等业务逻辑
- `src/model/`
  - 桌面快照、应用设置
- `src/paint_board/shape/`
  - 各类标注图形实现
- `resource/`
  - 图标与 QSS 样式资源
- `i18n/`
  - 中英文翻译

## 目录结构

```text
TZshot/
├─ src/
│  ├─ app/
│  ├─ widgets/
│  ├─ model/
│  ├─ viewmodel/
│  ├─ paint_board/
│  ├─ shortcut_key/
│  ├─ ocr/
│  └─ ai_call/
├─ i18n/
├─ resource/
├─ thirdpart/
└─ CMakeLists.txt
```

## 环境要求

- CMake >= 3.16
- Qt 6.x
- C++17 编译器

Windows:
- MSVC 2019/2022

Linux:
- X11/xcb 相关开发库

## 构建与运行

### 方式一：Qt Creator（推荐）

1. 打开项目根目录 `CMakeLists.txt`
2. 选择 Qt 6 Kit
3. Configure -> Build -> Run

### 方式二：命令行

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Windows 运行：

```bash
./build/Release/TZshot.exe
```

Qt Creator + Ninja 常见路径：

```bash
./build/<kit-name>/TZshot.exe
```

Linux 运行：

```bash
./build/TZshot
```

## OCR 依赖说明（Windows）

- 首次配置或构建时，CMake 会自动拉取并构建 `Leptonica`、`Tesseract`
- 输出目录位于 `thirdpart/ocr-install`
- 项目默认不自动下载 `tessdata`
- 请手动准备并放置
  - `thirdpart/ocr-install/share/tessdata/chi_sim.traineddata`
  - `thirdpart/ocr-install/share/tessdata/eng.traineddata`

若缺少上述文件，运行时会出现 `Tesseract Init 失败：未找到可用 tessdata`。

## 默认快捷键

- `Alt + A`: 截图
- `Alt + S`: 截图并保存
- `Alt + P`: 贴图到桌面
- `Alt + Q`: 显示或隐藏窗口

## 配置项（QSettings）

- AI
  - `AI/apiKey`
  - `AI/selectedModel`
- Vision
  - `Vision/apiKey`
  - `Vision/provider`
  - `Vision/model`
  - `Vision/proxyEnabled`
  - `Vision/proxyType`
  - `Vision/proxyHost`
  - `Vision/proxyPort`
- 快捷键
  - `Shortcuts/screenshot`
  - `Shortcuts/screenshotSave`
  - `Shortcuts/sticky`
  - `Shortcuts/toggle`
- 通用
  - `App/language`
  - `ImageSaver/savePath`

## 已知说明

- Linux 全局快捷键目前基于 X11，暂不覆盖 Wayland 原生实现
- 置顶浮层与贴图窗口体验目前以 Windows 最完整
- GIF、OCR 与长截图仍可继续做跨平台增强

## 贡献与安全提示

- 请勿提交真实的 `API Key`、令牌或个人隐私数据
- 建议统一使用 `UTF-8（无 BOM）` 与 `LF`
- 修改第三方依赖时，请同步补充许可证说明


