#include "arrow_shape.h"

ArrowShape::ArrowShape(const QPoint &startPoint, const QPoint &endPoint,
                       const QColor &color, int size)
    : TwoPointShape(startPoint, endPoint, color, size) {}

void ArrowShape::draw(QPainter *painter)
{
    if (!painter) return;

    // 起点和终点相同时不绘制
    if (m_startPoint == m_endPoint) return;

    painter->save();
    QPen pen(m_color, m_size);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);

    // 绘制箭杆
    painter->drawLine(m_startPoint, m_endPoint);

    // 计算箭头方向角
    const double angle = std::atan2(
        m_endPoint.y() - m_startPoint.y(),
        m_endPoint.x() - m_startPoint.x()
    );

    // 计算箭头两侧的点
    QPoint arrowP1(
        m_endPoint.x() - static_cast<int>(std::cos(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH),
        m_endPoint.y() - static_cast<int>(std::sin(angle + ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH)
    );
    QPoint arrowP2(
        m_endPoint.x() - static_cast<int>(std::cos(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH),
        m_endPoint.y() - static_cast<int>(std::sin(angle - ARROW_HEAD_ANGLE) * ARROW_HEAD_LENGTH)
    );

    // 绘制实心箭头头部
    QPolygon arrowHead;
    arrowHead << m_endPoint << arrowP1 << arrowP2;
    painter->setBrush(QBrush(m_color));
    painter->drawPolygon(arrowHead);

    painter->restore();
}

