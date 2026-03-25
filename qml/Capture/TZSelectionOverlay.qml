import QtQuick

QtObject {
    id: overlay

    property rect area: Qt.rect(0, 0, 0, 0)
    property rect initialArea: Qt.rect(0, 0, 0, 0)
    property point dragOffset: Qt.point(0, 0)
    property string dragMode: "none" // none | select | move
    property var coordinateItem: null

    readonly property rect localArea: {
        if (!coordinateItem)
            return area
        const topLeft = coordinateItem.mapFromGlobal(area.x, area.y)
        return Qt.rect(topLeft.x, topLeft.y, area.width, area.height)
    }

    readonly property rect localNormalizedArea: {
        const bounds = getNormalizedRectBounds(localArea)
        return bounds.isValid
            ? Qt.rect(bounds.left, bounds.top, bounds.width, bounds.height)
            : Qt.rect(0, 0, 0, 0)
    }

    readonly property rect normalizedArea: {
        const bounds = getNormalizedRectBounds(area)
        return bounds.isValid
            ? Qt.rect(bounds.left, bounds.top, bounds.width, bounds.height)
            : Qt.rect(0, 0, 0, 0)
    }

    function hasArea() {
        return Math.abs(area.width) > 2 && Math.abs(area.height) > 2
    }

    function getNormalizedRectBounds(targetRect) {
        if (targetRect.width === 0 || targetRect.height === 0) {
            return { left: 0, right: 0, top: 0, bottom: 0, isValid: false }
        }

        const left = Math.min(targetRect.x, targetRect.x + targetRect.width)
        const right = Math.max(targetRect.x, targetRect.x + targetRect.width)
        const top = Math.min(targetRect.y, targetRect.y + targetRect.height)
        const bottom = Math.max(targetRect.y, targetRect.y + targetRect.height)
        return {
            left: left,
            right: right,
            top: top,
            bottom: bottom,
            width: right - left,
            height: bottom - top,
            isValid: true
        }
    }

    function containsPoint(targetRect, x, y) {
        const bounds = getNormalizedRectBounds(targetRect)
        if (!bounds.isValid)
            return false
        return x >= bounds.left && x <= bounds.right
            && y >= bounds.top && y <= bounds.bottom
    }

    function containsCurrentPoint(x, y) {
        return containsPoint(area, x, y)
    }

    function selectionRectFromDrag(startX, startY, endX, endY, withShift, withAlt) {
        let x = startX
        let y = startY
        let w = endX - startX
        let h = endY - startY

        if (withAlt) {
            x = startX - (endX - startX)
            y = startY - (endY - startY)
            w = (endX - startX) * 2
            h = (endY - startY) * 2
        }

        if (withShift) {
            const absW = Math.abs(w)
            const absH = Math.abs(h)
            const side = Math.min(absW, absH)
            w = w < 0 ? -side : side
            h = h < 0 ? -side : side
        }

        return Qt.rect(x, y, w, h)
    }

    function beginSelection(ax, ay) {
        dragMode = "select"
        initialArea = Qt.rect(ax, ay, 0, 0)
        area = Qt.rect(ax, ay, 0, 0)
    }

    function beginMove(ax, ay) {
        dragMode = "move"
        dragOffset = Qt.point(ax, ay)
        initialArea = area
    }

    function updateMove(ax, ay) {
        const dx = ax - dragOffset.x
        const dy = ay - dragOffset.y
        area = Qt.rect(
            initialArea.x + dx,
            initialArea.y + dy,
            initialArea.width,
            initialArea.height
        )
    }

    function updateSelection(ax, ay, modifiers) {
        const useShift = (modifiers & Qt.ShiftModifier) !== 0
        const useAlt = (modifiers & Qt.AltModifier) !== 0
        area = selectionRectFromDrag(
            initialArea.x,
            initialArea.y,
            ax,
            ay,
            useShift,
            useAlt
        )
    }

    function normalizeCurrentArea() {
        const bounds = getNormalizedRectBounds(area)
        if (bounds.isValid) {
            area = Qt.rect(bounds.left, bounds.top, bounds.width, bounds.height)
            initialArea = area
        }
    }

    function endInteraction() {
        dragMode = "none"
    }

    function reset() {
        area = Qt.rect(0, 0, 0, 0)
        initialArea = Qt.rect(0, 0, 0, 0)
        dragOffset = Qt.point(0, 0)
        dragMode = "none"
    }
}
