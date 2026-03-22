import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5

Rectangle {
    id: tool
    radius: 10
    color: "#FFFFFF"
    visible: false
    border.color: "#E2E8F0"
    border.width: 1
    layer.enabled: true

    signal signalScreenshot
    signal signalCancel
    signal signalSave
    signal signalSticky
    signal signalUndo
    signal signalGifRecord
    signal signalOcr

    property string activeTool: ""
    property bool markToolInfoVisible: false
    property string hoverTipText: ""
    property real hoverTipCenterX: width / 2

    readonly property int shapeType: {
        switch (activeTool) {
        case "pencil": return 0
        case "rect": return 1
        case "circle": return 2
        case "arrow": return 3
        case "mosaic": return 4
        case "text": return 5
        case "number": return 6
        case "highlight": return 7
        default: return 1
        }
    }

    function setActiveTool(name) {
        if (activeTool === name) {
            activeTool = ""
            markToolInfoVisible = false
        } else {
            activeTool = name
            markToolInfoVisible = true
        }
    }

    function showTip(text, targetItem) {
        hoverTipText = text
        if (targetItem) {
            var p = tool.mapFromItem(targetItem, targetItem.width / 2, targetItem.height)
            hoverTipCenterX = p.x
        } else {
            hoverTipCenterX = width / 2
        }
    }

    function hideTip(text) {
        if (text === undefined || hoverTipText === text)
            hoverTipText = ""
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        layoutDirection: Qt.RightToLeft
        spacing: 2

        Rectangle {
            width: 32; height: 32; radius: 7
            color: checkArea.containsMouse ? "#DCFCE7" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_check.svg"; width: 18; height: 18 }
            MouseArea { id: checkArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("完成"), parent); onExited: tool.hideTip(qsTr("完成")); onPressed: signalScreenshot() }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: closeArea.containsMouse ? "#FEE2E2" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_x.svg"; width: 18; height: 18 }
            MouseArea { id: closeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("取消"), parent); onExited: tool.hideTip(qsTr("取消")); onPressed: signalCancel() }
        }

        Rectangle { width: 1; height: 20; color: "#E2E8F0"; Layout.alignment: Qt.AlignVCenter }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: ocrArea.containsMouse ? "#F0FDF4" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_ocr.svg"; width: 18; height: 18 }
            MouseArea { id: ocrArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("OCR"), parent); onExited: tool.hideTip(qsTr("OCR")); onPressed: signalOcr() }
        }

        Rectangle {
            id: gifRecordBtn
            width: 32; height: 32; radius: 7
            color: gifArea.containsMouse ? "#FFF7ED" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_gif_record.svg"; width: 18; height: 18 }
            MouseArea { id: gifArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("录制GIF"), parent); onExited: tool.hideTip(qsTr("录制GIF")); onPressed: signalGifRecord() }
        }

        Rectangle { width: 1; height: 20; color: "#E2E8F0"; Layout.alignment: Qt.AlignVCenter }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: saveArea.containsMouse ? "#EFF6FF" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_save.svg"; width: 18; height: 18 }
            MouseArea { id: saveArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("保存"), parent); onExited: tool.hideTip(qsTr("保存")); onPressed: signalSave() }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: fixArea.containsMouse ? "#EFF6FF" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_pin.svg"; width: 18; height: 18 }
            MouseArea { id: fixArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("贴图"), parent); onExited: tool.hideTip(qsTr("贴图")); onPressed: signalSticky() }
        }

        Rectangle { width: 1; height: 20; color: "#E2E8F0"; Layout.alignment: Qt.AlignVCenter }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "rect") ? "#EFF6FF" : (rectArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "rect") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_square.svg"; width: 18; height: 18 }
            MouseArea { id: rectArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("矩形"), parent); onExited: tool.hideTip(qsTr("矩形")); onPressed: tool.setActiveTool("rect") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "circle") ? "#EFF6FF" : (circleArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "circle") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_circle.svg"; width: 18; height: 18 }
            MouseArea { id: circleArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("圆形"), parent); onExited: tool.hideTip(qsTr("圆形")); onPressed: tool.setActiveTool("circle") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "arrow") ? "#EFF6FF" : (arrowArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "arrow") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_arrow.svg"; width: 18; height: 18 }
            MouseArea { id: arrowArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("箭头"), parent); onExited: tool.hideTip(qsTr("箭头")); onPressed: tool.setActiveTool("arrow") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "pencil") ? "#EFF6FF" : (pencilArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "pencil") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_pencil.svg"; width: 18; height: 18 }
            MouseArea { id: pencilArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("画笔"), parent); onExited: tool.hideTip(qsTr("画笔")); onPressed: tool.setActiveTool("pencil") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "highlight") ? "#EFF6FF" : (highlightArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "highlight") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_highlighter.svg"; width: 18; height: 18 }
            MouseArea { id: highlightArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("高亮"), parent); onExited: tool.hideTip(qsTr("高亮")); onPressed: tool.setActiveTool("highlight") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "mosaic") ? "#EFF6FF" : (mosaicArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "mosaic") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_mosaic.svg"; width: 18; height: 18 }
            MouseArea { id: mosaicArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("马赛克"), parent); onExited: tool.hideTip(qsTr("马赛克")); onPressed: tool.setActiveTool("mosaic") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "text") ? "#EFF6FF" : (textArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "text") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_text.svg"; width: 18; height: 18 }
            MouseArea { id: textArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("文字"), parent); onExited: tool.hideTip(qsTr("文字")); onPressed: tool.setActiveTool("text") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: (tool.activeTool === "number") ? "#EFF6FF" : (numberArea.containsMouse ? "#F1F5F9" : "transparent")
            border.color: (tool.activeTool === "number") ? "#3B82F6" : "transparent"
            border.width: 1
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_list_ordered.svg"; width: 18; height: 18 }
            MouseArea { id: numberArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("序号"), parent); onExited: tool.hideTip(qsTr("序号")); onPressed: tool.setActiveTool("number") }
        }

        Rectangle {
            width: 32; height: 32; radius: 7
            color: undoArea.containsMouse ? "#EFF6FF" : "transparent"
            Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_undo.svg"; width: 18; height: 18 }
            MouseArea { id: undoArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: tool.showTip(qsTr("撤销"), parent); onExited: tool.hideTip(qsTr("撤销")); onPressed: signalUndo() }
        }
    }

    Rectangle {
        id: tipBubble
        visible: tool.visible && tool.hoverTipText.length > 0
        z: 100
        parent: tool.parent
        width: Math.max(44, tipText.implicitWidth + 14)
        height: 24
        radius: 6
        color: "#0F172A"
        opacity: 0.92
        x: Math.max(tool.x, Math.min(tool.x + tool.width - width, tool.x + tool.hoverTipCenterX - width / 2))
        y: tool.y + tool.height + 6

        Text {
            id: tipText
            anchors.centerIn: parent
            text: tool.hoverTipText
            color: "#FFFFFF"
            font.pixelSize: 11
        }
    }
}
