#include "sticky_view_model.h"
#include <QPainter>
#include <QTransform>
#include <QGuiApplication>
#include <QScreen>
#include <QWindow>
#include <QDebug>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

StickyViewModel::StickyViewModel(StickyImageStore &store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
}

void StickyViewModel::requestSticky(const QString &imageUrl, const QRect &imgRect)
{
    if (imageUrl.isEmpty()) {
        return;
    }

    QRect targetRect = imgRect.normalized();
    QImage image = m_store.getImageByUrl(imageUrl);
    if (!image.isNull()) {
        QScreen *screen = QGuiApplication::screenAt(targetRect.topLeft());
        if (!screen) {
            screen = QGuiApplication::screenAt(targetRect.center());
        }
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }

        const qreal dpr = screen ? screen->devicePixelRatio() : 1.0;
        if (dpr > 0.0) {
            if (!qFuzzyCompare(image.devicePixelRatio(), dpr)) {
                image.setDevicePixelRatio(dpr);
                m_store.replaceImage(imageUrl, image);
            }
            const QSize logicalSize(qMax(1, qRound(image.width() / dpr)),
                                    qMax(1, qRound(image.height() / dpr)));
            targetRect.setSize(logicalSize);
        }
    }

    emit stickyReady(imageUrl, targetRect);
}

void StickyViewModel::releaseImage(const QString &imageUrl)
{
    m_store.releaseImage(imageUrl);
}

void StickyViewModel::positionStickyWindow(QObject *windowObject, const QRect &imgRect) const
{
    auto *window = qobject_cast<QWindow*>(windowObject);
    if (!window) {
        return;
    }

    const QRect targetRect = imgRect.normalized();
    const QPoint topLeft = targetRect.topLeft();

    window->setPosition(topLeft);

#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(window->winId())) {
        RECT rect{};
        GetWindowRect(hwnd, &rect);
        qDebug() << "[StickyHWND] qt-pos:"
                 << topLeft
                 << "hwnd:"
                 << QRect(rect.left,
                          rect.top,
                          rect.right - rect.left,
                          rect.bottom - rect.top)
                 << "qmlPos:" << window->x() << window->y();
    }
#endif
}

bool StickyViewModel::saveImage(const QString &imageUrl, const QUrl &targetUrl)
{
    return m_store.saveImage(imageUrl, targetUrl);
}

bool StickyViewModel::copyImageToClipboard(const QString &imageUrl)
{
    return m_store.copyImageToClipboard(imageUrl);
}

QImage StickyViewModel::getImageByUrl(const QString &imageUrl) const
{
    return m_store.getImageByUrl(imageUrl);
}

QImage StickyViewModel::mergeLayers(const QString &imageUrl, const QImage &annotationLayer) const
{
    QImage base = m_store.getImageByUrl(imageUrl);
    if (base.isNull()) {
        return {};
    }
    if (annotationLayer.isNull()) {
        return base;
    }

    QImage merged = base.convertToFormat(QImage::Format_ARGB32);
    QPainter painter(&merged);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.drawImage(0, 0, annotationLayer);
    painter.end();
    return merged;
}

bool StickyViewModel::overwriteWithAnnotations(const QString &imageUrl, const QImage &annotationLayer)
{
    const QImage merged = mergeLayers(imageUrl, annotationLayer);
    if (merged.isNull()) {
        return false;
    }
    return m_store.replaceImage(imageUrl, merged);
}

bool StickyViewModel::saveMergedImage(const QString &imageUrl, const QImage &annotationLayer, const QUrl &targetUrl)
{
    if (targetUrl.isEmpty() || !targetUrl.isLocalFile()) {
        return false;
    }
    const QImage merged = mergeLayers(imageUrl, annotationLayer);
    if (merged.isNull()) {
        return false;
    }
    return merged.save(targetUrl.toLocalFile());
}

bool StickyViewModel::rotateImage(const QString &imageUrl, int degreesClockwise)
{
    QImage img = m_store.getImageByUrl(imageUrl);
    if (img.isNull()) {
        return false;
    }

    int normalized = degreesClockwise % 360;
    if (normalized < 0) {
        normalized += 360;
    }
    if (normalized % 90 != 0) {
        return false;
    }

    if (normalized != 0) {
        QTransform t;
        t.rotate(normalized);
        img = img.transformed(t, Qt::SmoothTransformation);
    }
    return m_store.replaceImage(imageUrl, img);
}

bool StickyViewModel::mirrorImage(const QString &imageUrl, bool horizontal, bool vertical)
{
    QImage img = m_store.getImageByUrl(imageUrl);
    if (img.isNull()) {
        return false;
    }
    if (!horizontal && !vertical) {
        return true;
    }
    img = img.mirrored(horizontal, vertical);
    return m_store.replaceImage(imageUrl, img);
}
