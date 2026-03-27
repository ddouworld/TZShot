#include "screenshot_view_model.h"
#include "widgets/sticky_canvas_widget.h"

#include <QBuffer>
#include <QClipboard>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <QWidget>
#include <QWindow>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellscalingapi.h>
#endif

namespace {

qreal captureDprForRect(const QRect& rect)
{
#ifdef Q_OS_WIN
    const POINT centerPoint {
        rect.center().x(),
        rect.center().y()
    };
    if (HMONITOR monitor = MonitorFromPoint(centerPoint, MONITOR_DEFAULTTONEAREST)) {
        using GetDpiForMonitorFn = HRESULT (WINAPI *)(HMONITOR, MONITOR_DPI_TYPE, UINT *, UINT *);
        static const auto getDpiForMonitor = reinterpret_cast<GetDpiForMonitorFn>(
            GetProcAddress(LoadLibraryW(L"Shcore.dll"), "GetDpiForMonitor"));
        if (getDpiForMonitor) {
            UINT dpiX = 0;
            UINT dpiY = 0;
            if (SUCCEEDED(getDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)) && dpiX > 0) {
                return qreal(dpiX) / 96.0;
            }
        }
    }
#endif
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        return screen->devicePixelRatio();
    }
    return 1.0;
}

qreal effectiveCaptureDpr(StickyCanvasWidget *paintBoard, const QRect &rect)
{
    if (!rect.isEmpty()) {
        return captureDprForRect(rect);
    }
    if (paintBoard) {
        if (QWidget *topLevel = paintBoard->window()) {
            if (QWindow *handle = topLevel->windowHandle()) {
                return handle->devicePixelRatio();
            }
        }
    }
    if (QScreen *screen = QGuiApplication::primaryScreen()) {
        return screen->devicePixelRatio();
    }
    return 1.0;
}

QRect captureRectInSnapshot(const DesktopSnapshot& snapshot, const QRect& rect, qreal dpr)
{
    if (snapshot.isNull()) {
        return {};
    }

    if (rect.isEmpty()) {
        return snapshot.image().rect();
    }

#ifdef Q_OS_WIN
    Q_UNUSED(dpr);
    const QRect localRect = rect.translated(-snapshot.virtualGeometry().topLeft());
    return localRect.intersected(snapshot.image().rect());
#else
    const QRect localRect = rect.translated(-snapshot.virtualGeometry().topLeft());
    const QPoint topLeft = QPoint(qRound(localRect.left() * dpr),
                                  qRound(localRect.top() * dpr));
    const QPoint bottomRight = QPoint(qRound(localRect.right() * dpr),
                                      qRound(localRect.bottom() * dpr));
    QRect target;
    target.setCoords(topLeft.x(), topLeft.y(), bottomRight.x(), bottomRight.y());
    return target.intersected(snapshot.image().rect());
#endif
}

}

ScreenshotViewModel::ScreenshotViewModel(DesktopSnapshot  &snapshot,
                                         StickyImageStore &stickyStore,
                                         QObject          *parent)
    : QObject(parent)
    , m_snapshot(snapshot)
    , m_stickyStore(stickyStore)
{
}

QRect ScreenshotViewModel::captureRectForDisplay(const QRect &rect) const
{
    const QRect r = rect.normalized();
    if (r.isEmpty() || m_snapshot.isNull()) {
        return {};
    }
    return captureRectInSnapshot(m_snapshot, r, captureDprForRect(r));
}

void ScreenshotViewModel::captureDesktop()
{
    m_snapshot.grab();
}

void ScreenshotViewModel::releaseDesktopSnapshot()
{
    m_snapshot.release();
}

QColor ScreenshotViewModel::getPixelColor(int x, int y) const
{
    return m_snapshot.pixelColor(x, y);
}

QRect ScreenshotViewModel::windowAtPoint(int x, int y) const
{
    return m_snapshot.windowAtPoint(x, y);
}

bool ScreenshotViewModel::captureRectToClipboard(StickyCanvasWidget *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] capture area is empty";
        return false;
    }

    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] failed to capture image";
        return false;
    }

    return writeImageToClipboard(img);
}

QString ScreenshotViewModel::captureRectToBase64(StickyCanvasWidget *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] capture area is empty";
        return {};
    }

    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] failed to capture image";
        return {};
    }

    QByteArray data;
    QBuffer buf(&data);
    buf.open(QIODevice::WriteOnly);
    if (!img.save(&buf, "PNG")) {
        qWarning() << "[ScreenshotViewModel] failed to encode PNG";
        return {};
    }
    return data.toBase64();
}

QString ScreenshotViewModel::captureRectToStickyUrl(StickyCanvasWidget *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] sticky area is empty";
        return {};
    }

    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] failed to capture sticky image";
        return {};
    }

    const qreal dpr = effectiveCaptureDpr(paintBoard, r);
    if (dpr > 0.0) {
        img.setDevicePixelRatio(dpr);
    }

    return m_stickyStore.storeImage(img);
}

QImage ScreenshotViewModel::captureRectToImage(const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] OCR area is empty";
        return {};
    }

    if (!m_snapshot.isNull()) {
        const QRect captureRect = captureRectInSnapshot(m_snapshot, r, captureDprForRect(r));
        return m_snapshot.image().copy(captureRect).toImage();
    }

    qWarning() << "[ScreenshotViewModel] snapshot unavailable, falling back to primary screen";
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) {
        return {};
    }
    return screen->grabWindow(0, r.x(), r.y(), r.width(), r.height()).toImage();
}

QImage ScreenshotViewModel::captureRectToImageLive(const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        return {};
    }

    m_snapshot.grab();
    return captureRectToImage(r);
}

bool ScreenshotViewModel::copyImageToClipboard(const QImage &image)
{
    return writeImageToClipboard(image);
}

QImage ScreenshotViewModel::captureScreen(StickyCanvasWidget *paintBoard,
                                          const QRect &rect,
                                          bool setBackground)
{
    QImage screenshot;

    if (!m_snapshot.isNull()) {
        const qreal dpr = effectiveCaptureDpr(paintBoard, rect);
        const QRect captureRect = captureRectInSnapshot(m_snapshot, rect, dpr);
        screenshot = m_snapshot.image().copy(captureRect).toImage();
    } else {
        qWarning() << "[ScreenshotViewModel] snapshot unavailable, falling back to primary screen";
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return {};
        }
        screenshot = screen->grabWindow(0).toImage();
    }

    if (paintBoard) {
        const QImage composited = paintBoard->compositedImage();
        if (!composited.isNull()) {
            if (!rect.isEmpty()) {
                const QRect localRect = rect.translated(-m_snapshot.virtualGeometry().topLeft())
                                             .intersected(composited.rect());
                if (!localRect.isEmpty()) {
                    screenshot = composited.copy(localRect);
                }
            } else {
                screenshot = composited;
            }
        }

        if (setBackground) {
            paintBoard->setBackgroundImage(screenshot);
        }
    }

    return screenshot;
}

bool ScreenshotViewModel::writeImageToClipboard(const QImage &image)
{
    if (image.isNull()) {
        return false;
    }

    QClipboard *cb = QGuiApplication::clipboard();
    if (!cb) {
        return false;
    }

    cb->setImage(image, QClipboard::Clipboard);
#ifdef Q_OS_LINUX
    cb->setImage(image, QClipboard::Selection);
#endif
    return true;
}
