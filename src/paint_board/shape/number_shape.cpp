#include "number_shape.h"

#include <QFont>
#include <QFontMetrics>

NumberShape::NumberShape(const QPoint& center,
                         int number,
                         const QColor& color,
                         int size)
    : Shape(color, size)
    , m_center(center)
    , m_number(qMax(1, number))
{
}

void NumberShape::setEndPoint(const QPoint& point)
{
    m_center = point;
}

void NumberShape::draw(QPainter* painter)
{
    if (!painter) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    const int r = qMax(10, m_size * 2 + 6);
    const QRect circleRect(m_center.x() - r,
                           m_center.y() - r,
                           r * 2,
                           r * 2);

    painter->setPen(QPen(Qt::white, qMax(2, m_size / 2)));
    painter->setBrush(m_color);
    painter->drawEllipse(circleRect);

    QFont f = painter->font();
    f.setBold(true);
    f.setPixelSize(qMax(12, r));
    painter->setFont(f);
    painter->setPen(Qt::white);
    painter->drawText(circleRect, Qt::AlignCenter, QString::number(m_number));

    painter->restore();
}
