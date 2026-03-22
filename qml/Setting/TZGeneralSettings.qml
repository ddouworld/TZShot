import QtQuick
import QtQuick.Layouts
import QtQuick.Controls.Basic
import TZControls 1.0

Rectangle {
    color: "#F8FAFC"

    ScrollView {
        id: scrollView
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            // 必须绑定到 ScrollView.availableWidth，不能用 parent.width
            // （ScrollView 内部 contentItem 宽度不受限）
            x: 24
            width: scrollView.availableWidth - 48
            spacing: 8

            Item { Layout.preferredHeight: 4 }

            // ── 大模型设置 标题 ──────────────────────────
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
                    text: qsTr("大模型设置")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E293B"
                }
            }

            // ── 大模型设置 卡片 ──────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 120
                color: "#FFFFFF"
                radius: 10
                border.color: "#E2E8F0"
                border.width: 1

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    // 大模型选择
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 90
                            text: qsTr("大模型选择")
                            font.pixelSize: 13
                            color: "#374151"
                            Layout.alignment: Qt.AlignVCenter
                        }

                        TZComboBox {
                            id: combobox
                            model: [qsTr("阿里云百炼"), qsTr("火山引擎")]
                            currentIndex: O_AIViewModel.selectedModel
                            Layout.preferredWidth: 220
                            Layout.preferredHeight: 36

                            onCurrentIndexChanged: {
                               O_AIViewModel.setSelectedModel(currentIndex)
                            }
                        }
                    }

                    // API Key
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Text {
                            Layout.preferredWidth: 90
                            text: "API Key"
                            font.pixelSize: 13
                            color: "#374151"
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Rectangle {
                            Layout.preferredWidth: 320
                            Layout.preferredHeight: 36
                            radius: 6
                            color: apiKeyInput.activeFocus ? "#EFF6FF" : "#F8FAFC"
                            border.color: apiKeyInput.activeFocus ? "#3B82F6" : "#E2E8F0"
                            border.width: apiKeyInput.activeFocus ? 2 : 1

                            Behavior on border.color { ColorAnimation { duration: 150 } }
                            Behavior on color        { ColorAnimation { duration: 150 } }

                            TextInput {
                                id: apiKeyInput
                                anchors.fill: parent
                                anchors.margins: 10
                                font.pixelSize: 13
                                color: "#1E293B"
                                selectByMouse: true
                                verticalAlignment: Text.AlignVCenter
                                clip: true

                                // 初始化时从后端读取已持久化的 apiKey
                                Component.onCompleted: {
                                    text = O_AIViewModel.apiKey
                                }

                                Text {
                                    anchors.fill: parent
                                    verticalAlignment: Text.AlignVCenter
                                    text: qsTr("请输入 API Key")
                                    font.pixelSize: 13
                                    color: "#CBD5E1"
                                    visible: !apiKeyInput.text && !apiKeyInput.activeFocus
                                }
                            }
                        }

                        // 保存按钮
                        Rectangle {
                            id: saveApiKeyBtn
                            Layout.preferredWidth: 60
                            Layout.preferredHeight: 36
                            radius: 6
                            property bool modified: apiKeyInput.text !== O_AIViewModel.apiKey
                            color: {
                                if (!modified)    return "#E2E8F0"
                                if (saveArea.containsMouse) return "#2563EB"
                                return "#3B82F6"
                            }
                            Behavior on color { ColorAnimation { duration: 120 } }

                            Text {
                                anchors.centerIn: parent
                                text: qsTr("保存")
                                font.pixelSize: 13
                                color: saveApiKeyBtn.modified ? "#FFFFFF" : "#94A3B8"
                            }

                            MouseArea {
                                id: saveArea
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: saveApiKeyBtn.modified ? Qt.PointingHandCursor : Qt.ArrowCursor
                                enabled: saveApiKeyBtn.modified
                                onClicked: {
                                    O_AIViewModel.setApiKey(apiKeyInput.text)
                                }
                            }
                        }
                    }

                    Item { Layout.fillHeight: true }
                }
            }

            Item { Layout.preferredHeight: 8 }

            // ── 语言设置 标题 ────────────────────────────
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
                    text: qsTr("语言设置")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E293B"
                }
            }

            // ── 语言设置 卡片 ────────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                color: "#FFFFFF"
                radius: 10
                border.color: "#E2E8F0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("界面语言")
                        font.pixelSize: 13
                        color: "#374151"
                        Layout.alignment: Qt.AlignVCenter
                    }

                    TZComboBox {
                        id: langComboBox
                        // Index 0 = zh_CN, Index 1 = en
                        model: ["中文", "English"]
                        currentIndex: O_LanguageManager.language === "en" ? 1 : 0
                        Layout.preferredWidth: 160
                        Layout.preferredHeight: 36

                        onCurrentIndexChanged: {
                            var lang = currentIndex === 1 ? "en" : "zh_CN"
                            O_LanguageManager.setLanguage(lang)
                        }
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }

            // ── 截图保存路径 标题 ────────────────────────
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
                    text: qsTr("截图保存路径")
                    font.pixelSize: 13
                    font.bold: true
                    color: "#1E293B"
                }
            }

            // ── 截图保存路径 卡片 ────────────────────────
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 72
                color: "#FFFFFF"
                radius: 10
                border.color: "#E2E8F0"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 12

                    Text {
                        Layout.preferredWidth: 90
                        text: qsTr("保存路径")
                        font.pixelSize: 13
                        color: "#374151"
                        Layout.alignment: Qt.AlignVCenter
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 36
                        radius: 6
                        color: "#F8FAFC"
                        border.color: "#E2E8F0"
                        border.width: 1

                        Text {
                            id: saveFolder
                            anchors.fill: parent
                            anchors.margins: 10
                            font.pixelSize: 13
                            color: "#374151"
                            text: O_ImageSaver.savePath
                            elide: Text.ElideMiddle
                            verticalAlignment: Text.AlignVCenter
                            clip: true
                            wrapMode: Text.NoWrap

                            ToolTip {
                                id: folderTip
                                visible: false
                                text: saveFolder.text
                                delay: 500
                            }

                            MouseArea {
                                anchors.fill: parent
                                hoverEnabled: true
                                onEntered: folderTip.visible = true
                                onExited:  folderTip.visible = false
                            }
                        }
                    }

                    // 浏览按钮
                    Rectangle {
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 36
                        radius: 6
                        color: browseArea.containsMouse ? "#2563EB" : "#3B82F6"
                        Behavior on color { ColorAnimation { duration: 120 } }

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("浏览")
                            font.pixelSize: 13
                            color: "#FFFFFF"
                        }

                        MouseArea {
                            id: browseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: saveDialog.open()
                        }
                    }
                }
            }

            Item { Layout.fillHeight: true }
        }
    }
}

