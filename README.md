# TZshot

[English](./README.md) | [中文](./README_CN.md)

## Screenshots

| Sticky Annotate | AI Image Edit |
| --- | --- |
| ![Sticky Annotate](docs/images/sticky-annotate.png) | ![AI Image Edit](docs/images/ai-edit.png) |

| AI Vision | Long Capture |
| --- | --- |
| ![AI Vision](docs/images/ai-vision.png) | ![Long Capture](docs/images/long-capture.png) |

| OCR Result | GIF Record |
| --- | --- |
| ![OCR Result](docs/images/ocr-result.png) | ![GIF Record](docs/images/gif-record.png) |

## License

This project is licensed under **GNU General Public License v3.0 (GPL-3.0-only)**.  
See [LICENSE](./LICENSE).

## Overview

`TZshot` is a screenshot and pin-image utility built with **Qt 6 Widgets + C++**. It supports multi-screen capture, region annotation, sticky image editing, long screenshots, OCR, GIF recording, global shortcuts, tray integration, AI image editing, and AI visual understanding.

## Features

- Multi-screen capture with virtual desktop coordinates
- Capture overlay
  - Region selection, moving, and 8-direction resizing
  - Magnifier preview with pixel color readout
  - `Enter` performs the default action, `Esc` cancels
- Annotation tools
  - Rectangle, circle, arrow, freehand pen
  - Text annotation
  - Number annotation with auto-increment
  - Highlight, mosaic, blur
  - Undo
- Output actions
  - Copy to clipboard
  - Save to file
  - Pin to desktop
  - OCR
  - Long screenshot
  - GIF recording
- Sticky window
  - Dragging and opacity control
  - Zoom, rotate, mirror, reset to 1:1
  - Annotation, OCR, context menu actions
  - AI image editing and AI visual understanding
- Long screenshot toolbar with floating preview
- GIF recording
- OCR result window
- Global shortcuts
- System tray menu
- Chinese and English UI switching

## Tech Stack

- C++17
- Qt 6
  - Core / Gui / Widgets / Network / Concurrent / LinguistTools
- CMake
- Global shortcuts
  - Windows: `RegisterHotKey`
  - Linux(X11): `xcb_grab_key`

## Architecture

- `src/app/`
  - App bootstrap, service wiring, and widget runtime management
- `src/widgets/`
  - Capture overlay, sticky windows, settings dialog, OCR result window, about dialog, and related UI
- `src/viewmodel/`
  - Business logic for capture, sticky images, long screenshot, GIF, OCR, storage, and AI
- `src/model/`
  - Desktop snapshot and persisted app settings
- `src/paint_board/shape/`
  - Annotation shape implementations
- `resource/`
  - Icons and QSS resources
- `i18n/`
  - Chinese and English translations

## Project Structure

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

## Requirements

- CMake >= 3.16
- Qt 6.x
- C++17 compiler

Windows:
- MSVC 2019/2022

Linux:
- X11/xcb development libraries

## Build and Run

### Option 1: Qt Creator

1. Open the project root `CMakeLists.txt`
2. Select a Qt 6 kit
3. Configure -> Build -> Run

### Option 2: Command Line

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Run on Windows:

```bash
./build/Release/TZshot.exe
```

Typical Qt Creator + Ninja output path:

```bash
./build/<kit-name>/TZshot.exe
```

Run on Linux:

```bash
./build/TZshot
```

## OCR Notes on Windows

- On first configure or build, CMake will fetch and build `Leptonica` and `Tesseract`
- Output is placed under `thirdpart/ocr-install`
- This project does not auto-download `tessdata`
- Please prepare and place these files manually
  - `thirdpart/ocr-install/share/tessdata/chi_sim.traineddata`
  - `thirdpart/ocr-install/share/tessdata/eng.traineddata`

If they are missing, OCR will fail at runtime with `Tesseract Init failed: no usable tessdata found`.

## Default Shortcuts

- `Alt + A`: Screenshot
- `Alt + S`: Screenshot and Save
- `Alt + P`: Pin to Desktop
- `Alt + Q`: Show or Hide Window

## Configuration (QSettings)

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
- Shortcuts
  - `Shortcuts/screenshot`
  - `Shortcuts/screenshotSave`
  - `Shortcuts/sticky`
  - `Shortcuts/toggle`
- General
  - `App/language`
  - `ImageSaver/savePath`

## Notes

- Linux global shortcuts currently rely on X11 and do not yet cover native Wayland support
- Floating overlay and sticky window experience is currently most complete on Windows
- GIF, OCR, and long screenshot flows still have room for more cross-platform refinement

## Contributing and Security

- Do not commit real `API Key`s, tokens, or private data
- Use `UTF-8 (no BOM)` and `LF` line endings when possible
- When updating third-party dependencies, include their license notices

