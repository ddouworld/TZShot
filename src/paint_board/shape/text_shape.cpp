#include "text_shape.h"

#include <QFont>
#include <QFontMetrics>

TextShape::TextShape(const QPoint& startPoint,
                     const QString& text,
                     const QColor& color,
                     int size,
                     bool withBackground)
    : Shape(color, size)
    , m_point(startPoint)
    , m_text(text.isEmpty() ? QStringLiteral("Text") : text)
    , m_withBackground(withBackground)
{
}

void TextShape::setEndPoint(const QPoint& point)
{
    m_point = point;
}

void TextShape::draw(QPainter* painter)
{
    if (!painter) return;

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::TextAntialiasing, true);
    QFont f = painter->font();
    f.setPixelSize(qMax(12, m_size * 4));
    painter->setFont(f);

    const QFontMetrics fm(f);
    const QRect textRect = fm.boundingRect(m_text);
    const int padX = qMax(4, m_size);
    const int padY = qMax(2, m_size / 2);
    QRect drawRect(m_point.x(),
                   m_point.y() - textRect.height(),
                   textRect.width() + padX * 2,
                   textRect.height() + padY * 2);

    if (m_withBackground) {
        QColor bg(0, 0, 0, 140);
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(drawRect, 4, 4);
    }

    painter->setPen(m_color);
    const QPoint textPos(drawRect.x() + padX, drawRect.y() + padY + fm.ascent());
    painter->drawText(textPos, m_text);
    painter->restore();
}
