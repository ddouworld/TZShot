#ifndef RECTSHAPE_H
#define RECTSHAPE_H
#include "two_point_shape.h"

class RectShape : public TwoPointShape
{
public:
    RectShape(const QPoint& startPoint = QPoint(0, 0),
              const QPoint& endPoint   = QPoint(0, 0),
              const QColor& color      = Qt::black,
              int size                 = 2);

    void draw(QPainter* painter) override;
};

#endif // RECTSHAPE_H
