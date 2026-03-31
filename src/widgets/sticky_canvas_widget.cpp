#include "widgets/sticky_canvas_widget.h"

#include "paint_board/shape/blur_shape.h"
#include "paint_board/shape/mosaic_shape.h"
#include "paint_board/shape/text_shape.h"

#include <QApplication>
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

void StickyCanvasWidget::setContentOpacity(qreal opacity)
{
    const qreal next = qBound<qreal>(0.05, opacity, 1.0);
    if (qFuzzyCompare(m_contentOpacity, next)) {
        return;
    }
    m_contentOpacity = next;
    update();
}

void StickyCanvasWidget::setActiveShapeType(Shapeype type)
{
    if (m_shapeType != type && type != TEXT) {
        clearSelectedText();
        update();
    }
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
        clearSelectedText();
        update();
    }
}

void StickyCanvasWidget::setNumberValue(int value)
{
    m_numberValue = qMax(1, value);
}

void StickyCanvasWidget::setNumberAutoIncrement(bool enabled)
{
    m_numberAutoIncrement = enabled;
}

void StickyCanvasWidget::addTextAnnotation(const QPoint &point,
                                           const QString &text,
                                           const QColor &color,
                                           int size,
                                           bool withBackground)
{
    const QString resolved = text.trimmed();
    if (resolved.isEmpty()) {
        return;
    }

    const QPoint imagePoint(qRound(point.x() / m_viewScale),
                            qRound(point.y() / m_viewScale));

    Shape *textShape = m_factory.createTextShape(imagePoint,
                                                 resolved,
                                                 color,
                                                 size,
                                                 withBackground);
    if (!textShape) {
        return;
    }

    m_shapes.append(textShape);
    clearSelectedText();
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
        const int removedIndex = m_shapes.size() - 1;
        delete m_shapes.takeLast();
        if (m_selectedTextShapeIndex == removedIndex) {
            clearSelectedText();
        }
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

    painter.save();
    painter.setOpacity(m_contentOpacity);
    if (m_backgroundVisible) {
        painter.fillRect(rect(), Qt::white);
    }

    if (m_backgroundVisible && !m_backgroundImage.isNull()) {
        painter.drawImage(rect(), m_backgroundImage);
    }

    painter.scale(m_viewScale, m_viewScale);
    for (Shape *shape : m_shapes) {
        if (shape) {
            shape->draw(&painter);
        }
    }

    drawSelectedTextOutline(&painter);

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
    m_textPressDisplayPoint = displayPoint;

    if (m_shapeType == TEXT) {
        const int textIndex = findTopmostTextShapeIndex(m_startPoint);
        if (textIndex >= 0) {
            m_selectedTextShapeIndex = textIndex;
            m_draggingSelectedText = false;
            if (TextShape *textShape = textShapeAt(textIndex)) {
                m_selectedTextDragOffset = m_startPoint - textShape->anchorPoint();
            } else {
                m_selectedTextDragOffset = QPoint();
            }
            update();
            return;
        }

        clearSelectedText();
        update();
        emit textPlacementRequested(displayPoint);
        return;
    }

    clearSelectedText();

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

void StickyCanvasWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (!m_drawingEnabled || m_shapeType != TEXT || event->button() != Qt::LeftButton) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QPoint displayPoint = event->position().toPoint();
    const QPoint imagePoint(qRound(displayPoint.x() / m_viewScale),
                            qRound(displayPoint.y() / m_viewScale));
    const int textIndex = findTopmostTextShapeIndex(imagePoint);
    TextShape *textShape = textShapeAt(textIndex);
    if (!textShape) {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }

    const QPoint anchorPoint = textShape->anchorPoint();
    const QPoint displayAnchor(qRound(anchorPoint.x() * m_viewScale),
                               qRound(anchorPoint.y() * m_viewScale));
    const QString text = textShape->text();
    const QColor color = textShape->color();
    const int size = textShape->size();
    const bool withBackground = textShape->withBackground();

    delete m_shapes.takeAt(textIndex);
    clearSelectedText();
    update();
    emit textEditRequested(displayAnchor, text, color, size, withBackground);
}

void StickyCanvasWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_shapeType == TEXT && (event->buttons() & Qt::LeftButton)) {
        TextShape *selectedTextShape = textShapeAt(m_selectedTextShapeIndex);
        if (selectedTextShape) {
            if (!m_draggingSelectedText) {
                const int dragDistance =
                    (event->position().toPoint() - m_textPressDisplayPoint).manhattanLength();
                if (dragDistance < QApplication::startDragDistance()) {
                    return;
                }
                m_draggingSelectedText = true;
            }

            const QPoint imagePoint(qRound(event->position().x() / m_viewScale),
                                    qRound(event->position().y() / m_viewScale));
            selectedTextShape->setEndPoint(imagePoint - m_selectedTextDragOffset);
            update();
            return;
        }
    }

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
    if (m_shapeType == TEXT && event->button() == Qt::LeftButton) {
        if (m_draggingSelectedText) {
            m_draggingSelectedText = false;
            update();
            return;
        }
        if (m_selectedTextShapeIndex >= 0) {
            return;
        }
    }

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
    clearSelectedText();
}

void StickyCanvasWidget::clearSelectedText()
{
    m_selectedTextShapeIndex = -1;
    m_draggingSelectedText = false;
    m_textPressDisplayPoint = QPoint();
    m_selectedTextDragOffset = QPoint();
}

int StickyCanvasWidget::findTopmostTextShapeIndex(const QPoint &point) const
{
    for (int i = m_shapes.size() - 1; i >= 0; --i) {
        TextShape *textShape = textShapeAt(i);
        if (textShape && textShape->contains(point)) {
            return i;
        }
    }
    return -1;
}

TextShape *StickyCanvasWidget::textShapeAt(int index) const
{
    if (index < 0 || index >= m_shapes.size()) {
        return nullptr;
    }
    return dynamic_cast<TextShape *>(m_shapes.at(index));
}

void StickyCanvasWidget::drawSelectedTextOutline(QPainter *painter) const
{
    if (!painter || m_shapeType != TEXT) {
        return;
    }

    TextShape *selectedTextShape = textShapeAt(m_selectedTextShapeIndex);
    if (!selectedTextShape) {
        return;
    }

    QPen pen(selectedTextShape->color());
    pen.setWidth(1);
    pen.setCosmetic(true);
    pen.setStyle(Qt::CustomDashLine);
    pen.setDashPattern({ 3, 3 });
    painter->save();
    painter->setPen(pen);
    painter->setBrush(Qt::NoBrush);
    painter->drawRect(selectedTextShape->boundingRect().adjusted(-3, -3, 3, 3));
    painter->restore();
}


