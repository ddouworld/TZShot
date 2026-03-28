#ifndef BLUR_SHAPE_H
#define BLUR_SHAPE_H

#include "shape.h"
#include <QImage>
#include <QRect>
#include <QVector>

class BlurShape : public Shape
{
public:
    BlurShape(const QPoint& startPoint,
              int brushRadius = 20,
              int blurRadius = 8);

    void setEndPoint(const QPoint& point) override;
    void draw(QPainter* painter) override;
    void setCanvasSnapshot(const QImage& snapshot);

private:
    struct BlurPatch {
        QPoint center;
        QRect  rect;
        QImage image;
    };

    void applyBlurAt(const QPoint &center);
    QImage makeBlurredImage(const QImage& source) const;

    QVector<QPoint> m_points;
    QImage m_canvasSnapshot;
    QVector<BlurPatch> m_patches;
    int m_brushRadius;
    int m_blurRadius;
};

#endif // BLUR_SHAPE_H
