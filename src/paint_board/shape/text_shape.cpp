#include "text_shape.h"

#include <QFont>
#include <QFontMetrics>
#include <QStringList>

namespace {

QFont textShapeFont(int size)
{
    QFont font(QStringLiteral("Microsoft YaHei"));
    font.setStyleStrategy(QFont::PreferAntialias);
    font.setHintingPreference(QFont::PreferNoHinting);
    font.setPixelSize(qMax(12, size * 4));
    return font;
}

QRect textShapeRect(const QPoint &point, const QString &text, int size)
{
    const QFont font = textShapeFont(size);
    const QFontMetrics fm(font);
    const QStringList lines = text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);

    int textWidth = 0;
    for (const QString &line : lines) {
        const QString lineForMeasure = line.isEmpty() ? QStringLiteral(" ") : line;
        textWidth = qMax(textWidth, fm.horizontalAdvance(lineForMeasure));
    }

    const int lineCount = qMax(1, lines.size());
    const int textHeight = fm.height() + qMax(0, lineCount - 1) * fm.lineSpacing();
    const int padX = qMax(4, size);
    const int padY = qMax(2, size / 2);
    return QRect(point.x(),
                 point.y() - fm.ascent() - padY,
                 textWidth + padX * 2,
                 textHeight + padY * 2);
}

}

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
    const QFont f = textShapeFont(m_size);
    painter->setFont(f);

    const QFontMetrics fm(f);
    const QStringList lines = m_text.split(QLatin1Char('\n'), Qt::KeepEmptyParts);
    const int padX = qMax(4, m_size);
    const int padY = qMax(2, m_size / 2);
    const QRect drawRect = textShapeRect(m_point, m_text, m_size);

    if (m_withBackground) {
        QColor bg(0, 0, 0, 140);
        painter->setPen(Qt::NoPen);
        painter->setBrush(bg);
        painter->drawRoundedRect(drawRect, 4, 4);
    }

    painter->setPen(m_color);
    int baselineY = drawRect.y() + padY + fm.ascent();
    const int textX = drawRect.x() + padX;
    for (const QString &line : lines) {
        painter->drawText(QPoint(textX, baselineY), line);
        baselineY += fm.lineSpacing();
    }
    painter->restore();
}

QRect TextShape::boundingRect() const
{
    return textShapeRect(m_point, m_text, m_size);
}

bool TextShape::contains(const QPoint &point) const
{
    return boundingRect().adjusted(-2, -2, 2, 2).contains(point);
}
