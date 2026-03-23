#include "scroll_stitcher.h"

#include <QPainter>
#include <QtGlobal>

void ScrollStitcher::reset()
{
    m_lastFrame = QImage();
    m_result = QImage();
}

bool ScrollStitcher::hasBase() const
{
    return !m_lastFrame.isNull();
}

void ScrollStitcher::setBaseFrame(const QImage &frame)
{
    m_lastFrame = frame;
    m_result = frame;
}

int ScrollStitcher::appendFrame(const QImage &frame)
{
    if (frame.isNull()) {
        return 0;
    }
    if (m_lastFrame.isNull() || m_result.isNull()) {
        setBaseFrame(frame);
        return frame.height();
    }

    QImage curr = frame;
    if (curr.size() != m_lastFrame.size()) {
        curr = curr.scaled(m_lastFrame.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    const int overlap = findBestOverlap(m_lastFrame, curr);
    if (overlap <= 0 || overlap >= curr.height()) {
        m_lastFrame = curr;
        return 0;
    }

    const int appendHeight = curr.height() - overlap;
    if (appendHeight < 8) {
        m_lastFrame = curr;
        return 0;
    }

    QImage merged(m_result.width(), m_result.height() + appendHeight, QImage::Format_ARGB32_Premultiplied);
    merged.fill(Qt::transparent);

    QPainter painter(&merged);
    painter.drawImage(0, 0, m_result);
    painter.drawImage(
        QRect(0, m_result.height(), curr.width(), appendHeight),
        curr,
        QRect(0, overlap, curr.width(), appendHeight));
    painter.end();

    m_result = merged;
    m_lastFrame = curr;
    return appendHeight;
}

int ScrollStitcher::findBestOverlap(const QImage &prev, const QImage &curr) const
{
    const int w = qMin(prev.width(), curr.width());
    const int h = qMin(prev.height(), curr.height());
    if (w < 80 || h < 80) {
        return -1;
    }

    const int minOverlap = qMax(20, h * 20 / 100);
    const int maxOverlap = qMax(minOverlap + 1, h * 85 / 100);
    const int step = 3;

    int bestOverlap = -1;
    qint64 bestScore = std::numeric_limits<qint64>::max();

    const int left = w * 10 / 100;
    const int right = w * 90 / 100;

    for (int overlap = minOverlap; overlap <= maxOverlap; overlap += 2) {
        qint64 score = 0;
        int samples = 0;

        for (int y = 0; y < overlap; y += step) {
            const int py = prev.height() - overlap + y;
            const int cy = y;
            for (int x = left; x < right; x += step) {
                const int g1 = grayAt(prev, x, py);
                const int g2 = grayAt(curr, x, cy);
                score += qAbs(g1 - g2);
                ++samples;
            }
        }

        if (samples == 0) {
            continue;
        }
        const qint64 norm = score / samples;
        if (norm < bestScore) {
            bestScore = norm;
            bestOverlap = overlap;
        }
    }

    // Empirical threshold for noisy desktop content.
    if (bestScore > 22) {
        return -1;
    }
    return bestOverlap;
}

int ScrollStitcher::grayAt(const QImage &img, int x, int y)
{
    const QRgb p = img.pixel(x, y);
    return (qRed(p) * 30 + qGreen(p) * 59 + qBlue(p) * 11) / 100;
}
