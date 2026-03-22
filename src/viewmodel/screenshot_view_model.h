#ifndef SCREENSHOT_VIEW_MODEL_H
#define SCREENSHOT_VIEW_MODEL_H

#include <QObject>
#include <QRect>
#include <QColor>
#include <QQuickItem>
#include "model/desktop_snapshot.h"
#include "model/app_settings.h"
#include "sticky_image_store.h"

// ScreenshotViewModel
// -------------------
// 编排截图完整流程：抓快照 → 裁剪 → 合并标注 → 输出（剪贴板/Base64/贴图）。
// 暴露 Q_INVOKABLE 方法和信号供 QML 调用，自身不感知任何 QML 组件 id。
// 依赖的 Model：DesktopSnapshot（数据）、StickyImageStore（贴图存储）。

class ScreenshotViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotViewModel(DesktopSnapshot  &snapshot,
                                  StickyImageStore &stickyStore,
                                  QObject          *parent = nullptr);

    // ── 供 MagnifierProvider 只读访问快照 ────────────────
    const QImage& desktopSnapshot() const { return m_snapshot.image(); }

    // 虚拟桌面的逻辑矩形（所有屏幕 geometry 的并集），供 QML 定位截图窗口
    Q_INVOKABLE QRect virtualGeometry() const { return m_snapshot.virtualGeometry(); }

    // ── Q_INVOKABLE：供 QML 调用 ─────────────────────────

    // 抓取全屏快照（在截图窗口显示之前调用）
    Q_INVOKABLE void captureDesktop();

    // 释放快照（截图会话结束后调用）
    Q_INVOKABLE void releaseDesktopSnapshot();

    // 取指定逻辑坐标的像素颜色（放大镜取色）
    Q_INVOKABLE QColor getPixelColor(int x, int y) const;

    // 返回 (x,y) 下最上层窗口矩形（窗口识别）
    Q_INVOKABLE QRect windowAtPoint(int x, int y) const;

    // 截图选区并写入剪贴板，返回是否成功
    Q_INVOKABLE bool captureRectToClipboard(QQuickItem *paintBoard, const QRect &rect);

    // 截图选区并返回 Base64 字符串（供保存文件使用，不写剪贴板）
    Q_INVOKABLE QString captureRectToBase64(QQuickItem *paintBoard, const QRect &rect);

    // 截图选区并存入 StickyImageStore，返回 image://sticky/<id> URL
    Q_INVOKABLE QString captureRectToStickyUrl(QQuickItem *paintBoard, const QRect &rect);

    // 仅将选区写入 PaintBoard 背景（马赛克工具使用）
    Q_INVOKABLE void grabToPaintBoard(QQuickItem *paintBoard, const QRect &rect);

    // 截图选区并直接返回 QImage（OCR 使用）
    Q_INVOKABLE QImage captureRectToImage(const QRect &rect);

private:
    // 内部：从快照裁剪选区并叠加 PaintBoard 标注层
    // setBackground=true 时将结果写回 PaintBoard 背景（仅马赛克路径）
    QImage captureScreen(QQuickItem *paintBoard, const QRect &rect = QRect(),
                         bool setBackground = false);

    bool writeImageToClipboard(const QImage &image);

    DesktopSnapshot  &m_snapshot;
    StickyImageStore &m_stickyStore;
};

#endif // SCREENSHOT_VIEW_MODEL_H
