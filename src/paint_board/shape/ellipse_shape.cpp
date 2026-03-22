#include "ellipse_shape.h"

EllipseShape::EllipseShape(const QPoint &startPoint, const QPoint &endPoint,
                           const QColor &color, int size)
    : TwoPointShape(startPoint, endPoint, color, size) {}

void EllipseShape::draw(QPainter *painter)
{
    if (!painter) return;
    painter->save();
    QPen pen(m_color, m_size);
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->setRenderHint(QPainter::Antialiasing);
    // 用两点构造外接矩形来绘制椭圆，normalized() 处理反向拖动
    painter->drawEllipse(QRect(m_startPoint, m_endPoint).normalized());
    painter->restore();
}

