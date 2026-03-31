#ifndef TEXT_SHAPE_H
#define TEXT_SHAPE_H

#include "shape.h"
#include <QString>

class TextShape : public Shape
{
public:
    TextShape(const QPoint& startPoint,
              const QString& text,
              const QColor& color,
              int size,
              bool withBackground);

    void setEndPoint(const QPoint& point) override;
    void draw(QPainter* painter) override;
    bool contains(const QPoint &point) const;
    QRect boundingRect() const;
    QPoint anchorPoint() const { return m_point; }
    QString text() const { return m_text; }
    bool withBackground() const { return m_withBackground; }

private:
    QPoint  m_point;
    QString m_text;
    bool    m_withBackground = true;
};

#endif // TEXT_SHAPE_H
