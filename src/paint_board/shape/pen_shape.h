#ifndef PENSHAPE_H
#define PENSHAPE_H

#include "shape.h"
#include <QList>

// 画笔图形：记录鼠标经过的所有点，连线绘制
class PenShape : public Shape
{
public:
    PenShape(const QPoint& startPoint,
             const QColor& color = Qt::black,
             int size = 2);

    // 追加一个路径点
    void addPoint(const QPoint& point);

    // 实现 Shape 纯虚函数，内部调用 addPoint
    void setEndPoint(const QPoint& point) override;

    void draw(QPainter* painter) override;

private:
    QVector<QPoint> m_points; // 路径上所有点
};

#endif // PENSHAPE_H
