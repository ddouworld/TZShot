#ifndef STICKY_CANVAS_WIDGET_H
#define STICKY_CANVAS_WIDGET_H

#include <QColor>
#include <QImage>
#include <QPoint>
#include <QWidget>

#include "paint_board/shape/shape.h"
#include "paint_board/shape/shape_factory.h"
#include "paint_board/shape/shape_type.h"

class StickyCanvasWidget : public QWidget
{
    Q_OBJECT

public:
    explicit StickyCanvasWidget(QWidget *parent = nullptr);
    ~StickyCanvasWidget() override;

    void setBackgroundImage(const QImage &image);
    void setViewScale(qreal scale);
    void setActiveShapeType(Shapeype type);
    void setPenColor(const QColor &color);
    void setPenSize(int size);
    void setDrawingEnabled(bool enabled);
    void setAnnotationText(const QString &text);
    void setTextBackgroundEnabled(bool enabled);
    void setNumberValue(int value);
    void setNumberAutoIncrement(bool enabled);
    bool drawingEnabled() const { return m_drawingEnabled; }
    Shapeype activeShapeType() const { return m_shapeType; }
    void addTextAnnotation(const QPoint &point, const QString &text);
    void undo();
    void reset();

signals:
    void numberValueChanged(int value);
    void textPlacementRequested(const QPoint &point);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    void finishCurrentShape();
    void clearShapes();

    ShapeFactory m_factory;
    QList<Shape*> m_shapes;
    Shape *m_currentShape = nullptr;
    QImage m_backgroundImage;
    QColor m_penColor = Qt::red;
    int m_penSize = 6;
    Shapeype m_shapeType = PEN;
    QString m_annotationText = QStringLiteral("Text");
    bool m_textBackgroundEnabled = true;
    bool m_numberAutoIncrement = true;
    int m_numberValue = 1;
    bool m_drawingEnabled = false;
    bool m_draggingShape = false;
    qreal m_viewScale = 1.0;
    QPoint m_startPoint;
};

#endif // STICKY_CANVAS_WIDGET_H
