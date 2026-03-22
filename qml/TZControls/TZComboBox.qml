import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: control

    property color fillColor:       "#F8FAFC"
    property color borderColor:     "#E2E8F0"
    property color activeBorderColor: "#3B82F6"
    property color hoverColor:      "#F1F5F9"
    property color textColor:       "#1E293B"
    property color dimTextColor:    "#64748B"

    width: 120
    height: 36
    font.pixelSize: 13
    font.family: "Microsoft YaHei"

    // 当前选中文字
    contentItem: Text {
        leftPadding: 12
        rightPadding: 28
        text: control.displayText
        font: control.font
        color: control.enabled ? textColor : dimTextColor
        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    // 背景
    background: Rectangle {
        implicitWidth: control.width
        implicitHeight: control.height
        color: control.hovered ? hoverColor : fillColor
        border.color: control.activeFocus ? activeBorderColor : borderColor
        border.width: control.activeFocus ? 2 : 1
        radius: 6

        Behavior on border.color { ColorAnimation { duration: 150 } }
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    // 下拉箭头
    indicator: Canvas {
        id: canvas
        x: control.width - width - 10
        y: (control.height - height) / 2
        width: 10
        height: 6
        contextType: "2d"

        onPaint: {
            context.reset()
            context.lineWidth = 1.5
            context.strokeStyle = dimTextColor
            context.lineCap = "round"
            context.lineJoin = "round"
            context.moveTo(0, 0)
            context.lineTo(width / 2, height)
            context.lineTo(width, 0)
            context.stroke()
        }
    }

    // 下拉列表每项
    delegate: ItemDelegate {
        width: control.width
        height: 36

        contentItem: Text {
            leftPadding: 12
            text: modelData
            font: control.font
            color: highlighted ? "#3B82F6" : textColor
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            color: highlighted ? "#EFF6FF" : fillColor
        }
    }

    // 弹出框
    popup: Popup {
        y: control.height + 4
        width: control.width
        implicitHeight: listview.contentHeight
        padding: 4

        contentItem: ListView {
            id: listview
            clip: true
            model: control.popup.visible ? control.delegateModel : null
            currentIndex: control.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }

        background: Rectangle {
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1
            radius: 8

            // 顶部细线装饰
            Rectangle {
                width: parent.width - 2
                height: 1
                anchors.top: parent.top
                anchors.topMargin: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#3B82F6"
                opacity: 0.4
            }
        }

        enter: Transition {
            NumberAnimation { property: "opacity"; from: 0.0; to: 1.0; duration: 120 }
            NumberAnimation { property: "scale";   from: 0.95; to: 1.0; duration: 120 }
        }
        exit: Transition {
            NumberAnimation { property: "opacity"; from: 1.0; to: 0.0; duration: 80 }
        }
    }
}


