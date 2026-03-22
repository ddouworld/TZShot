import QtQuick
import QtQuick.Window
import QtQuick.Layouts

Window {
    id: aboutWin
    width: 400
    height: 350
    title: qsTr("关于 TZshot")
    color: "#F8FAFC"
    flags: Qt.Dialog | Qt.WindowCloseButtonHint
    minimumWidth: width
    maximumWidth: width
    minimumHeight: height
    maximumHeight: height

    Component.onCompleted: {
        x = (Screen.width  - width)  / 2
        y = (Screen.height - height) / 2
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 32
        spacing: 0

        // ── Logo 区 ──────────────────────────────────
        Item {
            Layout.alignment: Qt.AlignHCenter
            width: 72; height: 72

            Rectangle {
                anchors.fill: parent
                radius: 18
                color: "#3B82F6"

                Text {
                    anchors.centerIn: parent
                    text: "T"
                    font.pixelSize: 40
                    font.bold: true
                    color: "#FFFFFF"
                }
            }
        }

        Item { Layout.preferredHeight: 16 }

        // ── 应用名称 ──────────────────────────────────
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "TZshot"
            font.pixelSize: 22
            font.bold: true
            color: "#1E293B"
        }

        Item { Layout.preferredHeight: 6 }

        // ── 版本号 ────────────────────────────────────
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("版本 v1.0.0")
            font.pixelSize: 13
            color: "#64748B"
        }

        Item { Layout.preferredHeight: 16 }

        // ── 分割线 ────────────────────────────────────
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#E2E8F0"
        }

        Item { Layout.preferredHeight: 16 }

        // ── 描述 ──────────────────────────────────────
        Text {
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignHCenter
            text: qsTr("简洁高效的截图贴图工具，支持标注、AI 编辑、马赛克等功能。")
            font.pixelSize: 13
            color: "#475569"
            wrapMode: Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
        }

        Item { Layout.fillHeight: true }

        // ── 版权 ──────────────────────────────────────
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 4
            text: qsTr("© 2025 TZshot. All rights reserved.")
            font.pixelSize: 11
            color: "#94A3B8"
        }

        // ── 关闭按钮 ──────────────────────────────────
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            width: 88; height: 34
            radius: 8
            color: closeMouse.containsMouse ? "#2563EB" : "#3B82F6"

            Behavior on color { ColorAnimation { duration: 100 } }

            Text {
                anchors.centerIn: parent
                text: qsTr("关闭")
                font.pixelSize: 13
                color: "#FFFFFF"
            }

            MouseArea {
                id: closeMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: aboutWin.close()
            }
        }

        Item { Layout.preferredHeight: 4 }
    }
}
