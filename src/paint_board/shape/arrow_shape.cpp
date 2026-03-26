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
    const QPointF startPoint(m_startPoint);
    const QPointF endPoint(m_endPoint);
    QPen pen(m_color, m_size);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter->setPen(pen);
    painter->setRenderHint(QPainter::Antialiasing);

    painter->drawLine(startPoint, endPoint);

    // 计算箭头方向角
    const double angle = std::atan2(
        endPoint.y() - startPoint.y(),
        endPoint.x() - startPoint.x()
    );

    const qreal headLength = ARROW_HEAD_LENGTH;
    QPointF arrowP1(
        endPoint.x() - std::cos(angle + ARROW_HEAD_ANGLE) * headLength,
        endPoint.y() - std::sin(angle + ARROW_HEAD_ANGLE) * headLength
    );
    QPointF arrowP2(
        endPoint.x() - std::cos(angle - ARROW_HEAD_ANGLE) * headLength,
        endPoint.y() - std::sin(angle - ARROW_HEAD_ANGLE) * headLength
    );

    QPolygonF arrowHead;
    arrowHead << endPoint << arrowP1 << arrowP2;
    painter->setBrush(QBrush(m_color));
    painter->drawPolygon(arrowHead);

    painter->restore();
}
