import QtQuick

Rectangle {
    id: editor

    property alias editorText: textInput.text

    signal accepted(string text)
    signal canceled()

    width: 260
    height: 88
    visible: false
    radius: 8
    color: "#FFFFFF"
    border.color: "#CBD5E1"
    border.width: 1

    function beginEdit(textValue) {
        editorText = textValue
        visible = true
        textInput.forceActiveFocus()
        textInput.selectAll()
    }

    Column {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 8

        Rectangle {
            width: parent.width
            height: 34
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
                onAccepted: editor.accepted(text)
            }
        }

        Row {
            spacing: 8

            Rectangle {
                width: 72
                height: 28
                radius: 6
                color: cancelInputArea.containsMouse ? "#F1F5F9" : "transparent"
                border.color: "#CBD5E1"
                border.width: 1

                Text {
                    anchors.centerIn: parent
                    text: qsTr("Cancel")
                    color: "#334155"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: cancelInputArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: editor.canceled()
                }
            }

            Rectangle {
                width: 72
                height: 28
                radius: 6
                color: confirmInputArea.containsMouse ? "#2563EB" : "#3B82F6"

                Text {
                    anchors.centerIn: parent
                    text: qsTr("OK")
                    color: "#FFFFFF"
                    font.pixelSize: 12
                }

                MouseArea {
                    id: confirmInputArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onPressed: editor.accepted(textInput.text)
                }
            }
        }
    }
}
