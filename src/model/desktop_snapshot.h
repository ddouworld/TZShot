#ifndef DESKTOP_SNAPSHOT_H
#define DESKTOP_SNAPSHOT_H

#include <QImage>
#include <QRect>
#include <QVector>
#include <QColor>

// DesktopSnapshot — Model
// 负责全屏快照的采集与存储，以及顶层窗口列表的枚举。
// 支持多屏：将所有屏幕的快照拼合为一张以虚拟桌面坐标系为基准的大图。
// 不依赖 QML，不继承 QObject，纯数据/系统调用。

class DesktopSnapshot
{
public:
    DesktopSnapshot() = default;

    // 抓取所有屏幕快照并拼合，枚举顶层窗口列表（含 50ms 延时等待合成器）
    void grab();
    // 释放快照和窗口列表
    void release();

    bool isNull() const { return m_snapshot.isNull(); }

    // 拼合后的虚拟桌面快照（物理像素坐标系）
    const QImage& image() const { return m_snapshot; }

    // 虚拟桌面的逻辑坐标矩形（所有屏幕的 geometry 联合，逻辑像素）
    QRect virtualGeometry() const { return m_virtualGeometry; }

    // 将逻辑坐标 (x,y) 转换为快照中的颜色（支持多屏 DPI 混合）
    QColor pixelColor(int logicalX, int logicalY) const;

    // 返回包含逻辑坐标点 (x,y) 的最上层窗口矩形（逻辑坐标）
    QRect windowAtPoint(int x, int y) const;

private:
    void buildWindowList();

    QImage         m_snapshot;
    QRect          m_virtualGeometry;  // 虚拟桌面逻辑矩形（所有屏幕 geometry 的联合）
    QVector<QRect> m_windowRects;      // 按 Z 轴从顶到底排列（逻辑坐标）
};

#endif // DESKTOP_SNAPSHOT_H
