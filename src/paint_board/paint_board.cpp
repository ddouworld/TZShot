#include "paint_board.h"
#include "shape/mosaic_shape.h"
#include <QDebug>

PaintBoard::PaintBoard(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setAcceptedMouseButtons(Qt::LeftButton);
}

void PaintBoard::setBackgroundImg(QImage img)
{
    m_background_img = img;
    update();
}

void PaintBoard::addTextAnnotation(int x, int y, const QString& text)
{
    const QString resolvedText = text.trimmed().isEmpty() ? m_annotationText : text;
    Shape* shape = m_factory.createTextShape(
        QPoint(x, y),
        resolvedText,
        m_currentColor,
        m_currentSize,
        m_textBackgroundEnabled
    );
    if (!shape)
        return;

    m_shapes.append(shape);
    update();
}

void PaintBoard::setShapeType(int type)
{
    const auto t = static_cast<Shapeype>(type);
    if (m_currentType == t)
        return;
    m_currentType = t;
    emit shapeTypeChanged(type);
}

void PaintBoard::setPenColor(const QColor &color)
{
    if (m_currentColor == color)
        return;
    m_currentColor = color;
    emit penColorChanged(color);
}

void PaintBoard::setPenSize(int size)
{
    if (m_currentSize == size)
        return;
    m_currentSize = size;
    emit penSizeChanged(size);
}

void PaintBoard::setAnnotationText(const QString& text)
{
    if (m_annotationText == text)
        return;
    m_annotationText = text;
    emit annotationTextChanged(m_annotationText);
}

void PaintBoard::setTextBackgroundEnabled(bool enabled)
{
    if (m_textBackgroundEnabled == enabled)
        return;
    m_textBackgroundEnabled = enabled;
    emit textBackgroundEnabledChanged(enabled);
}

void PaintBoard::setMosaicBlurLevel(int level)
{
    const int normalized = qBound(1, level, 4);
    if (m_mosaicBlurLevel == normalized)
        return;
    m_mosaicBlurLevel = normalized;
    emit mosaicBlurLevelChanged(m_mosaicBlurLevel);
}

void PaintBoard::setNumberAutoIncrement(bool enabled)
{
    if (m_numberAutoIncrement == enabled)
        return;
    m_numberAutoIncrement = enabled;
    emit numberAutoIncrementChanged(m_numberAutoIncrement);
}

void PaintBoard::setNumberValue(int value)
{
    const int normalized = qBound(1, value, 9999);
    if (m_numberValue == normalized)
        return;
    m_numberValue = normalized;
    emit numberValueChanged(m_numberValue);
}

void PaintBoard::undo()
{
    if (!m_shapes.isEmpty()) {
        delete m_shapes.last();
        m_shapes.removeLast();
        update();
    }
}

void PaintBoard::reset()
{
    qDeleteAll(m_shapes);
    m_shapes.clear();

    delete m_currentShape;
    m_currentShape = nullptr;
    m_drawing = false;

    m_background_img = QImage();
    update();
}

QImage PaintBoard::renderToImage() const
{
    if (m_shapes.isEmpty())
        return QImage();

    QImage img(qRound(width()), qRound(height()), QImage::Format_ARGB32);
    img.fill(Qt::transparent);

    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    for (Shape* shape : m_shapes) {
        if (shape)
            shape->draw(&painter);
    }
    painter.end();
    return img;
}

void PaintBoard::paint(QPainter *painter)
{
    drawBkImg(painter, m_background_img);
    drawShape(painter);
}

void PaintBoard::mousePressEvent(QMouseEvent *event)
{
    m_startPoint = event->position().toPoint();
    m_drawing = true;

    if (m_currentType == TEXT) {
        // Text insertion is handled by TZWindow popup flow.
        m_drawing = false;
        return;
    }

    if (m_currentType == NUMBER) {
        const int drawNumber = m_numberValue;
        if (m_numberAutoIncrement)
            setNumberValue(m_numberValue + 1);

        m_currentShape = m_factory.createNumberShape(
            m_startPoint,
            drawNumber,
            m_currentColor,
            m_currentSize
        );
        m_shapes.append(m_currentShape);
        m_currentShape = nullptr;
        m_drawing = false;
        update();
        return;
    }

    if (m_currentType == MOSAIC) {
        const int blurScale = qMax(1, m_mosaicBlurLevel);
        auto* mosaicShape = new MosaicShape(
            m_startPoint,
            m_currentSize * 5 * blurScale,
            qMax(2, m_currentSize * blurScale)
        );

        QImage snapshot = m_background_img;
        qDebug() << "[PaintBoard][MOSAIC] press at" << m_startPoint
                 << "snapshotNull=" << snapshot.isNull()
                 << "snapshotSize=" << snapshot.size()
                 << "penSize=" << m_currentSize
                 << "blurLevel=" << m_mosaicBlurLevel;
        mosaicShape->setCanvasSnapshot(snapshot);
        m_currentShape = mosaicShape;
        return;
    }

    m_currentShape = m_factory.createShape(
        m_currentType,
        m_startPoint,
        m_startPoint,
        m_currentColor,
        m_currentSize
    );
}

void PaintBoard::mouseMoveEvent(QMouseEvent *event)
{
    if (!m_drawing || !m_currentShape)
        return;

    m_currentShape->setEndPoint(event->position().toPoint());
    update();
}

void PaintBoard::mouseReleaseEvent(QMouseEvent *event)
{
    if (!m_drawing || !m_currentShape)
        return;

    m_currentShape->setEndPoint(event->position().toPoint());
    m_shapes.append(m_currentShape);
    m_currentShape = nullptr;
    m_drawing = false;
    update();
}

void PaintBoard::hoverMoveEvent(QHoverEvent *event)
{
    Q_UNUSED(event)
}

bool PaintBoard::drawBkImg(QPainter *painter, QImage img)
{
    if (!painter || img.isNull())
        return false;

    QRectF drawRect = img.rect();
    drawRect.moveCenter(boundingRect().center());
    painter->drawImage(drawRect, img);
    return true;
}

bool PaintBoard::drawShape(QPainter *painter)
{
    if (!painter)
        return false;

    for (Shape* shape : m_shapes) {
        if (shape)
            shape->draw(painter);
    }

    if (m_currentShape)
        m_currentShape->draw(painter);

    return true;
}
