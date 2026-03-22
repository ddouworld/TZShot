import QtQuick
import QtQuick.Layouts

Rectangle {
    signal signalSetting()
    signal signalHotkeySettings()

    property string selectName: "setting"
    color: "#F1F5F9"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // 顶部 Logo / 标题区
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 72
            color: "transparent"

            ColumnLayout {
                anchors.centerIn: parent
                spacing: 4

                Rectangle {
                    width: 36
                    height: 36
                    radius: 10
                    color: "#3B82F6"
                    Layout.alignment: Qt.AlignHCenter

                    Text {
                        anchors.centerIn: parent
                        text: "T"
                        font.pixelSize: 20
                        font.bold: true
                        color: "#FFFFFF"
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "TZshot"
                    font.pixelSize: 12
                    color: "#64748B"
                }
            }
        }

        // 分割线
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            color: "#E2E8F0"
        }

        Item { Layout.preferredHeight: 12 }

        // ── 通用设置 菜单项 ──────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: 10
            Layout.rightMargin: 10

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: selectName === "setting" ? "#FFFFFF" : "transparent"
                border.color: selectName === "setting" ? "#E2E8F0" : "transparent"
                border.width: 1
            }

            Rectangle {
                width: 3
                height: 20
                radius: 2
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                color: selectName === "setting" ? "#3B82F6" : "transparent"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 18
                text: qsTr("通用设置")
                font.pixelSize: 13
                font.bold: selectName === "setting"
                color: selectName === "setting" ? "#3B82F6" : "#64748B"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    selectName = "setting"
                    signalSetting()
                }
            }
        }

        Item { Layout.preferredHeight: 4 }

        // ── 快捷键设置 菜单项 ────────────────────
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 44
            Layout.leftMargin: 10
            Layout.rightMargin: 10

            Rectangle {
                anchors.fill: parent
                radius: 8
                color: selectName === "shortcutKey" ? "#FFFFFF" : "transparent"
                border.color: selectName === "shortcutKey" ? "#E2E8F0" : "transparent"
                border.width: 1
            }

            Rectangle {
                width: 3
                height: 20
                radius: 2
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                color: selectName === "shortcutKey" ? "#3B82F6" : "transparent"
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 18
                text: qsTr("快捷键设置")
                font.pixelSize: 13
                font.bold: selectName === "shortcutKey"
                color: selectName === "shortcutKey" ? "#3B82F6" : "#64748B"
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    selectName = "shortcutKey"
                    signalHotkeySettings()
                }
            }
        }

        // 底部弹性空间
        Item { Layout.fillHeight: true }

        // 版本号
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 12
            text: "v1.0.0"
            font.pixelSize: 11
            color: "#CBD5E1"
        }
    }
}

