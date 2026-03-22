#include "shape_factory.h"
#include "rect_shape.h"
#include "arrow_shape.h"
#include "ellipse_shape.h"
#include "pen_shape.h"
#include "text_shape.h"
#include "number_shape.h"

ShapeFactory::ShapeFactory() {}

Shape* ShapeFactory::createShape(Shapeype type,
                                 const QPoint& startPoint,
                                 const QPoint& endPoint,
                                 const QColor& color,
                                 int size)
{
    switch (type)
    {
    case PEN:
        // Freehand pen only needs the start point.
        return new PenShape(startPoint, color, size);
    case RECTANGLE:
        return new RectShape(startPoint, endPoint, color, size);
    case ELLIPSE:
        return new EllipseShape(startPoint, endPoint, color, size);
    case ARROW:
        return new ArrowShape(startPoint, endPoint, color, size);
    case HIGHLIGHT: {
        QColor hi = color;
        hi.setAlpha(100);
        return new PenShape(startPoint, hi, qMax(8, size * 2));
    }
    default:
        return nullptr;
    }
}

Shape* ShapeFactory::createTextShape(const QPoint& startPoint,
                                     const QString& text,
                                     const QColor& color,
                                     int size,
                                     bool withBackground)
{
    return new TextShape(startPoint, text, color, size, withBackground);
}

Shape* ShapeFactory::createNumberShape(const QPoint& center,
                                       int number,
                                       const QColor& color,
                                       int size)
{
    return new NumberShape(center, number, color, size);
}
