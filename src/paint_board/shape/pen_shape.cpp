#include "pen_shape.h"

PenShape::PenShape(const QPoint &startPoint, const QColor &color, int size)
    : Shape(color, size)
{
    m_points.append(startPoint);
}

void PenShape::addPoint(const QPoint &point)
{
    // 避免重复添加相同的点
    if (!m_points.isEmpty() && m_points.last() == point)
        return;
    m_points.append(point);
}

void PenShape::setEndPoint(const QPoint &point)
{
    addPoint(point);
}

void PenShape::draw(QPainter *painter)
{
    if (!painter || m_points.size() < 2)
        return;

    painter->save();
    QPen pen(m_color, m_size);
    pen.setCapStyle(Qt::RoundCap);    // 端点圆滑
    pen.setJoinStyle(Qt::RoundJoin);  // 转折点圆滑
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->drawPolyline(QPolygon(m_points));

    painter->restore();
}
