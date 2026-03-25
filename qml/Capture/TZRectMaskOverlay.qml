import QtQuick

Item {
    id: overlay

    property rect holeRect: Qt.rect(0, 0, 0, 0)
    property color maskColor: "#99000000"

    readonly property rect normalizedHoleRect: {
        const left = Math.min(holeRect.x, holeRect.x + holeRect.width)
        const right = Math.max(holeRect.x, holeRect.x + holeRect.width)
        const top = Math.min(holeRect.y, holeRect.y + holeRect.height)
        const bottom = Math.max(holeRect.y, holeRect.y + holeRect.height)
        return Qt.rect(left, top, Math.max(0, right - left), Math.max(0, bottom - top))
    }

    Rectangle {
        x: 0
        y: 0
        width: overlay.width
        height: Math.max(0, overlay.normalizedHoleRect.y)
        color: overlay.maskColor
    }

    Rectangle {
        x: 0
        y: overlay.normalizedHoleRect.y
        width: Math.max(0, overlay.normalizedHoleRect.x)
        height: Math.max(0, overlay.normalizedHoleRect.height)
        color: overlay.maskColor
    }

    Rectangle {
        x: overlay.normalizedHoleRect.x + overlay.normalizedHoleRect.width
        y: overlay.normalizedHoleRect.y
        width: Math.max(0, overlay.width - x)
        height: Math.max(0, overlay.normalizedHoleRect.height)
        color: overlay.maskColor
    }

    Rectangle {
        x: 0
        y: overlay.normalizedHoleRect.y + overlay.normalizedHoleRect.height
        width: overlay.width
        height: Math.max(0, overlay.height - y)
        color: overlay.maskColor
    }
}
