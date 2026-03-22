#ifndef ARROWSHAPE_H
#define ARROWSHAPE_H

#include "two_point_shape.h"
#include <cmath>

class ArrowShape : public TwoPointShape
{
public:
    ArrowShape(const QPoint& startPoint = QPoint(0, 0),
               const QPoint& endPoint   = QPoint(0, 0),
               const QColor& color      = Qt::black,
               int size                 = 2);

    void draw(QPainter* painter) override;

private:
    static constexpr double ARROW_HEAD_LENGTH = 15.0; // 箭头头部长度
    static constexpr double ARROW_HEAD_ANGLE  = M_PI / 6.0; // 箭头张角 30°
};

#endif // ARROWSHAPE_H
