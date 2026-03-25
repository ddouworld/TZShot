import QtQuick
import QtQuick.Window


Rectangle{
    id:handleGroup
    property rect area: Qt.rect(0,0,0,0)
    property rect initialArea: Qt.rect(0,0,0,0)
    property var mousePointStart: Qt.point(0,0)
    property var coordinateItem: null
    x:area.x
    y:area.y
    width: area.width
    height: area.height
    color: "transparent"
    function pointInCoordinateItem(item, x, y) {
        if (coordinateItem)
            return item.mapToItem(coordinateItem, x, y)
        return item.mapToItem(null, x, y)
    }
    //左上角控制点
    TZResizeHandle
    {
        id:leftTop
        x:  - handleSize/2
        y: - handleSize/2
        // 仅当选区宽度/高度非零时显示
        visible: area.width !== 0 || area.height !== 0
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(leftTop, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                // 重置所有临时变量
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(leftTop, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    let dy = globalPos.y - mousePointStart.y
                    area.x = initialArea.x + dx
                    area.y = initialArea.y + dy
                    area.width = initialArea.width - dx
                    area.height = initialArea.height - dy
                }
            }
        }
    }

    //正上方控制点
    TZResizeHandle
    {
        id: top
        x:  (area.width - handleSize/2)/2
        y:  -handleSize/2
        // 仅当选区宽度/高度非零时显示
        visible: area.width !== 0 || area.height !== 0
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(top, mouseX, mouseY);
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                // 重置所有临时变量
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(top, mouseX, mouseY);
                    let dy = globalPos.y - mousePointStart.y
                    area.y = initialArea.y + dy
                    area.height = initialArea.height - dy
                }
            }
        }
    }

    //右上角控制点
    TZResizeHandle
    {
        id:rightTop
        x:  area.width - handleSize/2
        y:  -handleSize/2
        visible: area.width !== 0 || area.height !== 0
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeBDiagCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(rightTop, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(rightTop, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    let dy = globalPos.y - mousePointStart.y
                    area.y = initialArea.y + dy
                    area.width = initialArea.width + dx
                    area.height = initialArea.height - dy
                }
            }
        }


    }

    //右边控制点
    TZResizeHandle
    {
        id: right
        x:  area.width - handleSize/2
        y:  (area.height -handleSize/2)/2
        visible: area.width !== 0 || area.height !== 0
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(right, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(right, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    area.width = initialArea.width + dx
                }
            }
        }


    }

    //右下角控制点
    TZResizeHandle
    {
        id:rightButtom
        x: area.width - handleSize/2
        y: area.height - handleSize/2
        visible: area.width !== 0 || area.height !== 0
        z:2
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeFDiagCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(rightButtom, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(rightButtom, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    let dy = globalPos.y - mousePointStart.y
                    area.width = initialArea.width + dx
                    area.height = initialArea.height + dy
                }
            }
        }
    }

    //正下方控制点
    TZResizeHandle
    {
        id:buttom
        x: (area.width - handleSize/2)/2
        y: area.height - handleSize/2
        visible: area.width !== 0 || area.height !== 0
        z:2
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeVerCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(buttom, mouseX, mouseY);
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(buttom, mouseX, mouseY);
                    let dy = globalPos.y - mousePointStart.y
                    area.height = initialArea.height + dy
                }
            }
        }
    }

    //左下角控制点
    TZResizeHandle
    {
        id:leftBottom
        x:  -handleSize/2
        y:  area.height - handleSize/2
        visible: area.width !== 0 || area.height !== 0
        z:2
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeBDiagCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(leftBottom, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                mousePointStart.y = globalPos.y
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(leftBottom, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    let dy = globalPos.y - mousePointStart.y
                    area.x = initialArea.x + dx
                    area.width = initialArea.width - dx
                    area.height = initialArea.height + dy
                }
            }
        }

    }

    //左边控制点
    TZResizeHandle
    {
        id:left
        x:  -handleSize/2
        y:  (area.height - handleSize/2)/2
        visible: area.width !== 0 || area.height !== 0
        z:2
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.SizeHorCursor
            onPressed: {
                let globalPos = handleGroup.pointInCoordinateItem(left, mouseX, mouseY);
                mousePointStart.x = globalPos.x
                initialArea = area
            }

            onReleased: {
                mousePointStart = Qt.point(0,0)
                initialArea = area
            }

            onPositionChanged: {
                if (pressed) {
                    let globalPos = handleGroup.pointInCoordinateItem(left, mouseX, mouseY);
                    let dx = globalPos.x - mousePointStart.x
                    area.x = initialArea.x + dx
                    area.width = initialArea.width - dx
                }
            }
        }

    }

}
