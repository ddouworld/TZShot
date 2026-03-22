#ifndef SHAPE_H
#define SHAPE_H
#include <QPainter>
#include <QColor>
#include <QPoint>

class Shape
{
public:
    Shape(const QColor& color = Qt::black, int size = 2);

    virtual ~Shape() = default;
    // 纯虚函数：统一的绘制接口（子类必须实现）
    virtual void draw(QPainter* painter) = 0;
    // 纯虚函数：更新终点（用于绘制预览）
    virtual void setEndPoint(const QPoint& point) = 0;

    void setColor(const QColor& color) { m_color = color; }
    QColor color() const { return m_color; }

    void setSize(int size) { m_size = size; }
    int size() const { return m_size; }

protected:
    QColor m_color;
    int    m_size;
};

#endif // SHAPE_H
