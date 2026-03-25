import QtQuick

QtObject {
    id: controller

    property bool spaceHeld: false
    property bool textClickPending: false
    property bool textPressMoved: false
    property point textClickAbsPoint: Qt.point(0, 0)
    property point textClickLocalPoint: Qt.point(0, 0)

    function resetTextClickTracking() {
        textClickPending = false
        textPressMoved = false
        textClickAbsPoint = Qt.point(0, 0)
        textClickLocalPoint = Qt.point(0, 0)
    }

    function beginTextClick(absX, absY, localX, localY) {
        textClickPending = true
        textPressMoved = false
        textClickAbsPoint = Qt.point(absX, absY)
        textClickLocalPoint = Qt.point(localX, localY)
    }

    function updatePendingTextClick(absX, absY, moveThreshold) {
        if (!textClickPending)
            return false

        const threshold = moveThreshold === undefined ? 3 : moveThreshold
        const dx = absX - textClickAbsPoint.x
        const dy = absY - textClickAbsPoint.y
        if (Math.abs(dx) > threshold || Math.abs(dy) > threshold) {
            textPressMoved = true
            textClickPending = false
            return false
        }

        return true
    }

    function shouldOpenTextEditor() {
        return textClickPending && !textPressMoved
    }

    function translatedRect(initialRect, startPoint, currentX, currentY) {
        const dx = currentX - startPoint.x
        const dy = currentY - startPoint.y
        return Qt.rect(
            initialRect.x + dx,
            initialRect.y + dy,
            initialRect.width,
            initialRect.height
        )
    }
}
