#include "screenshot_view_model.h"
#include "paint_board/paint_board.h"

#include <QBuffer>
#include <QClipboard>
#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QDebug>
#include <QQuickWindow>

ScreenshotViewModel::ScreenshotViewModel(DesktopSnapshot  &snapshot,
                                         StickyImageStore &stickyStore,
                                         QObject          *parent)
    : QObject(parent)
    , m_snapshot(snapshot)
    , m_stickyStore(stickyStore)
{
}

// ── captureDesktop ────────────────────────────────────────
void ScreenshotViewModel::captureDesktop()
{
    m_snapshot.grab();
}

// ── releaseDesktopSnapshot ────────────────────────────────
void ScreenshotViewModel::releaseDesktopSnapshot()
{
    m_snapshot.release();
}

// ── getPixelColor ─────────────────────────────────────────
QColor ScreenshotViewModel::getPixelColor(int x, int y) const
{
    return m_snapshot.pixelColor(x, y);
}

// ── windowAtPoint ─────────────────────────────────────────
QRect ScreenshotViewModel::windowAtPoint(int x, int y) const
{
    return m_snapshot.windowAtPoint(x, y);
}

// ── captureRectToClipboard ────────────────────────────────
bool ScreenshotViewModel::captureRectToClipboard(QQuickItem *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] 截图区域为空";
        return false;
    }
    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] 截图失败，捕获到空图片";
        return false;
    }
    return writeImageToClipboard(img);
}

// ── captureRectToBase64 ───────────────────────────────────
QString ScreenshotViewModel::captureRectToBase64(QQuickItem *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] 截图区域为空";
        return {};
    }
    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] 截图失败，捕获到空图片";
        return {};
    }
    QByteArray data;
    QBuffer    buf(&data);
    buf.open(QIODevice::WriteOnly);
    if (!img.save(&buf, "PNG")) {
        qWarning() << "[ScreenshotViewModel] PNG 编码失败";
        return {};
    }
    return data.toBase64();
}

// ── captureRectToStickyUrl ────────────────────────────────
QString ScreenshotViewModel::captureRectToStickyUrl(QQuickItem *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] 贴图区域为空";
        return {};
    }
    QImage img = captureScreen(paintBoard, r);
    if (img.isNull()) {
        qWarning() << "[ScreenshotViewModel] 贴图截图失败";
        return {};
    }
    return m_stickyStore.storeImage(img);
}

// ── grabToPaintBoard ──────────────────────────────────────
void ScreenshotViewModel::grabToPaintBoard(QQuickItem *paintBoard, const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) return;
    captureScreen(paintBoard, r, /*setBackground=*/true);
}

// ── captureRectToImage ────────────────────────────────────
QImage ScreenshotViewModel::captureRectToImage(const QRect &rect)
{
    const QRect r = rect.normalized();
    if (r.isEmpty()) {
        qWarning() << "[ScreenshotViewModel] OCR 区域为空";
        return {};
    }

    if (!m_snapshot.isNull()) {
        const QRect vg = m_snapshot.virtualGeometry();
        const qreal scaleX = vg.width()  > 0 ? qreal(m_snapshot.image().width())  / qreal(vg.width())  : 1.0;
        const qreal scaleY = vg.height() > 0 ? qreal(m_snapshot.image().height()) / qreal(vg.height()) : 1.0;

        const QRect physRect = QRect(qRound((r.x() - vg.x()) * scaleX),
                                     qRound((r.y() - vg.y()) * scaleY),
                                     qRound(r.width()  * scaleX),
                                     qRound(r.height() * scaleY))
                                  .intersected(m_snapshot.image().rect());
        return m_snapshot.image().copy(physRect);
    }

    qWarning() << "[ScreenshotViewModel] 快照未缓存，OCR 回退到实时抓屏（仅主屏）";
    QScreen *screen = QGuiApplication::primaryScreen();
    if (!screen) return {};
    return screen->grabWindow(0, r.x(), r.y(), r.width(), r.height()).toImage();
}

// ── captureScreen (private) ───────────────────────────────
QImage ScreenshotViewModel::captureScreen(QQuickItem *paintBoard,
                                          const QRect &rect,
                                          bool setBackground)
{
    PaintBoard *board = qobject_cast<PaintBoard*>(paintBoard);

    QImage screenshot;

    if (!m_snapshot.isNull()) {
        // 快照坐标系：逻辑坐标相对于虚拟桌面原点；物理像素按统一 DPR 缩放。
        // 将逻辑选区映射到快照物理像素矩形后裁剪。
        const QRect vg = m_snapshot.virtualGeometry();
        const qreal scaleX = vg.width()  > 0 ? qreal(m_snapshot.image().width())  / qreal(vg.width())  : 1.0;
        const qreal scaleY = vg.height() > 0 ? qreal(m_snapshot.image().height()) / qreal(vg.height()) : 1.0;

        const QRect physRect = rect.isEmpty()
            ? m_snapshot.image().rect()
            : QRect(qRound((rect.x() - vg.x()) * scaleX),
                    qRound((rect.y() - vg.y()) * scaleY),
                    qRound(rect.width()  * scaleX),
                    qRound(rect.height() * scaleY))
                  .intersected(m_snapshot.image().rect());

        screenshot = m_snapshot.image().copy(physRect);
    } else {
        qWarning() << "[ScreenshotViewModel] 快照未缓存，回退到实时抓屏（仅主屏）";
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) return {};
        screenshot = screen->grabWindow(0).toImage();
    }

    if (board) {
        const QImage annotations = board->renderToImage();
        if (!annotations.isNull()) {
            QPainter p(&screenshot);

            // PaintBoard 现在是全屏画布：导出选区时，需要从全屏标注层裁剪对应区域再叠加
            if (!rect.isEmpty()) {
                QQuickWindow *win = board->window();
                const int windowAbsX = win ? win->x() : 0;
                const int windowAbsY = win ? win->y() : 0;
                const int boardAbsX  = windowAbsX + qRound(board->x());
                const int boardAbsY  = windowAbsY + qRound(board->y());

                const int wantX = rect.x() - boardAbsX;
                const int wantY = rect.y() - boardAbsY;
                QRect srcRect(wantX, wantY, screenshot.width(), screenshot.height());
                srcRect = srcRect.intersected(annotations.rect());

                if (!srcRect.isEmpty()) {
                    const int dstX = srcRect.x() - wantX;
                    const int dstY = srcRect.y() - wantY;
                    p.drawImage(QPoint(dstX, dstY), annotations, srcRect);
                }
            } else {
                p.drawImage(0, 0, annotations);
            }
        }

        if (setBackground)
            board->setBackgroundImg(screenshot);
    }

    return screenshot;
}

// ── writeImageToClipboard (private) ──────────────────────
bool ScreenshotViewModel::writeImageToClipboard(const QImage &image)
{
    if (image.isNull()) return false;
    QClipboard *cb = QGuiApplication::clipboard();
    if (!cb) return false;
    cb->setImage(image, QClipboard::Clipboard);
#ifdef Q_OS_LINUX
    cb->setImage(image, QClipboard::Selection);
#endif
    return true;
}
