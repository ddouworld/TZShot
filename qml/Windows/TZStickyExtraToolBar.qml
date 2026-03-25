import QtQuick

Rectangle {
    id: bar
    radius: 10
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1

    signal zoomOut()
    signal zoomIn()
    signal rotate90()
    signal mirror()
    signal resetSize()
    signal doneAndCopy()
    signal hideToolbar()
    signal hoverEntered(string text, Item targetItem)
    signal hoverExited(string text)

    property real zoomFactor: 1.0

    width: extraRow.implicitWidth + 16
    height: 40

    Row {
        id: extraRow
        anchors.centerIn: parent
        spacing: 6

        Rectangle { width: 30; height: 30; radius: 7; color: zoomOutArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "-"; color: "#334155"; font.pixelSize: 16 } MouseArea { id: zoomOutArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Zoom out"), parent); onExited: bar.hoverExited(qsTr("Zoom out")); onPressed: bar.zoomOut() } }
        Rectangle { width: 40; height: 30; radius: 7; color: "transparent"; Text { anchors.centerIn: parent; text: Math.round(bar.zoomFactor * 100) + "%"; color: "#64748B"; font.pixelSize: 11 } }
        Rectangle { width: 30; height: 30; radius: 7; color: zoomInArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "+"; color: "#334155"; font.pixelSize: 16 } MouseArea { id: zoomInArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Zoom in"), parent); onExited: bar.hoverExited(qsTr("Zoom in")); onPressed: bar.zoomIn() } }
        Rectangle { width: 30; height: 30; radius: 7; color: rotateArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "↻"; color: "#334155"; font.pixelSize: 14 } MouseArea { id: rotateArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Rotate 90°"), parent); onExited: bar.hoverExited(qsTr("Rotate 90°")); onPressed: bar.rotate90() } }
        Rectangle { width: 30; height: 30; radius: 7; color: mirrorArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "⇋"; color: "#334155"; font.pixelSize: 14 } MouseArea { id: mirrorArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Mirror"), parent); onExited: bar.hoverExited(qsTr("Mirror")); onPressed: bar.mirror() } }
        Rectangle { width: 30; height: 30; radius: 7; color: resetSizeArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "1:1"; color: "#334155"; font.pixelSize: 10 } MouseArea { id: resetSizeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Reset size"), parent); onExited: bar.hoverExited(qsTr("Reset size")); onPressed: bar.resetSize() } }
        Rectangle { width: 1; height: 18; color: "#E2E8F0"; anchors.verticalCenter: parent.verticalCenter }
        Rectangle { width: 30; height: 30; radius: 7; color: overwriteArea.containsMouse ? "#E0F2FE" : "transparent"; Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_check.svg"; width: 18; height: 18 } MouseArea { id: overwriteArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Done and copy"), parent); onExited: bar.hoverExited(qsTr("Done and copy")); onPressed: bar.doneAndCopy() } }
        Rectangle { width: 30; height: 30; radius: 7; color: clearToolArea.containsMouse ? "#FEE2E2" : "transparent"; Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_x.svg"; width: 18; height: 18 } MouseArea { id: clearToolArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: bar.hoverEntered(qsTr("Hide toolbar"), parent); onExited: bar.hoverExited(qsTr("Hide toolbar")); onPressed: bar.hideToolbar() } }
    }
}
