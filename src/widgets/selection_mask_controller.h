#ifndef SELECTION_MASK_CONTROLLER_H
#define SELECTION_MASK_CONTROLLER_H

#include <QPoint>
#include <QRect>
#include <QSize>
#include <Qt>

class QPainter;

class SelectionMaskController
{
public:
    enum class DragMode {
        None,
        Selecting,
        Moving,
        ResizeTopLeft,
        ResizeTop,
        ResizeTopRight,
        ResizeRight,
        ResizeBottomRight,
        ResizeBottom,
        ResizeBottomLeft,
        ResizeLeft
    };

    void reset();

    QRect rect() const { return m_rect.normalized(); }
    QRect rawRect() const { return m_rect; }
    bool hasSelection() const { return !m_rect.normalized().isEmpty(); }
    DragMode dragMode() const { return m_dragMode; }

    void beginInteraction(const QPoint &pos);
    void updateInteraction(const QPoint &pos, const QSize &bounds);
    void endInteraction();
    void updateHover(const QPoint &pos);

    Qt::CursorShape cursorShape() const;
    void paint(QPainter &painter, const QRect &canvasRect) const;

private:
    static QRect clampRectToBounds(const QRect &rect, const QSize &bounds);
    static QPoint clampPointToBounds(const QPoint &point, const QSize &bounds);

    DragMode hitTest(const QPoint &pos) const;
    bool isMoveHit(const QPoint &pos) const;
    QRect handleRect(DragMode mode) const;

    QRect m_rect;
    QRect m_pressRect;
    QPoint m_pressPos;
    DragMode m_dragMode = DragMode::None;
    DragMode m_hoverMode = DragMode::None;
    int m_hitMargin = 8;
    int m_handleSize = 10;
};

#endif // SELECTION_MASK_CONTROLLER_H
