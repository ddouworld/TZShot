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
}

void BlurShape::draw(QPainter *painter)
{
    if (!painter || m_blurredSnapshot.isNull() || m_points.isEmpty()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    for (const QPoint &point : m_points) {
        QPainterPath path;
        path.addEllipse(QPointF(point), m_brushRadius, m_brushRadius);
        painter->save();
        painter->setClipPath(path);
        painter->drawImage(QPoint(0, 0), m_blurredSnapshot);
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
    m_blurredSnapshot = makeBlurredImage(m_canvasSnapshot);
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
