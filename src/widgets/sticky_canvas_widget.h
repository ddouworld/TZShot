#ifndef STICKY_CANVAS_WIDGET_H
#define STICKY_CANVAS_WIDGET_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

#include "paint_board/shape/shape.h"
#include "paint_board/shape/shape_factory.h"
#include "paint_board/shape/shape_type.h"

class TextShape;

class StickyCanvasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StickyCanvasWidget(QWidget *parent = nullptr);
    ~StickyCanvasWidget() override;

    void setBackgroundImage(const QImage &image);
    void setBackgroundVisible(bool visible);
    void setViewScale(qreal scale);
    void setContentOpacity(qreal opacity);
    void setActiveShapeType(Shapeype type);
    void setPenColor(const QColor &color);
    void setPenSize(int size);
    void setDrawingEnabled(bool enabled);
    void setNumberValue(int value);
    void setNumberAutoIncrement(bool enabled);
    bool drawingEnabled() const { return m_drawingEnabled; }
    Shapeype activeShapeType() const { return m_shapeType; }
    bool hasAnnotations() const { return !m_shapes.isEmpty(); }
    QImage compositedImage() const;
    QImage compositedImage(const QRect &displayRect) const;
    void addTextAnnotation(const QPoint &point,
                           const QString &text,
                           const QColor &color,
                           int size,
                           bool withBackground);
    void undo();
    void reset();

signals:
    void numberValueChanged(int value);
    void textPlacementRequested(const QPoint &point);
    void textEditRequested(const QPoint &point,
                           const QString &text,
                           const QColor &color,
                           int size,
                           bool withBackground);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void finishCurrentShape();
    void clearShapes();
    void clearSelectedText();
    int findTopmostTextShapeIndex(const QPoint &point) const;
    TextShape *textShapeAt(int index) const;
    void drawSelectedTextOutline(QPainter *painter) const;

    ShapeFactory m_factory;
    QList<Shape*> m_shapes;
    Shape *m_currentShape = nullptr;
    QImage m_backgroundImage;
    bool m_backgroundVisible = true;
    QColor m_penColor = Qt::red;
    int m_penSize = 6;
    Shapeype m_shapeType = PEN;
    bool m_numberAutoIncrement = true;
    int m_numberValue = 1;
    bool m_drawingEnabled = false;
    bool m_draggingShape = false;
    int m_selectedTextShapeIndex = -1;
    bool m_draggingSelectedText = false;
    qreal m_viewScale = 1.0;
    qreal m_contentOpacity = 1.0;
    QPoint m_startPoint;
    QPoint m_textPressDisplayPoint;
    QPoint m_selectedTextDragOffset;
};

#endif // STICKY_CANVAS_WIDGET_H

