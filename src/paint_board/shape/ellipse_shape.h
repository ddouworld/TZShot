#ifndef CIRCLESHAPE_H
#define CIRCLESHAPE_H

#include "two_point_shape.h"

class EllipseShape : public TwoPointShape
{
public:
    EllipseShape(const QPoint& startPoint = QPoint(0, 0),
                 const QPoint& endPoint   = QPoint(0, 0),
                 const QColor& color      = Qt::black,
                 int size                 = 2);

    void draw(QPainter* painter) override;
};

#endif // CIRCLESHAPE_H
