#include "sticky_view_model.h"

#include "ai_view_model.h"
#include "widgets/sticky_pin_widget.h"

#include <QGuiApplication>
#include <QPainter>
#include <QScreen>
#include <QTransform>
#include <QWindow>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

StickyViewModel::StickyViewModel(StickyImageStore &store, QObject *parent)
    : QObject(parent)
    , m_store(store)
{
}

void StickyViewModel::setAiViewModel(AIViewModel *aiViewModel)
{
    m_aiViewModel = aiViewModel;
}

void StickyViewModel::requestSticky(const QString &imageUrl, const QRect &imgRect)
{
    if (imageUrl.isEmpty()) {
        return;
    }

    QRect targetRect = imgRect.normalized();
#ifdef Q_OS_WIN
    const QImage image = m_store.getImageByUrl(imageUrl);
    if (!image.isNull()) {
        auto *widget = new StickyPinWidget(imageUrl, targetRect, image, &m_store, m_aiViewModel);
        Q_UNUSED(widget);
        return;
    }
#endif
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

#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(window->winId())) {
        SetWindowPos(hwnd,
                     HWND_TOPMOST,
                     topLeft.x(),
                     topLeft.y(),
                     0,
                     0,
                     SWP_NOSIZE | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        return;
    }
#endif

    QScreen *screen = QGuiApplication::screenAt(topLeft);
    if (!screen) {
        screen = QGuiApplication::screenAt(targetRect.center());
    }
    if (!screen) {
        screen = QGuiApplication::primaryScreen();
    }

    if (screen && window->screen() != screen) {
        window->setScreen(screen);
    }

    window->setPosition(topLeft);
}

void StickyViewModel::resizeStickyWindow(QObject *windowObject, int physicalWidth, int physicalHeight) const
{
    auto *window = qobject_cast<QWindow*>(windowObject);
    if (!window) {
        return;
    }

    const int targetWidth = qMax(1, physicalWidth);
    const int targetHeight = qMax(1, physicalHeight);

#ifdef Q_OS_WIN
    if (HWND hwnd = reinterpret_cast<HWND>(window->winId())) {
        SetWindowPos(hwnd,
                     nullptr,
                     0,
                     0,
                     targetWidth,
                     targetHeight,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOOWNERZORDER);
        return;
    }
#endif

    window->resize(targetWidth, targetHeight);
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

QSize StickyViewModel::getImageSizeByUrl(const QString &imageUrl) const
{
    const QImage image = m_store.getImageByUrl(imageUrl);
    if (image.isNull()) {
        return {};
    }
    return image.size();
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
