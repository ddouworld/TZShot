#include "widgets/magnifier_widget.h"

#include <QPainter>

namespace {
constexpr int kFrameSize = 130;
constexpr int kInfoHeight = 48;
constexpr int kOuterWidth = 130;
constexpr int kOuterHeight = 180;
constexpr int kLensPadding = 10;
constexpr int kSampleRadius = 11;
constexpr int kOffset = 15;
}

MagnifierWidget::MagnifierWidget(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setAttribute(Qt::WA_OpaquePaintEvent, true);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setFixedSize(kOuterWidth, kOuterHeight);
    hide();
}

void MagnifierWidget::setSnapshot(const QPixmap &snapshot, const QRect &virtualGeometry)
{
    m_snapshot = snapshot;
    m_virtualGeometry = virtualGeometry;
    update();
}

void MagnifierWidget::updateView(const QPoint &localPoint,
                                 const QPoint &globalPoint,
                                 const QColor &pixelColor,
                                 const QSize &overlaySize)
{
    m_localPoint = localPoint;
    m_globalPoint = globalPoint;
    m_pixelColor = pixelColor;

    int x = localPoint.x() + kOffset;
    if (x + width() > overlaySize.width()) {
        x = localPoint.x() - width() - kOffset;
    }
    int y = localPoint.y() + kOffset;
    if (y + height() > overlaySize.height()) {
        y = localPoint.y() - height() - kOffset;
    }
    const QPoint nextPos(x, y);
    if (pos() != nextPos) {
        move(nextPos);
    }
    if (!isVisible()) {
        show();
    }
    raise();
    update();
}

void MagnifierWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const QRect frameRect(0, 0, kFrameSize, kFrameSize);
    const QRect infoRect(0, kFrameSize + 2, kOuterWidth, kInfoHeight);

    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(QColor(26, 26, 26));
    painter.drawRoundedRect(frameRect, 4, 4);

    const QRect innerRect(kLensPadding, kLensPadding, 110, 110);
    if (!m_snapshot.isNull()) {
        const QRect srcRect(m_localPoint.x() - kSampleRadius,
                            m_localPoint.y() - kSampleRadius,
                            kSampleRadius * 2,
                            kSampleRadius * 2);
        const QSize sampleSize(srcRect.width(), srcRect.height());
        QPixmap region(sampleSize);
        region.fill(Qt::black);

        QPainter regionPainter(&region);
        const QRect bounded = srcRect.intersected(m_snapshot.rect());
        if (!bounded.isEmpty()) {
            const int destX = bounded.x() - srcRect.x();
            const int destY = bounded.y() - srcRect.y();
            regionPainter.drawPixmap(destX,
                                     destY,
                                     m_snapshot,
                                     bounded.x(),
                                     bounded.y(),
                                     bounded.width(),
                                     bounded.height());
        }
        regionPainter.end();

        painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
        painter.drawPixmap(innerRect, region, region.rect());

        const int sampleCount = sampleSize.width();
        const qreal cellW = qreal(innerRect.width()) / qreal(sampleCount);
        const qreal cellH = qreal(innerRect.height()) / qreal(sampleCount);
        painter.setPen(QPen(QColor(255, 255, 255, 38), 1));
        for (int i = 1; i < sampleCount; ++i) {
            const int x = qRound(innerRect.left() + i * cellW);
            const int y = qRound(innerRect.top() + i * cellH);
            painter.drawLine(x, innerRect.top(), x, innerRect.bottom());
            painter.drawLine(innerRect.left(), y, innerRect.right(), y);
        }

        painter.setPen(QPen(QColor(255, 0, 0, 220), 1));
        const int centerX = innerRect.center().x();
        const int centerY = innerRect.center().y();
        painter.drawLine(centerX, innerRect.top(), centerX, innerRect.bottom());
        painter.drawLine(innerRect.left(), centerY, innerRect.right(), centerY);
    }

    painter.setPen(QPen(QColor("#555555"), 1));
    painter.setBrush(QColor(26, 26, 26, 204));
    painter.drawRoundedRect(infoRect, 4, 4);

    const QRect swatchRect(10, infoRect.y() + 11, 16, 16);
    painter.setPen(QPen(Qt::white, 1));
    painter.setBrush(m_pixelColor);
    painter.drawRoundedRect(swatchRect, 2, 2);

    painter.setPen(Qt::white);
    QFont textFont = painter.font();
    textFont.setPixelSize(11);
    textFont.setFamily(QStringLiteral("Consolas"));
    painter.setFont(textFont);
    const QString rgbText = QStringLiteral("R:%1 G:%2 B:%3")
                                .arg(m_pixelColor.red())
                                .arg(m_pixelColor.green())
                                .arg(m_pixelColor.blue());
    painter.drawText(QRect(34, infoRect.y() + 4, 90, 18),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     rgbText);

    painter.setPen(QColor("#CBD5E1"));
    QFont coordFont = painter.font();
    coordFont.setPixelSize(10);
    coordFont.setFamily(QStringLiteral("Consolas"));
    painter.setFont(coordFont);
    const QString pointText = QStringLiteral("X:%1  Y:%2")
                                  .arg(m_globalPoint.x())
                                  .arg(m_globalPoint.y());
    painter.drawText(QRect(34, infoRect.y() + 22, 90, 16),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     pointText);
}
