#ifndef DESKTOP_SNAPSHOT_H
#define DESKTOP_SNAPSHOT_H

#include <QColor>
#include <QPoint>
#include <QPixmap>
#include <QRect>
#include <QVector>

class DesktopSnapshot
{
public:
    DesktopSnapshot() = default;

    void grab();
    void release();

    bool isNull() const { return m_snapshot.isNull(); }
    const QPixmap& image() const { return m_snapshot; }
    QRect virtualGeometry() const { return m_virtualGeometry; }

    QColor pixelColor(int logicalX, int logicalY) const;
    QPoint mapLogicalPointToPhysical(const QPoint &logicalPoint) const;
    QRect mapLogicalRectToPhysical(const QRect &logicalRect) const;
    QRect windowAtPoint(int x, int y) const;

private:
    struct ScreenEntry {
        QRect logicalGeometry;
        QRect physicalRect;
        qreal dpr = 1.0;
    };

    void buildScreenEntries();
    void buildWindowList();
    const ScreenEntry* screenEntryForPoint(const QPoint &logicalPoint) const;

    QPixmap m_snapshot;
    QRect m_virtualGeometry;
    QVector<QRect> m_windowRects;
    QVector<ScreenEntry> m_screenEntries;
};

#endif // DESKTOP_SNAPSHOT_H
