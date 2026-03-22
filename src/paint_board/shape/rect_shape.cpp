#include "rect_shape.h"

RectShape::RectShape(const QPoint &startPoint, const QPoint &endPoint,
                     const QColor &color, int size)
    : TwoPointShape(startPoint, endPoint, color, size) {}

void RectShape::draw(QPainter *painter)
{
    if (!painter) return;
    painter->save();
    QPen pen(m_color, m_size);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->setRenderHint(QPainter::Antialiasing);
    // normalized() 处理从右下往左上拖动的情况
    painter->drawRect(QRect(m_startPoint, m_endPoint).normalized());
    painter->restore();
}

