import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic

Rectangle {
    color: "#F8FAFC"

    // ── 按键录制弹窗 ──────────────────────────────────────
    Rectangle {
        id: recordOverlay
        anchors.fill: parent
        color: "#80000000"
        visible: false
        z: 100

        property int  recordingActionId: 0
        property string recordingLabel: ""
        property string pendingKeySeq: ""

        // 捕获键盘焦点以接收按键事件
        focus: visible

        Keys.onPressed: function(event) {
            event.accepted = true
            var seq = O_GlobalShortcut.buildKeySequence(event.key, event.modifiers)
            if (seq !== "") {
                recordOverlay.pendingKeySeq = seq
            }
        }

        // 弹窗卡片
        Rectangle {
            anchors.centerIn: parent
            width: 360
            height: 220
            radius: 12
            color: "#FFFFFF"
            border.color: "#E2E8F0"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 16

                // 标题
                Text {
                    Layout.fillWidth: true
                    text: qsTr("编辑快捷键 · %1").arg(recordOverlay.recordingLabel)
                    font.pixelSize: 14
                    font.bold: true
                    color: "#1E293B"
                }

                // 录制区域
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 56
                    radius: 8
                    color: "#F1F5F9"
                    border.color: recordOverlay.pendingKeySeq !== "" ? "#3B82F6" : "#CBD5E1"
                    border.width: recordOverlay.pendingKeySeq !== "" ? 2 : 1

                    Behavior on border.color { ColorAnimation { duration: 120 } }

                    Text {
                        anchors.centerIn: parent
                    text: recordOverlay.pendingKeySeq !== ""
                          ? recordOverlay.pendingKeySeq
                          : qsTr("请按下新的快捷键组合...")
                        font.pixelSize: 15
                        font.family: "Monospace"
                        color: recordOverlay.pendingKeySeq !== "" ? "#1E293B" : "#94A3B8"
                    }
                }

                // 冲突提示
                Text {
                    id: conflictHint
                    Layout.fillWidth: true
                    font.pixelSize: 12
                    color: "#EF4444"
                    visible: false
                    wrapMode: Text.WordWrap
                }

                // 按钮行
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    // 取消
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 6
                        color: cancelArea.containsMouse ? "#F1F5F9" : "#F8FAFC"
                        border.color: "#E2E8F0"
                        Behavior on color { ColorAnimation { duration: 100 } }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("取消")
                            font.pixelSize: 13
                            color: "#64748B"
                        }
                        MouseArea {
                            id: cancelArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                recordOverlay.visible = false
                                recordOverlay.pendingKeySeq = ""
                                conflictHint.visible = false
                            }
                        }
                    }

                    // 确认
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 6
                        color: {
                            if (recordOverlay.pendingKeySeq === "") return "#93C5FD"
                            return confirmArea.containsMouse ? "#2563EB" : "#3B82F6"
                        }
                        Behavior on color { ColorAnimation { duration: 100 } }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("确认")
                            font.pixelSize: 13
                            color: "#FFFFFF"
                        }
                        MouseArea {
                            id: confirmArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            enabled: recordOverlay.pendingKeySeq !== ""
                            onClicked: {
                                var seq = recordOverlay.pendingKeySeq
                                var aid = recordOverlay.recordingActionId

                                // 检查冲突
                                var conflictId = O_GlobalShortcut.checkConflict(seq)
                                if (conflictId !== 0 && conflictId !== aid) {
                                var labels = ["", qsTr("截图"), qsTr("截图并保存"), qsTr("贴图到桌面"), qsTr("显示/隐藏窗口")]
                                conflictHint.text = "「" + seq + "」已被「" + labels[conflictId] + "」使用，请换一个"
                                    conflictHint.visible = true
                                    return
                                }

                                // 应用
                                if (O_GlobalShortcut.updateShortcut(aid, seq)) {
                                    recordOverlay.visible = false
                                    recordOverlay.pendingKeySeq = ""
                                    conflictHint.visible = false
                                } else {
                                    conflictHint.text = "快捷键「" + seq + "」注册失败，请换一个组合"
                                    conflictHint.visible = true
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ── 主内容区 ──────────────────────────────────────────
    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: 8

            anchors.left:      parent.left
            anchors.right:     parent.right
            anchors.margins:   24
            anchors.topMargin: 24

            Item { Layout.preferredHeight: 4 }

            // ── 标题 ────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                spacing: 8
                Rectangle {
                    width: 3; height: 15; radius: 2
                    color: "#3B82F6"
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("全局快捷键")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E293B"
                }
            }

            // ── 快捷键列表卡片 ───────────────────────────
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: hotkeyCol.implicitHeight + 32
                color: "#FFFFFF"
                radius: 10
                border.color: "#E2E8F0"
                border.width: 1

                ColumnLayout {
                    id: hotkeyCol
                    anchors.left:    parent.left
                    anchors.right:   parent.right
                    anchors.top:     parent.top
                    anchors.margins: 16
                    spacing: 0

                    Repeater {
                        // 动态从后端读取当前快捷键
                        model: [
                            { label: qsTr("截图"),          actionId: 1, keySeq: O_GlobalShortcut.shortcutScreenshot     },
                            { label: qsTr("截图并保存"),    actionId: 2, keySeq: O_GlobalShortcut.shortcutScreenshotSave },
                            { label: qsTr("贴图到桌面"),    actionId: 3, keySeq: O_GlobalShortcut.shortcutSticky         },
                            { label: qsTr("显示/隐藏窗口"), actionId: 4, keySeq: O_GlobalShortcut.shortcutToggle         }
                        ]

                        delegate: ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            RowLayout {
                                Layout.fillWidth: true
                                Layout.topMargin:    12
                                Layout.bottomMargin: 12

                                // 功能名称
                                Text {
                                    Layout.preferredWidth: 120
                                    text: modelData.label
                                    font.pixelSize: 13
                                    color: "#374151"
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                // 快捷键徽章
                                RowLayout {
                                    spacing: 4
                                    Layout.alignment: Qt.AlignVCenter

                                    Repeater {
                                        model: modelData.keySeq.split("+")
                                        delegate: Rectangle {
                                            implicitWidth:  kbLabel.implicitWidth + 16
                                            implicitHeight: 28
                                            radius: 5
                                            color: "#F1F5F9"
                                            border.color: "#CBD5E1"
                                            border.width: 1
                                            Text {
                                                id: kbLabel
                                                anchors.centerIn: parent
                                                text: modelData
                                                font.pixelSize: 12
                                                font.family: "Monospace"
                                                color: "#374151"
                                            }
                                        }
                                    }
                                }

                                Item { Layout.fillWidth: true }

                                // 编辑按钮
                                Rectangle {
                                    implicitWidth:  60
                                    implicitHeight: 28
                                    radius: 5
                                    color: editArea.containsMouse ? "#EFF6FF" : "#F8FAFC"
                                    border.color: editArea.containsMouse ? "#3B82F6" : "#E2E8F0"
                                    border.width: 1
                                    Behavior on color       { ColorAnimation { duration: 100 } }
                                    Behavior on border.color{ ColorAnimation { duration: 100 } }

                                    Text {
                                        anchors.centerIn: parent
                                        text: qsTr("编辑")
                                        font.pixelSize: 12
                                        color: editArea.containsMouse ? "#3B82F6" : "#64748B"
                                        Behavior on color { ColorAnimation { duration: 100 } }
                                    }

                                    MouseArea {
                                        id: editArea
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            recordOverlay.recordingActionId = modelData.actionId
                                            recordOverlay.recordingLabel    = modelData.label
                                            recordOverlay.pendingKeySeq     = ""
                                            conflictHint.visible            = false
                                            recordOverlay.visible           = true
                                            recordOverlay.forceActiveFocus()
                                        }
                                    }
                                }
                            }

                            // 分割线（最后一行不显示）
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 1
                                color: "#F1F5F9"
                                visible: index < 3
                            }
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }

            // ── 说明 ────────────────────────────────────
            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 8
                spacing: 8
                Rectangle {
                    width: 3; height: 15; radius: 2
                    color: "#3B82F6"
                    Layout.alignment: Qt.AlignVCenter
                }
                Text {
                    text: qsTr("说明")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E293B"
                }
            }

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: noteText.implicitHeight + 24
                color: "#FFFFFF"
                radius: 10
                border.color: "#E2E8F0"
                border.width: 1

                Text {
                    id: noteText
                    anchors.left:    parent.left
                    anchors.right:   parent.right
                    anchors.top:     parent.top
                    anchors.margins: 16
                    text: qsTr("快捷键在全局范围内生效。点击「编辑」后直接按下新的组合键，再点「确认」即可保存。\n若与系统或其他应用快捷键冲突，请修改为其他组合。")
                    font.pixelSize: 13
                    color: "#64748B"
                    wrapMode: Text.WordWrap
                    lineHeight: 1.6
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}
