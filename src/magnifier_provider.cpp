#include "magnifier_provider.h"
#include <QPainter>

MagnifierProvider::MagnifierProvider(ScreenshotViewModel *viewModel)
    : QQuickImageProvider(QQuickImageProvider::Image)
    , m_viewModel(viewModel)
{
}

// requestImage
// ------------
// id 格式："<x>,<y>"（由 QML 传入的逻辑像素鼠标坐标）
QImage MagnifierProvider::requestImage(const QString &id, QSize *size, const QSize & /*requestedSize*/)
{
    const QString cleanId = id.section('?', 0, 0);

    // 解析坐标
    const QStringList parts = cleanId.split(',');
    if (parts.size() < 2) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) *size = blank.size();
        return blank;
    }
    bool okX = false;
    bool okY = false;
    const int cx = parts[0].toInt(&okX);
    const int cy = parts[1].toInt(&okY);
    if (!okX || !okY) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) *size = blank.size();
        return blank;
    }

    // 从桌面快照中取一份线程安全的副本
    const QImage snapshot = m_viewModel->desktopSnapshot();
    if (snapshot.isNull()) {
        QImage blank(kOutputSize, kOutputSize, QImage::Format_ARGB32_Premultiplied);
        blank.fill(Qt::black);
        if (size) *size = blank.size();
        return blank;
    }

    // 逻辑坐标 → 快照像素坐标（按虚拟桌面逻辑尺寸与快照物理像素比例换算，支持多屏）
    const QRect vg = m_viewModel->virtualGeometry();
    const qreal scaleX = (vg.width() > 0)  ? (qreal(snapshot.width())  / qreal(vg.width()))  : 1.0;
    const qreal scaleY = (vg.height() > 0) ? (qreal(snapshot.height()) / qreal(vg.height())) : 1.0;
    const int pcx = qRound((cx - vg.x()) * scaleX);
    const int pcy = qRound((cy - vg.y()) * scaleY);

    // 采样半径按 X 轴比例缩放；Y 轴保持一致比例，保证中心点准确
    const int srcRadiusX = qMax(1, qRound(kSrcRadius * scaleX));
    const int srcRadiusY = qMax(1, qRound(kSrcRadius * scaleY));
    const int srcDiamW   = srcRadiusX * 2;
    const int srcDiamH   = srcRadiusY * 2;

    // 计算裁剪矩形（以物理像素光标为中心）
    QRect srcRect(pcx - srcRadiusX, pcy - srcRadiusY, srcDiamW, srcDiamH);

    // 处理边界：当光标靠近屏幕边缘时，用黑色填充越界区域
    QImage region(srcDiamW, srcDiamH, QImage::Format_ARGB32_Premultiplied);
    region.fill(Qt::black);
    {
        QPainter rp(&region);
        QRect bounded = srcRect.intersected(snapshot.rect());
        if (!bounded.isEmpty()) {
            int destX = bounded.x() - srcRect.x();
            int destY = bounded.y() - srcRect.y();
            rp.drawImage(destX, destY, snapshot, bounded.x(), bounded.y(),
                         bounded.width(), bounded.height());
        }
        rp.end();
    }

    // 放大到 kOutputSize×kOutputSize
    QImage zoomed = region.scaled(kOutputSize, kOutputSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 绘制红色十字准线（1px 宽）
    {
        QPainter p(&zoomed);
        p.setRenderHint(QPainter::Antialiasing, false);
        QPen pen(QColor(255, 0, 0, 220), 1);
        p.setPen(pen);
        const int half = kOutputSize / 2;
        p.drawLine(half, 0, half, kOutputSize - 1);   // 竖线
        p.drawLine(0, half, kOutputSize - 1, half);   // 横线
        p.end();
    }

    if (size) *size = zoomed.size();
    return zoomed;
}
