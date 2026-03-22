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

private:
    QPoint  m_point;
    QString m_text;
    bool    m_withBackground = true;
};

#endif // TEXT_SHAPE_H
