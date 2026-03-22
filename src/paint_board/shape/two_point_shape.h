#ifndef TWOPOINTSHAPE_H
#define TWOPOINTSHAPE_H

#include "shape.h"

// 两点图形基类：所有由起点+终点确定的图形（矩形、椭圆、箭头）均继承此类
class TwoPointShape : public Shape
{
public:
    TwoPointShape(const QPoint& startPoint,
                  const QPoint& endPoint,
                  const QColor& color = Qt::black,
                  int size = 2);

    void setStartPoint(const QPoint& point);
    QPoint startPoint() const;

    // 实现 Shape 的纯虚函数
    void setEndPoint(const QPoint& point) override;
    QPoint endPoint() const;

protected:
    QPoint m_startPoint;
    QPoint m_endPoint;
};

#endif // TWOPOINTSHAPE_H
