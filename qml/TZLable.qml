import QtQuick

// 显示当前截图大小
Rectangle {
    property string lableText: ""

    radius: 6
    color: "#1E293B"
    opacity: 0.85
    border.color: "#334155"
    border.width: 1

    implicitWidth: labelContent.implicitWidth + 16
    implicitHeight: labelContent.implicitHeight + 8

    Text {
        id: labelContent
        anchors.centerIn: parent
        font.pixelSize: 12
        font.family: "Microsoft YaHei"
        color: "#F1F5F9"
        text: lableText
        renderType: Text.NativeRendering
    }
}

