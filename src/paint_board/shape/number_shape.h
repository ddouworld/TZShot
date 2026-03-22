#ifndef NUMBER_SHAPE_H
#define NUMBER_SHAPE_H

#include "shape.h"

class NumberShape : public Shape
{
public:
    NumberShape(const QPoint& center,
                int number,
                const QColor& color,
                int size);

    void setEndPoint(const QPoint& point) override;
    void draw(QPainter* painter) override;

private:
    QPoint m_center;
    int    m_number = 1;
};

#endif // NUMBER_SHAPE_H
