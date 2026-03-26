#include "mosaic_shape.h"
#include <QPainter>

MosaicShape::MosaicShape(const QPoint& startPoint,
                         int brushRadius,
                         int blockSize)
    : Shape(Qt::transparent, blockSize)
    , m_brushRadius(brushRadius)
    , m_blockSize(blockSize)
{
    m_points.append(startPoint);
    // 注意：此时还没有画布快照，需要在 setCanvasSnapshot 后才能真正应用马赛克
}

void MosaicShape::setCanvasSnapshot(const QImage& snapshot)
{
    if (snapshot.isNull())
        return;
    
    // 转换为 ARGB32 格式以便快速像素访问
    if (snapshot.format() != QImage::Format_ARGB32) {
        m_canvasSnapshot = snapshot.convertToFormat(QImage::Format_ARGB32);
    } else {
        m_canvasSnapshot = snapshot;
    }
    
    // 如果有待处理的点，现在应用马赛克
    if (!m_points.isEmpty() && m_blocks.isEmpty()) {
        for (const QPoint& point : m_points) {
            applyMosaicAt(point);
        }
    }
}

void MosaicShape::setEndPoint(const QPoint& point)
{
    if (!m_points.isEmpty() && m_points.last() == point)
        return;
    m_points.append(point);
    
    // 只有在有画布快照的情况下才应用马赛克
    if (!m_canvasSnapshot.isNull()) {
        applyMosaicAt(point);
    }
}

void MosaicShape::draw(QPainter* painter)
{
    if (!painter || m_blocks.isEmpty())
        return;

    painter->save();
    painter->setPen(Qt::NoPen);

    // 只绘制马赛克色块，不绘制任何背景像素，避免覆盖画板上其他图形
    for (auto it = m_blocks.constBegin(); it != m_blocks.constEnd(); ++it) {
        const QPoint& blockOrigin = it.key();
        const QColor& color       = it.value();

        QRectF blockRect(blockOrigin.x(),
                         blockOrigin.y(),
                         m_blockSize,
                         m_blockSize);
        
        // 无需裁剪到背景图范围，因为我们不再依赖背景图
        // 但为了保证安全，可以检查坐标是否为正
        if (blockOrigin.x() < 0 || blockOrigin.y() < 0)
            continue;

        painter->fillRect(blockRect, color);
    }

    painter->restore();
}

void MosaicShape::applyMosaicAt(const QPoint& center)
{
    if (m_canvasSnapshot.isNull())
        return;

    int r    = m_brushRadius;
    int bs   = qMax(1, m_blockSize);
    int imgW = m_canvasSnapshot.width();
    int imgH = m_canvasSnapshot.height();

    // 遍历圆形区域内所有马赛克块，步长为 bs
    int startX = ((center.x() - r) / bs) * bs;
    int startY = ((center.y() - r) / bs) * bs;
    int endX   = center.x() + r;
    int endY   = center.y() + r;

    for (int bx = startX; bx <= endX; bx += bs) {
        for (int by = startY; by <= endY; by += bs) {
            // 判断此块中心是否在笔刷圆形内
            int cx = bx + bs / 2;
            int cy = by + bs / 2;
            int dx = cx - center.x();
            int dy = cy - center.y();
            if (dx * dx + dy * dy > r * r)
                continue;

            int x0 = qMax(0, bx);
            int y0 = qMax(0, by);
            if (x0 >= imgW || y0 >= imgH)
                continue;

            QPoint blockKey(bx, by);

            // 已处理过的块不重复取色，保留首次取色
            if (m_blocks.contains(blockKey))
                continue;

            // 从画布快照取块内左上角像素作为代表色
            QRgb representative = m_canvasSnapshot.pixel(x0, y0);
            m_blocks.insert(blockKey, QColor(representative));
        }
    }
}
