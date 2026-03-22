#ifndef SHAPEFACTORY_H
#define SHAPEFACTORY_H

#include "shape.h"
#include "shape_type.h"
#include <QColor>
#include <QPoint>
#include <QString>

class ShapeFactory
{
public:
    ShapeFactory();

    Shape* createShape(Shapeype type,
                       const QPoint& startPoint,
                       const QPoint& endPoint,
                       const QColor& color,
                       int size);

    Shape* createTextShape(const QPoint& startPoint,
                           const QString& text,
                           const QColor& color,
                           int size,
                           bool withBackground);

    Shape* createNumberShape(const QPoint& center,
                             int number,
                             const QColor& color,
                             int size);
};

#endif // SHAPEFACTORY_H
