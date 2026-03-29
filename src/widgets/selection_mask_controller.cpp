#include "widgets/selection_mask_controller.h"

#include <QCoreApplication>
#include <QPainter>
#include <QPainterPath>

namespace {

bool nearValue(int value, int target, int margin)
{
    return value >= target - margin && value <= target + margin;
}

bool isResizeMode(SelectionMaskController::DragMode mode)
{
    return mode != SelectionMaskController::DragMode::None
        && mode != SelectionMaskController::DragMode::Selecting
        && mode != SelectionMaskController::DragMode::Moving;
}

}

void SelectionMaskController::reset()
{
    m_rect = QRect();
    m_pressRect = QRect();
    m_pressPos = QPoint();
    m_dragMode = DragMode::None;
    m_hoverMode = DragMode::None;
}

void SelectionMaskController::beginInteraction(const QPoint &pos)
{
    m_pressPos = pos;
    m_pressRect = rect();

    if (hasSelection()) {
        const DragMode hit = hitTest(pos);
        if (hit != DragMode::None) {
            m_dragMode = hit;
            return;
        }
    }

    m_dragMode = DragMode::Selecting;
    m_rect = QRect(pos, pos);
}

void SelectionMaskController::updateInteraction(const QPoint &pos, const QSize &bounds)
{
    const QPoint clampedPos = clampPointToBounds(pos, bounds);

    switch (m_dragMode) {
    case DragMode::Selecting:
        m_rect = clampRectToBounds(QRect(m_pressPos, clampedPos).normalized(), bounds);
        break;
    case DragMode::Moving: {
        QRect moved = m_pressRect.translated(clampedPos - m_pressPos);
        if (moved.left() < 0) moved.moveLeft(0);
        if (moved.top() < 0) moved.moveTop(0);
        if (moved.right() > bounds.width()) moved.moveRight(bounds.width());
        if (moved.bottom() > bounds.height()) moved.moveBottom(bounds.height());
        m_rect = moved;
        break;
    }
    case DragMode::ResizeTopLeft: {
        QRect r = m_pressRect;
        r.setTopLeft(clampedPos);
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeTop: {
        QRect r = m_pressRect;
        r.setTop(clampedPos.y());
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeTopRight: {
        QRect r = m_pressRect;
        r.setTopRight(clampedPos);
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeRight: {
        QRect r = m_pressRect;
        r.setRight(clampedPos.x());
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeBottomRight: {
        QRect r = m_pressRect;
        r.setBottomRight(clampedPos);
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeBottom: {
        QRect r = m_pressRect;
        r.setBottom(clampedPos.y());
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeBottomLeft: {
        QRect r = m_pressRect;
        r.setBottomLeft(clampedPos);
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::ResizeLeft: {
        QRect r = m_pressRect;
        r.setLeft(clampedPos.x());
        m_rect = clampRectToBounds(r.normalized(), bounds);
        break;
    }
    case DragMode::None:
        break;
    }
}

void SelectionMaskController::endInteraction()
{
    if (m_rect.width() < 2 || m_rect.height() < 2) {
        m_rect = QRect();
    }
    m_dragMode = DragMode::None;
}

void SelectionMaskController::updateHover(const QPoint &pos)
{
    if (!hasSelection()) {
        m_hoverMode = DragMode::None;
        return;
    }
    m_hoverMode = hitTest(pos);
}

Qt::CursorShape SelectionMaskController::cursorShape() const
{
    const DragMode mode = (m_dragMode != DragMode::None) ? m_dragMode : m_hoverMode;
    switch (mode) {
    case DragMode::Moving: return Qt::SizeAllCursor;
    case DragMode::ResizeTopLeft:
    case DragMode::ResizeBottomRight: return Qt::SizeFDiagCursor;
    case DragMode::ResizeTopRight:
    case DragMode::ResizeBottomLeft: return Qt::SizeBDiagCursor;
    case DragMode::ResizeTop:
    case DragMode::ResizeBottom: return Qt::SizeVerCursor;
    case DragMode::ResizeLeft:
    case DragMode::ResizeRight: return Qt::SizeHorCursor;
    case DragMode::Selecting:
    case DragMode::None:
        return Qt::CrossCursor;
    }
    return Qt::CrossCursor;
}

void SelectionMaskController::paint(QPainter &painter, const QRect &canvasRect) const
{
    const QRect selection = rect();
    const DragMode activeMode = (m_dragMode != DragMode::None) ? m_dragMode : m_hoverMode;

    painter.setBrush(QColor(0, 0, 0, 120));
    painter.setPen(Qt::NoPen);

    QPainterPath path;
    path.addRect(canvasRect.adjusted(-1, -1, 1, 1));
    if (!selection.isEmpty()) {
        path.addRect(selection);
    }
    painter.drawPath(path);

    if (selection.isEmpty()) {
        return;
    }

    painter.setBrush(Qt::NoBrush);
    painter.setPen(QPen(QColor(255, 255, 255, 235), 2.0));
    painter.drawRect(selection);
    painter.setPen(QPen(QColor(37, 99, 235, 235), activeMode == DragMode::Moving ? 1.6 : 1.2));
    painter.drawRect(selection);

    const QString text = QCoreApplication::translate("SelectionMaskController",
                                                     "X:%1  Y:%2  宽:%3  高:%4")
                             .arg(selection.x()).arg(selection.y())
                             .arg(selection.width()).arg(selection.height());
    QFont font = painter.font();
    font.setPointSizeF(font.pointSizeF() > 0 ? font.pointSizeF() + 1.5 : 12.5);
    font.setBold(true);
    painter.setFont(font);
    const QFontMetrics fm(font);
    const int textWidth = fm.horizontalAdvance(text);
    const int textHeight = fm.height();

    QRect labelRect;
    if (selection.y() - 25 >= 0) {
        labelRect = QRect(selection.x(), selection.y() - 32, textWidth + 22, textHeight + 10);
    } else {
        labelRect = QRect(selection.x() + 2, selection.y() + 2, textWidth + 22, textHeight + 10);
    }

    painter.setPen(QPen(QColor(125, 211, 252, 210), 1));
    painter.setBrush(QColor(15, 23, 42, 210));
    painter.drawRoundedRect(labelRect, 5, 5);
    painter.setPen(Qt::white);
    painter.setBrush(Qt::NoBrush);
    painter.drawText(labelRect, Qt::AlignCenter, text);

    const QList<DragMode> handleModes {
        DragMode::ResizeTopLeft,
        DragMode::ResizeTop,
        DragMode::ResizeTopRight,
        DragMode::ResizeRight,
        DragMode::ResizeBottomRight,
        DragMode::ResizeBottom,
        DragMode::ResizeBottomLeft,
        DragMode::ResizeLeft
    };

    painter.setPen(Qt::NoPen);
    for (DragMode mode : handleModes) {
        const QRect r = handleRect(mode);
        if (r.isEmpty()) {
            continue;
        }

        const bool highlighted = (activeMode == mode);
        const QRect outerRect = r.adjusted(-1, -1, 1, 1);
        painter.setBrush(QColor(255, 255, 255, highlighted ? 255 : 235));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(outerRect);
        painter.setBrush(highlighted ? QColor("#2563EB") : QColor("#0EA5E9"));
        painter.drawEllipse(r);
        if (highlighted) {
            painter.setBrush(QColor(37, 99, 235, 50));
            painter.drawEllipse(outerRect.adjusted(-2, -2, 2, 2));
        }
    }

    if (activeMode != DragMode::None && activeMode != DragMode::Selecting && activeMode != DragMode::Moving) {
        painter.setPen(QPen(QColor(37, 99, 235, 180), 1, Qt::DashLine));
        switch (activeMode) {
        case DragMode::ResizeTop:
            painter.drawLine(selection.topLeft(), selection.topRight());
            break;
        case DragMode::ResizeBottom:
            painter.drawLine(selection.bottomLeft(), selection.bottomRight());
            break;
        case DragMode::ResizeLeft:
            painter.drawLine(selection.topLeft(), selection.bottomLeft());
            break;
        case DragMode::ResizeRight:
            painter.drawLine(selection.topRight(), selection.bottomRight());
            break;
        case DragMode::ResizeTopLeft:
            painter.drawLine(selection.topLeft(), selection.topRight());
            painter.drawLine(selection.topLeft(), selection.bottomLeft());
            break;
        case DragMode::ResizeTopRight:
            painter.drawLine(selection.topLeft(), selection.topRight());
            painter.drawLine(selection.topRight(), selection.bottomRight());
            break;
        case DragMode::ResizeBottomLeft:
            painter.drawLine(selection.topLeft(), selection.bottomLeft());
            painter.drawLine(selection.bottomLeft(), selection.bottomRight());
            break;
        case DragMode::ResizeBottomRight:
            painter.drawLine(selection.topRight(), selection.bottomRight());
            painter.drawLine(selection.bottomLeft(), selection.bottomRight());
            break;
        default:
            break;
        }
    }
}

QRect SelectionMaskController::clampRectToBounds(const QRect &rect, const QSize &bounds)
{
    QRect r = rect;
    if (r.left() < 0) r.moveLeft(0);
    if (r.top() < 0) r.moveTop(0);
    if (r.right() > bounds.width()) r.setRight(bounds.width());
    if (r.bottom() > bounds.height()) r.setBottom(bounds.height());
    return r.normalized();
}

QPoint SelectionMaskController::clampPointToBounds(const QPoint &point, const QSize &bounds)
{
    return QPoint(qBound(0, point.x(), bounds.width()),
                  qBound(0, point.y(), bounds.height()));
}

SelectionMaskController::DragMode SelectionMaskController::hitTest(const QPoint &pos) const
{
    const QRect r = rect();
    if (r.isEmpty()) {
        return DragMode::None;
    }

    const int left = r.left();
    const int top = r.top();
    const int right = r.right();
    const int bottom = r.bottom();
    const int x = pos.x();
    const int y = pos.y();

    if (nearValue(x, left, m_hitMargin) && nearValue(y, top, m_hitMargin)) {
        return DragMode::ResizeTopLeft;
    }
    if (nearValue(y, top, m_hitMargin) && x >= left && x <= right) {
        return DragMode::ResizeTop;
    }
    if (nearValue(x, right, m_hitMargin) && nearValue(y, top, m_hitMargin)) {
        return DragMode::ResizeTopRight;
    }
    if (nearValue(x, right, m_hitMargin) && y >= top && y <= bottom) {
        return DragMode::ResizeRight;
    }
    if (nearValue(x, right, m_hitMargin) && nearValue(y, bottom, m_hitMargin)) {
        return DragMode::ResizeBottomRight;
    }
    if (nearValue(y, bottom, m_hitMargin) && x >= left && x <= right) {
        return DragMode::ResizeBottom;
    }
    if (nearValue(x, left, m_hitMargin) && nearValue(y, bottom, m_hitMargin)) {
        return DragMode::ResizeBottomLeft;
    }
    if (nearValue(x, left, m_hitMargin) && y >= top && y <= bottom) {
        return DragMode::ResizeLeft;
    }
    if (isMoveHit(pos)) {
        return DragMode::Moving;
    }
    return DragMode::None;
}

bool SelectionMaskController::isMoveHit(const QPoint &pos) const
{
    const QRect r = rect();
    if (r.isEmpty()) {
        return false;
    }
    return pos.x() > r.left() + 1
        && pos.y() > r.top() + 1
        && pos.x() < r.right() - 1
        && pos.y() < r.bottom() - 1;
}

QRect SelectionMaskController::handleRect(DragMode mode) const
{
    const QRect r = rect();
    if (r.isEmpty() || !isResizeMode(mode)) {
        return {};
    }

    QPoint center;
    switch (mode) {
    case DragMode::ResizeTopLeft:
        center = r.topLeft();
        break;
    case DragMode::ResizeTop:
        center = QPoint(r.center().x(), r.top());
        break;
    case DragMode::ResizeTopRight:
        center = r.topRight();
        break;
    case DragMode::ResizeRight:
        center = QPoint(r.right(), r.center().y());
        break;
    case DragMode::ResizeBottomRight:
        center = r.bottomRight();
        break;
    case DragMode::ResizeBottom:
        center = QPoint(r.center().x(), r.bottom());
        break;
    case DragMode::ResizeBottomLeft:
        center = r.bottomLeft();
        break;
    case DragMode::ResizeLeft:
        center = QPoint(r.left(), r.center().y());
        break;
    default:
        return {};
    }

    return QRect(center.x() - m_handleSize / 2,
                 center.y() - m_handleSize / 2,
                 m_handleSize,
                 m_handleSize);
}
