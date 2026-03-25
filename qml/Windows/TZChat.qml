import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Basic

QtObject {
    id: chatPanel

    // 对应贴图的图片 URL，由外部绑定
    property string imageUrl: ""

    signal signalSendPrompt(var text)

    // 公开方法
    function open(px, py) {
        win.x = px
        win.y = py
        win.visible = true
        win.raise()
        win.requestActivate()
    }

    function close() {
        win.visible = false
    }

    // ── 内部 Window ─────────────────────────────────────
    property var win: Window {
        id: win
        flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
        color: "transparent"
        width: 420
        height: 380
        visible: false

        Rectangle {
            anchors.fill: parent
            color: "#ffffff"
            radius: 10
            border.color: "#E2E8F0"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 12
                spacing: 8

                // ── 顶部标题栏 ────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 36
                    color: "transparent"

                    // 拖动区域（z:-1 让关闭按钮的点击不被拦截）
                    MouseArea {
                        id: dragHandle
                        anchors.fill: parent
                        z: -1
                        property point startPos

                        onPressed: startPos = Qt.point(mouseX, mouseY)
                        onPositionChanged: {
                            if (pressed) {
                                win.x += mouseX - startPos.x
                                win.y += mouseY - startPos.y
                            }
                        }
                        cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Image {
                            source: "qrc:/resource/img/lc_chat.svg"
                            width: 20
                            height: 20
                            fillMode: Image.PreserveAspectFit
                        }

                        Text {
                            text: qsTr("AI 图像编辑")
                            font.pixelSize: 14
                            font.weight: Font.Medium
                            color: "#1E293B"
                        }

                        Item { Layout.fillWidth: true }

                        // 关闭按钮
                        Rectangle {
                            width: 28
                            height: 28
                            radius: 6
                            color: clearMouse.containsMouse ? "#FEE2E2" : "transparent"

                            Image {
                                anchors.centerIn: parent
                                source: "qrc:/resource/img/lc_close.svg"
                                width: 14
                                height: 14
                                fillMode: Image.PreserveAspectFit
                            }

                            MouseArea {
                                id: clearMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: chatPanel.close()
                            }
                        }
                    }

                    // 标题栏底部分割线
                    Rectangle {
                        anchors.bottom: parent.bottom
                        anchors.left: parent.left
                        anchors.right: parent.right
                        height: 1
                        color: "#E2E8F0"
                    }
                }

                // ── 贴图缩略图预览 ────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 64
                    radius: 6
                    color: "#F1F5F9"
                    border.color: "#E2E8F0"
                    border.width: 1
                    clip: true

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        // 缩略图
                        Rectangle {
                            width: 48
                            height: 48
                            radius: 4
                            color: "#E2E8F0"
                            clip: true

                            Image {
                                anchors.fill: parent
                                source: chatPanel.imageUrl
                                fillMode: Image.PreserveAspectCrop
                                cache: false
                            }
                        }

                        // 文字说明
                        Column {
                            Layout.fillWidth: true
                            spacing: 3

                            Text {
                                text: qsTr("当前贴图")
                                font.pixelSize: 11
                                color: "#94A3B8"
                            }

                            Text {
                                width: parent.width
                                text: chatPanel.imageUrl !== ""
                                      ? chatPanel.imageUrl.replace(/^image:\/\/[^/]+\//, "ID: ")
                                      : qsTr("未关联贴图")
                                font.pixelSize: 12
                                color: "#475569"
                                elide: Text.ElideRight
                            }

                            Text {
                                text: qsTr("发送后将对此图执行 AI 编辑")
                                font.pixelSize: 10
                                color: "#94A3B8"
                            }
                        }
                    }
                }

                // ── 输入区域 ──────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    color: "#F8FAFC"
                    radius: 8
                    border.color: "#E2E8F0"
                    border.width: 1

                    TextArea {
                        id: input
                        anchors.fill: parent
                        anchors.margins: 12
                        font.family: "Segoe UI"
                        font.pixelSize: 13
                        color: "#1E293B"
                        wrapMode: Text.Wrap
                        placeholderText: qsTr("描述您想要的修改...")
                        background: Rectangle { color: "transparent" }
                        selectByMouse: true
                        focus: true
                    }
                }

                // ── 底部工具栏 ────────────────────────────────────
                Rectangle {
                    Layout.fillWidth: true
                    height: 40
                    color: "transparent"

                    RowLayout {
                        anchors.fill: parent
                        spacing: 8

                        Item { Layout.fillWidth: true }

                        // 取消按钮
                        Rectangle {
                            width: 70; height: 32; radius: 6
                            color: cancelMouse.containsMouse ? "#F1F5F9" : "#F8FAFC"
                            border.color: "#E2E8F0"; border.width: 1

                            Text { anchors.centerIn: parent; text: qsTr("取消"); font.pixelSize: 13; color: "#64748B" }

                            MouseArea {
                                id: cancelMouse
                                anchors.fill: parent; hoverEnabled: true
                                onClicked: chatPanel.close()
                            }
                        }

                        // 发送按钮
                        Rectangle {
                            width: 70; height: 32; radius: 6
                            color: sendMouse.pressed ? "#0066cc" : (sendMouse.containsMouse ? "#3399ff" : "#0088ff")

                            RowLayout {
                                anchors.centerIn: parent; spacing: 4
                                Image { source: "qrc:/resource/img/lc_send.svg"; width: 14; height: 14 }
                                Text { text: qsTr("发送"); font.pixelSize: 13; color: "white" }
                            }

                            MouseArea {
                                id: sendMouse
                                anchors.fill: parent; hoverEnabled: true
                                onClicked: {
                                    if (input.text.trim() !== "") {
                                        chatPanel.signalSendPrompt(input.text)
                                        chatPanel.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

