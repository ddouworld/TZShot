import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

Rectangle {
    id: panel

    property color currentColor: "#F43F5E"
    property int currentSize: 6
    property bool showColorRow: true
    property string toolName: ""

    property string annotationText: qsTr("文本")
    property bool textBackgroundEnabled: true
    property int mosaicBlurLevel: 2
    property bool numberAutoIncrement: true
    property int numberValue: 1

    width: 260
    height: toolName === "text" ? 128
           : (toolName === "number" ? 132
           : ((toolName === "mosaic") ? 92 : (showColorRow ? 84 : 50)))
    radius: 10
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 8

        RowLayout {
            spacing: 6
            Layout.fillWidth: true
            visible: panel.showColorRow
            height: panel.showColorRow ? implicitHeight : 0

            Repeater {
                model: ["#F43F5E","#EF4444","#F97316","#EAB308",
                        "#22C55E","#3B82F6","#8B5CF6","#000000","#FFFFFF"]
                delegate: Rectangle {
                    width: 18; height: 18; radius: 9
                    color: modelData
                    border.color: panel.currentColor === modelData ? "#3B82F6" : "#CBD5E1"
                    border.width: panel.currentColor === modelData ? 2 : 1
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onPressed: panel.currentColor = modelData
                    }
                }
            }
        }

        RowLayout {
            spacing: 6
            Layout.fillWidth: true

            Repeater {
                model: [
                    { size: 2, label: qsTr("细") },
                    { size: 4, label: qsTr("中") },
                    { size: 6, label: qsTr("粗") },
                    { size: 10, label: qsTr("特粗") }
                ]
                delegate: Rectangle {
                    width: 48; height: 28
                    radius: 6
                    color: panel.currentSize === modelData.size ? "#EFF6FF" : (sizeArea.containsMouse ? "#F1F5F9" : "transparent")
                    border.color: panel.currentSize === modelData.size ? "#3B82F6" : "#E2E8F0"
                    border.width: 1

                    Column {
                        anchors.centerIn: parent
                        spacing: 3
                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: 24
                            height: Math.min(modelData.size, 6)
                            radius: height / 2
                            color: panel.currentSize === modelData.size ? "#3B82F6" : "#94A3B8"
                        }
                        Text {
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: modelData.label
                            font.pixelSize: 10
                            color: panel.currentSize === modelData.size ? "#3B82F6" : "#64748B"
                        }
                    }

                    MouseArea {
                        id: sizeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: panel.currentSize = modelData.size
                    }
                }
            }
        }

        RowLayout {
            visible: panel.toolName === "text"
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                radius: 6
                color: "#F8FAFC"
                border.color: textInput.activeFocus ? "#3B82F6" : "#E2E8F0"
                border.width: 1

                TextInput {
                    id: textInput
                    anchors.fill: parent
                    anchors.margins: 8
                    verticalAlignment: TextInput.AlignVCenter
                    color: "#0F172A"
                    font.pixelSize: 13
                    selectByMouse: true
                    text: panel.annotationText
                    onTextChanged: panel.annotationText = text
                }

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 8
                    visible: textInput.text.length === 0 && !textInput.activeFocus
                    text: qsTr("输入文字")
                    color: "#94A3B8"
                    font.pixelSize: 13
                }
            }

            CheckBox {
                text: qsTr("背景")
                checked: panel.textBackgroundEnabled
                onToggled: panel.textBackgroundEnabled = checked
            }
        }

        RowLayout {
            visible: panel.toolName === "mosaic"
            Layout.fillWidth: true
            spacing: 8

            Text {
                text: qsTr("模糊")
                color: "#334155"
            }

            Slider {
                Layout.fillWidth: true
                from: 1
                to: 4
                stepSize: 1
                value: panel.mosaicBlurLevel
                onMoved: panel.mosaicBlurLevel = Math.round(value)
            }

            Text {
                text: panel.mosaicBlurLevel
                color: "#64748B"
            }
        }

        ColumnLayout {
            visible: panel.toolName === "number"
            Layout.fillWidth: true
            spacing: 8

            CheckBox {
                text: qsTr("自动递增")
                checked: panel.numberAutoIncrement
                onToggled: panel.numberAutoIncrement = checked
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: qsTr("数字")
                    color: "#334155"
                }

                Rectangle {
                    width: 28
                    height: 28
                    radius: 6
                    color: minusArea.containsMouse ? "#F1F5F9" : "transparent"
                    border.color: "#CBD5E1"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "-"
                        color: "#334155"
                        font.pixelSize: 16
                    }

                    MouseArea {
                        id: minusArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: panel.numberValue = Math.max(1, panel.numberValue - 1)
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 28
                    radius: 6
                    color: "#F8FAFC"
                    border.color: "#E2E8F0"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: panel.numberValue
                        color: "#0F172A"
                        font.pixelSize: 13
                    }
                }

                Rectangle {
                    width: 28
                    height: 28
                    radius: 6
                    color: plusArea.containsMouse ? "#F1F5F9" : "transparent"
                    border.color: "#CBD5E1"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "+"
                        color: "#334155"
                        font.pixelSize: 16
                    }

                    MouseArea {
                        id: plusArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onPressed: panel.numberValue = Math.min(9999, panel.numberValue + 1)
                    }
                }
            }
        }
    }
}
