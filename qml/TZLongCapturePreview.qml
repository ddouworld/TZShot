import QtQuick
import QtQuick.Window

Window {
    id: longPreview
    width: 260
    height: 420
    visible: false
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus

    property string previewUrl: ""
    property string statusText: ""
    property int frameCount: 0

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#111827"
        border.color: "#1F2937"
        border.width: 1

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 8

            Row {
                width: parent.width
                spacing: 8

                Text {
                    text: qsTr("长截图预览")
                    color: "#F9FAFB"
                    font.pixelSize: 13
                    font.bold: true
                }

                Text {
                    text: qsTr("%1 帧").arg(longPreview.frameCount)
                    color: "#9CA3AF"
                    font.pixelSize: 12
                }
            }

            Rectangle {
                width: parent.width
                height: parent.height - 58
                radius: 8
                color: "#0B1220"
                border.color: "#334155"
                border.width: 1
                clip: true

                Image {
                    anchors.fill: parent
                    anchors.margins: 6
                    source: longPreview.previewUrl
                    fillMode: Image.PreserveAspectFit
                    smooth: true
                    cache: false
                }

                Text {
                    anchors.centerIn: parent
                    visible: longPreview.previewUrl.length === 0
                    text: qsTr("暂无预览")
                    color: "#64748B"
                    font.pixelSize: 12
                }
            }

            Text {
                width: parent.width
                text: longPreview.statusText
                color: "#CBD5E1"
                font.pixelSize: 12
                elide: Text.ElideRight
            }
        }
    }
}
