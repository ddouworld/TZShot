import QtQuick

Rectangle {
    id: highlight

    color: "transparent"
    border.color: "#3B82F6"
    border.width: 2

    Rectangle {
        anchors.fill: parent
        color: "#153B82F6"
    }

    Repeater {
        model: [
            { ax: 0, ay: 0, bx: 1, by: 0, cx: 0, cy: 1 },
            { ax: parent.width, ay: 0, bx: -1, by: 0, cx: 0, cy: 1 },
            { ax: 0, ay: parent.height, bx: 1, by: 0, cx: 0, cy: -1 },
            { ax: parent.width, ay: parent.height, bx: -1, by: 0, cx: 0, cy: -1 }
        ]

        delegate: Item {
            x: modelData.ax - (modelData.bx > 0 ? 0 : 3)
            y: modelData.ay - (modelData.cy > 0 ? 0 : 3)

            Rectangle {
                x: 0
                y: 0
                width: 12
                height: 3
                color: "#3B82F6"
                transform: Scale { xScale: modelData.bx }
            }

            Rectangle {
                x: 0
                y: 0
                width: 3
                height: 12
                color: "#3B82F6"
                transform: Scale { yScale: modelData.cy }
            }
        }
    }
}
