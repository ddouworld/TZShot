#include "magnifier_provider.h"

#include <QPainter>

MagnifierProvider::MagnifierProvider(ScreenshotViewModel *viewModel)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_viewModel(viewModel)
{
}

QImage MagnifierProvider::requestImage(const QString &id,
                                       QSize *size,
                                       const QSize & /*requestedSize*/)
{
    const QString cleanId = id.section('?', 0, 0);
    const QStringList parts = cleanId.split(',');
    if (parts.size() < 2) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) {
            *size = blank.size();
        }
        return blank;
    }

    bool okX = false;
    bool okY = false;
    const int logicalX = parts[0].toInt(&okX);
    const int logicalY = parts[1].toInt(&okY);
    if (!okX || !okY) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) {
            *size = blank.size();
        }
        return blank;
    }

    const int srcDiameter = kSrcRadius * 2;
    QImage region(srcDiameter, srcDiameter, QImage::Format_ARGB32_Premultiplied);
    region.fill(Qt::black);

    const QPixmap snapshot = m_viewModel->desktopSnapshot();
    if (snapshot.isNull()) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) {
            *size = blank.size();
        }
        return blank;
    }

    const QRect virtualRect = m_viewModel->virtualGeometry();
    const QPoint center(logicalX - virtualRect.x(), logicalY - virtualRect.y());
    QRect srcRect(center.x() - kSrcRadius,
                  center.y() - kSrcRadius,
                  srcDiameter,
                  srcDiameter);

    QPainter regionPainter(&region);
    const QRect bounded = srcRect.intersected(snapshot.rect());
    if (!bounded.isEmpty()) {
        const int destX = bounded.x() - srcRect.x();
        const int destY = bounded.y() - srcRect.y();
        regionPainter.drawPixmap(destX,
                                 destY,
                                 snapshot,
                                 bounded.x(),
                                 bounded.y(),
                                 bounded.width(),
                                 bounded.height());
    }
    regionPainter.end();

    QImage zoomed = region.scaled(kOutputSize,
                                  kOutputSize,
                                  Qt::IgnoreAspectRatio,
                                  Qt::SmoothTransformation);

    QPainter p(&zoomed);
    p.setRenderHint(QPainter::Antialiasing, false);
    p.setPen(QPen(QColor(255, 0, 0, 220), 1));
    const int half = kOutputSize / 2;
    p.drawLine(half, 0, half, kOutputSize - 1);
    p.drawLine(0, half, kOutputSize - 1, half);
    p.end();

    if (size) {
        *size = zoomed.size();
    }
    return zoomed;
}
