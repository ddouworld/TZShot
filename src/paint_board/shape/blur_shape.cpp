#include "blur_shape.h"

#include <QPainter>
#include <QPainterPath>

BlurShape::BlurShape(const QPoint &startPoint, int brushRadius, int blurRadius)
    : Shape(Qt::transparent, blurRadius)
    , m_brushRadius(qMax(4, brushRadius))
    , m_blurRadius(qMax(2, blurRadius))
{
    m_points.append(startPoint);
}

void BlurShape::setEndPoint(const QPoint &point)
{
    if (!m_points.isEmpty() && m_points.last() == point) {
        return;
    }
    m_points.append(point);
    applyBlurAt(point);
}

void BlurShape::draw(QPainter *painter)
{
    if (!painter || m_patches.isEmpty()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const BlurPatch &patch : m_patches) {
        QPainterPath path;
        path.addEllipse(QPointF(patch.center), m_brushRadius, m_brushRadius);
        painter->save();
        painter->setClipPath(path);
        painter->drawImage(patch.rect.topLeft(), patch.image);
        painter->restore();
    }
    painter->restore();
}

void BlurShape::setCanvasSnapshot(const QImage &snapshot)
{
    if (snapshot.isNull()) {
        return;
    }
    m_canvasSnapshot = snapshot.convertToFormat(QImage::Format_ARGB32);
    m_patches.clear();
    for (const QPoint &point : m_points) {
        applyBlurAt(point);
    }
}

void BlurShape::applyBlurAt(const QPoint &center)
{
    if (m_canvasSnapshot.isNull()) {
        return;
    }

    if (!m_patches.isEmpty()) {
        const QPoint delta = m_patches.constLast().center - center;
        const int minDistance = qMax(2, m_brushRadius / 3);
        if ((delta.x() * delta.x() + delta.y() * delta.y()) < (minDistance * minDistance)) {
            return;
        }
    }

    const int patchRadius = m_brushRadius + m_blurRadius * 2;
    QRect patchRect(center.x() - patchRadius,
                    center.y() - patchRadius,
                    patchRadius * 2 + 1,
                    patchRadius * 2 + 1);
    patchRect = patchRect.intersected(m_canvasSnapshot.rect());
    if (patchRect.isEmpty()) {
        return;
    }

    BlurPatch patch;
    patch.center = center;
    patch.rect = patchRect;
    patch.image = makeBlurredImage(m_canvasSnapshot.copy(patchRect));
    if (patch.image.isNull()) {
        return;
    }
    m_patches.append(patch);
}

QImage BlurShape::makeBlurredImage(const QImage &source) const
{
    if (source.isNull()) {
        return {};
    }

    const int downscale = qMax(1, m_blurRadius);
    const QSize smallSize(qMax(1, source.width() / downscale),
                          qMax(1, source.height() / downscale));
    const QImage small = source.scaled(smallSize,
                                       Qt::IgnoreAspectRatio,
                                       Qt::SmoothTransformation);
    return small.scaled(source.size(),
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation);
}
