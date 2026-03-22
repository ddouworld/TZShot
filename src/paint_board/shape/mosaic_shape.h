#ifndef MOSAICSHAPE_H
#define MOSAICSHAPE_H

#include "shape.h"
#include <QImage>
#include <QVector>
#include <QHash>
#include <QPoint>
#include <QColor>

// QPoint 作为 QHash key 需要 qHash 支持
inline uint qHash(const QPoint& p, uint seed = 0)
{
    return qHash(QPair<int,int>(p.x(), p.y()), seed);
}

// 马赛克图形：沿鼠标路径对当前画布做像素化处理
// 不再依赖预生成的底图，而是实时从当前画布读取像素
class MosaicShape : public Shape
{
public:
    // blockSize: 马赛克块的像素大小（笔刷半径区域内每块的边长）
    MosaicShape(const QPoint& startPoint,
                int brushRadius = 20,
                int blockSize   = 10);

    void setEndPoint(const QPoint& point) override;

    void draw(QPainter* painter) override;

    // 设置当前画布的快照，用于马赛克取色
    // 每次绘制前由 PaintBoard 调用
    void setCanvasSnapshot(const QImage& snapshot);

private:
    // 将 center 为中心、半径 m_brushRadius 的圆形区域内所有块写入 m_blocks
    void applyMosaicAt(const QPoint& center);

    // key: 块左上角坐标（以 blockSize 对齐），value: 该块的代表色
    // 只存已被马赛克处理的块，draw() 时仅绘制这些色块
    QHash<QPoint, QColor> m_blocks;

    QImage m_canvasSnapshot;        // 当前画布快照，实时获取
    QVector<QPoint> m_points;       // 路径点列表
    int             m_brushRadius;  // 笔刷半径（像素）
    int             m_blockSize;    // 每个马赛克块的边长（像素）
};

#endif // MOSAICSHAPE_H
