#ifndef PAINTBOARD_H
#define PAINTBOARD_H

#include <QObject>
#include <QQuickPaintedItem>
#include <QPainter>
#include <QColor>
#include <QString>
#include <memory>

#include "shape/shape.h"
#include "shape/shape_type.h"
#include "shape/shape_factory.h"
#include "shape/mosaic_shape.h"

class PaintBoard : public QQuickPaintedItem
{
    Q_OBJECT

    Q_PROPERTY(int shapeType READ shapeType WRITE setShapeType NOTIFY shapeTypeChanged FINAL)
    Q_PROPERTY(QColor penColor READ penColor WRITE setPenColor NOTIFY penColorChanged FINAL)
    Q_PROPERTY(int penSize READ penSize WRITE setPenSize NOTIFY penSizeChanged FINAL)
    Q_PROPERTY(QString annotationText READ annotationText WRITE setAnnotationText NOTIFY annotationTextChanged FINAL)
    Q_PROPERTY(bool textBackgroundEnabled READ textBackgroundEnabled WRITE setTextBackgroundEnabled NOTIFY textBackgroundEnabledChanged FINAL)
    Q_PROPERTY(int mosaicBlurLevel READ mosaicBlurLevel WRITE setMosaicBlurLevel NOTIFY mosaicBlurLevelChanged FINAL)
    Q_PROPERTY(bool numberAutoIncrement READ numberAutoIncrement WRITE setNumberAutoIncrement NOTIFY numberAutoIncrementChanged FINAL)
    Q_PROPERTY(int numberValue READ numberValue WRITE setNumberValue NOTIFY numberValueChanged FINAL)

public:
    explicit PaintBoard(QQuickItem *parent = nullptr);

    int shapeType() const { return static_cast<int>(m_currentType); }
    QColor penColor() const { return m_currentColor; }
    int penSize() const { return m_currentSize; }
    QString annotationText() const { return m_annotationText; }
    bool textBackgroundEnabled() const { return m_textBackgroundEnabled; }
    int mosaicBlurLevel() const { return m_mosaicBlurLevel; }
    bool numberAutoIncrement() const { return m_numberAutoIncrement; }
    int numberValue() const { return m_numberValue; }

    void setShapeType(int type);
    void setPenColor(const QColor &color);
    void setPenSize(int size);
    void setAnnotationText(const QString& text);
    void setTextBackgroundEnabled(bool enabled);
    void setMosaicBlurLevel(int level);
    void setNumberAutoIncrement(bool enabled);
    void setNumberValue(int value);

public:
    Q_INVOKABLE void setBackgroundImg(QImage img);
    Q_INVOKABLE bool hasBackgroundImg() const { return !m_background_img.isNull(); }
    Q_INVOKABLE void addTextAnnotation(int x, int y, const QString& text);

    // Render all annotations onto a transparent image for screenshot merge.
    Q_INVOKABLE QImage renderToImage() const;

    Q_INVOKABLE void undo();
    Q_INVOKABLE void reset();  // Clear all shapes and background image.

signals:
    void shapeTypeChanged(int shapeType);
    void penColorChanged(QColor penColor);
    void penSizeChanged(int penSize);
    void annotationTextChanged(const QString& text);
    void textBackgroundEnabledChanged(bool enabled);
    void mosaicBlurLevelChanged(int level);
    void numberAutoIncrementChanged(bool enabled);
    void numberValueChanged(int value);

protected:
    void paint(QPainter *painter) override;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    void hoverMoveEvent(QHoverEvent *event) override;

    bool drawBkImg(QPainter *painter, QImage img);
    bool drawShape(QPainter *painter);

private:
    ShapeFactory m_factory;

    QList<Shape*> m_shapes;
    Shape* m_currentShape = nullptr;
    bool m_drawing = false;
    QPoint m_startPoint;

    Shapeype m_currentType = RECTANGLE;
    QColor m_currentColor = Qt::red;
    int m_currentSize = 6;
    QString m_annotationText = QStringLiteral("Text");
    bool m_textBackgroundEnabled = true;
    int m_mosaicBlurLevel = 2;
    bool m_numberAutoIncrement = true;
    int m_numberValue = 1;

    QImage m_background_img;
};

#endif // PAINTBOARD_H
