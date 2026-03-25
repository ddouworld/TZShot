import QtQuick
import QtQuick.Window

Window {
    id: longFrame
    visible: false
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowTransparentForInput

    property rect captureRect: Qt.rect(0, 0, 0, 0)

    x: captureRect.x - 2
    y: captureRect.y - 2
    width: Math.max(0, captureRect.width) + 4
    height: Math.max(0, captureRect.height) + 4

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.color: "#0EA5E9"
        border.width: 2
        radius: 2

        Rectangle {
            x: 0
            y: 0
            width: 6
            height: 6
            color: "#0EA5E9"
        }
        Rectangle {
            x: parent.width - width
            y: 0
            width: 6
            height: 6
            color: "#0EA5E9"
        }
        Rectangle {
            x: 0
            y: parent.height - height
            width: 6
            height: 6
            color: "#0EA5E9"
        }
        Rectangle {
            x: parent.width - width
            y: parent.height - height
            width: 6
            height: 6
            color: "#0EA5E9"
        }
    }
}
