import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: longBar
    width: 500
    height: 56
    visible: false
    color: "transparent"
    flags: Qt.Tool | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.WindowDoesNotAcceptFocus

    signal startRequested()
    signal stopRequested()

    property bool running: false
    property int frameCount: 0
    property string message: qsTr("点击开始后，滑动内容自动拼接")

    Rectangle {
        anchors.fill: parent
        radius: 12
        color: "#111827"
        border.color: "#1F2937"
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                width: 26
                height: 26
                radius: 13
                color: "#F97316"
                Layout.alignment: Qt.AlignVCenter
                Text {
                    anchors.centerIn: parent
                    text: "长"
                    color: "#FFFFFF"
                    font.pixelSize: 14
                    font.bold: true
                }
            }

            Text {
                Layout.fillWidth: true
                text: longBar.message
                color: "#E5E7EB"
                font.pixelSize: 13
                elide: Text.ElideRight
                verticalAlignment: Text.AlignVCenter
            }

            Text {
                text: qsTr("%1 帧").arg(longBar.frameCount)
                color: "#D1D5DB"
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
            }

            Rectangle {
                width: 72
                height: 30
                radius: 6
                visible: !longBar.running
                color: startArea.containsMouse ? "#1D4ED8" : "#2563EB"
                Layout.alignment: Qt.AlignVCenter
                Behavior on color { ColorAnimation { duration: 100 } }
                Text {
                    anchors.centerIn: parent
                    text: qsTr("开始")
                    color: "#FFFFFF"
                    font.pixelSize: 13
                }
                MouseArea {
                    id: startArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: longBar.startRequested()
                }
            }

            Rectangle {
                width: 72
                height: 30
                radius: 6
                color: stopArea.containsMouse ? "#DC2626" : "#EF4444"
                Layout.alignment: Qt.AlignVCenter
                Behavior on color { ColorAnimation { duration: 100 } }
                Text {
                    anchors.centerIn: parent
                    text: longBar.running ? qsTr("完成") : qsTr("关闭")
                    color: "#FFFFFF"
                    font.pixelSize: 13
                }
                MouseArea {
                    id: stopArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: longBar.stopRequested()
                }
            }
        }
    }
}
