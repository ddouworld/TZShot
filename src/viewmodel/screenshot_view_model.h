#ifndef SCREENSHOT_VIEW_MODEL_H
#define SCREENSHOT_VIEW_MODEL_H

#include <QColor>
#include <QObject>
#include <QQuickItem>
#include <QRect>
#include <QWindow>
#include "model/desktop_snapshot.h"
#include "model/app_settings.h"
#include "sticky_image_store.h"

class ScreenshotViewModel : public QObject
{
    Q_OBJECT

public:
    explicit ScreenshotViewModel(DesktopSnapshot  &snapshot,
                                 StickyImageStore &stickyStore,
                                 QObject          *parent = nullptr);

    const QPixmap& desktopSnapshot() const { return m_snapshot.image(); }
    Q_INVOKABLE QRect virtualGeometry() const { return m_snapshot.virtualGeometry(); }
    QPoint mapLogicalPointToPhysical(const QPoint &point) const
    {
        return m_snapshot.mapLogicalPointToPhysical(point);
    }
    Q_INVOKABLE QRect mapLogicalRectToPhysicalRect(const QRect &rect) const
    {
        return captureRectForDisplay(rect);
    }

    Q_INVOKABLE void captureDesktop();
    Q_INVOKABLE void releaseDesktopSnapshot();
    Q_INVOKABLE QColor getPixelColor(int x, int y) const;
    Q_INVOKABLE QRect windowAtPoint(int x, int y) const;
    Q_INVOKABLE bool captureRectToClipboard(QQuickItem *paintBoard, const QRect &rect);
    Q_INVOKABLE QString captureRectToBase64(QQuickItem *paintBoard, const QRect &rect);
    Q_INVOKABLE QString captureRectToStickyUrl(QQuickItem *paintBoard, const QRect &rect);
    Q_INVOKABLE void grabToPaintBoard(QQuickItem *paintBoard, const QRect &rect);
    Q_INVOKABLE QImage captureRectToImage(const QRect &rect);

    QImage captureRectToImageLive(const QRect &rect);
    bool copyImageToClipboard(const QImage &image);

private:
    QRect captureRectForDisplay(const QRect &rect) const;
    QImage captureScreen(QQuickItem *paintBoard,
                         const QRect &rect = QRect(),
                         bool setBackground = false);
    bool writeImageToClipboard(const QImage &image);

    DesktopSnapshot &m_snapshot;
    StickyImageStore &m_stickyStore;
};

#endif // SCREENSHOT_VIEW_MODEL_H
