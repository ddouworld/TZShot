#include "widgets/sticky_canvas_widget.h"

#include "paint_board/shape/blur_shape.h"
#include "paint_board/shape/mosaic_shape.h"

#include <QMouseEvent>
#include <QPainter>

StickyCanvasWidget::StickyCanvasWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setMouseTracking(true);
}

StickyCanvasWidget::~StickyCanvasWidget()
{
    clearShapes();
}

void StickyCanvasWidget::setBackgroundImage(const QImage &image)
{
    m_backgroundImage = image;
    for (Shape *shape : m_shapes) {
        if (auto *mosaic = dynamic_cast<MosaicShape*>(shape)) {
            mosaic->setCanvasSnapshot(m_backgroundImage);
        } else if (auto *blur = dynamic_cast<BlurShape*>(shape)) {
            blur->setCanvasSnapshot(m_backgroundImage);
        }
    }
    update();
}

void StickyCanvasWidget::setBackgroundVisible(bool visible)
{
    if (m_backgroundVisible == visible) {
        return;
    }
    m_backgroundVisible = visible;
    update();
}

void StickyCanvasWidget::setViewScale(qreal scale)
{
    const qreal next = qMax<qreal>(0.01, scale);
    if (qFuzzyCompare(m_viewScale, next)) {
        return;
    }
    m_viewScale = next;
    update();
}

void StickyCanvasWidget::setActiveShapeType(Shapeype type)
{
    m_shapeType = type;
}

void StickyCanvasWidget::setPenColor(const QColor &color)
{
    m_penColor = color;
}

void StickyCanvasWidget::setPenSize(int size)
{
    m_penSize = qMax(1, size);
}

void StickyCanvasWidget::setDrawingEnabled(bool enabled)
{
    m_drawingEnabled = enabled;
    setAttribute(Qt::WA_TransparentForMouseEvents, !m_drawingEnabled);
    if (!m_drawingEnabled) {
        finishCurrentShape();
    }
}

void StickyCanvasWidget::setAnnotationText(const QString &text)
{
    m_annotationText = text;
}

void StickyCanvasWidget::setTextBackgroundEnabled(bool enabled)
{
    m_textBackgroundEnabled = enabled;
}

void StickyCanvasWidget::setNumberValue(int value)
{
    m_numberValue = qMax(1, value);
}

void StickyCanvasWidget::setNumberAutoIncrement(bool enabled)
{
    m_numberAutoIncrement = enabled;
}

void StickyCanvasWidget::addTextAnnotation(const QPoint &point, const QString &text)
{
    const QString resolved = text.trimmed();
    if (resolved.isEmpty()) {
        return;
    }

    const QPoint imagePoint(qRound(point.x() / m_viewScale),
                            qRound(point.y() / m_viewScale));

    Shape *textShape = m_factory.createTextShape(imagePoint,
                                                 resolved,
                                                 m_penColor,
                                                 m_penSize,
                                                 m_textBackgroundEnabled);
    if (!textShape) {
        return;
    }

    m_shapes.append(textShape);
    m_annotationText = resolved;
    update();
}

QImage StickyCanvasWidget::compositedImage() const
{
    return compositedImage(rect());
}

QImage StickyCanvasWidget::compositedImage(const QRect &displayRect) const
{
    const QRect targetRect = displayRect.normalized().intersected(rect());
    if (targetRect.isEmpty()) {
        return {};
    }

    QImage image(targetRect.size(), QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(-targetRect.topLeft());

    if (!m_backgroundImage.isNull()) {
        painter.drawImage(rect(), m_backgroundImage);
    } else {
        painter.fillRect(targetRect, Qt::white);
    }

    painter.save();
    painter.scale(m_viewScale, m_viewScale);
    for (Shape *shape : m_shapes) {
        if (shape) {
            shape->draw(&painter);
        }
    }
    if (m_currentShape) {
        m_currentShape->draw(&painter);
    }
    painter.restore();

    return image;
}

void StickyCanvasWidget::undo()
{
    if (!m_shapes.isEmpty()) {
        delete m_shapes.takeLast();
        update();
    }
}

void StickyCanvasWidget::reset()
{
    finishCurrentShape();
    clearShapes();
    m_backgroundImage = QImage();
    update();
}

void StickyCanvasWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    if (m_backgroundVisible) {
        painter.fillRect(rect(), Qt::white);
    }

    if (m_backgroundVisible && !m_backgroundImage.isNull()) {
        painter.drawImage(rect(), m_backgroundImage);
    }

    painter.save();
    painter.scale(m_viewScale, m_viewScale);
    for (Shape *shape : m_shapes) {
        if (shape) {
            shape->draw(&painter);
        }
    }

    if (m_currentShape) {
        m_currentShape->draw(&painter);
    }
    painter.restore();
}

void StickyCanvasWidget::mousePressEvent(QMouseEvent *event)
{
    if (!m_drawingEnabled || event->button() != Qt::LeftButton) {
        QWidget::mousePressEvent(event);
        return;
    }

    const QPoint displayPoint = event->position().toPoint();
    m_startPoint = QPoint(qRound(displayPoint.x() / m_viewScale),
                          qRound(displayPoint.y() / m_viewScale));

    if (m_shapeType == TEXT) {
        emit textPlacementRequested(displayPoint);
        return;
    }

    if (m_shapeType == NUMBER) {
        Shape *numberShape = m_factory.createNumberShape(m_startPoint,
                                                         m_numberValue,
                                                         m_penColor,
                                                         m_penSize);
        if (numberShape) {
            m_shapes.append(numberShape);
            if (m_numberAutoIncrement) {
                ++m_numberValue;
                emit numberValueChanged(m_numberValue);
            }
            update();
        }
        return;
    }

    delete m_currentShape;
    m_currentShape = nullptr;

    if (m_shapeType == MOSAIC) {
        auto *mosaic = new MosaicShape(m_startPoint,
                                       qMax(10, m_penSize * 5),
                                       qMax(2, m_penSize));
        mosaic->setCanvasSnapshot(m_backgroundImage);
        m_currentShape = mosaic;
        m_draggingShape = true;
        update();
        return;
    }

    if (m_shapeType == BLUR) {
        auto *blur = new BlurShape(m_startPoint,
                                   qMax(10, m_penSize * 5),
                                   qMax(2, m_penSize));
        blur->setCanvasSnapshot(m_backgroundImage);
        m_currentShape = blur;
        m_draggingShape = true;
        update();
        return;
    }

    m_currentShape = m_factory.createShape(m_shapeType,
                                           m_startPoint,
                                           m_startPoint,
                                           m_penColor,
                                           m_penSize);
    m_draggingShape = (m_currentShape != nullptr);
    update();
}

void StickyCanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_draggingShape || !m_currentShape) {
        QWidget::mouseMoveEvent(event);
        return;
    }

    const QPoint imagePoint(qRound(event->position().x() / m_viewScale),
                            qRound(event->position().y() / m_viewScale));
    m_currentShape->setEndPoint(imagePoint);
    update();
}

void StickyCanvasWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_draggingShape || !m_currentShape || event->button() != Qt::LeftButton) {
        QWidget::mouseReleaseEvent(event);
        return;
    }

    const QPoint imagePoint(qRound(event->position().x() / m_viewScale),
                            qRound(event->position().y() / m_viewScale));
    m_currentShape->setEndPoint(imagePoint);
    m_shapes.append(m_currentShape);
    m_currentShape = nullptr;
    m_draggingShape = false;
    update();
}

void StickyCanvasWidget::finishCurrentShape()
{
    if (m_currentShape) {
        delete m_currentShape;
        m_currentShape = nullptr;
    }
    m_draggingShape = false;
}

void StickyCanvasWidget::clearShapes()
{
    qDeleteAll(m_shapes);
    m_shapes.clear();
}
