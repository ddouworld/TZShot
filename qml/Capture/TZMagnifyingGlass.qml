import QtQuick
import QtQuick.Window

// ── 放大镜 ───────────────────────────────────────────────
Rectangle {
    id: magnifier
    width:  130
    height: 170   // 额外 40px 供颜色信息显示
    z: 10
    color: "transparent"
    visible: root.visibility === Window.FullScreen

    // 跟随鼠标移动，始终显示（不随截图状态切换）
    property int   magnifierMouseX: 0
    property int   magnifierMouseY: 0
    property int   magnifierAbsX: 0
    property int   magnifierAbsY: 0
    property color magnifierPixelColor: "#000000"

    // 跟随鼠标，偏移 +15px，防止被遮挡；超出屏幕时翻转
    x: {
        var px = magnifierMouseX + 15
        if (px + width > root.width) px = magnifierMouseX - width - 15
        return px
    }
    y: {
        var py = magnifierMouseY + 15
        if (py + height > root.height) py = magnifierMouseY - height - 15
        return py
    }

    // 外框
    Rectangle {
        id: magnifierFrame
        anchors.top: parent.top
        width: 130
        height: 130
        radius: 4
        color: "#1a1a1a"
        border.color: "#ffffff"
        border.width: 1
        clip: true

        // 放大图像（来自 ImageProvider）
        Image {
            id: magnifierImage
            anchors.centerIn: parent
            width:  110
            height: 110
            // cache: false 确保每次坐标变化时重新请求图像
            cache: false
            // source 用时间戳 +坐标，强制 QML 重新加载
            source: magnifier.visible
                    ? ("image://magnifier/" + magnifierAbsX + "," + magnifierAbsY + "?t=" + Date.now())
                    : ""
            fillMode: Image.Stretch
        }

        // 中心像素颜色色块（8×8，叠在图像中心上方，不遮挡准线）
        Rectangle {
            anchors.centerIn: parent
            width: 8
            height: 8
            color: magnifierPixelColor
            border.color: "#ffffff"
            border.width: 1
            visible: false   // 色块已在 RGB 文本行体现，避免遮挡准线
        }
    }

    // 颜色信息栏
    Rectangle {
        id: colorInfoBar
        anchors.top: magnifierFrame.bottom
        anchors.topMargin: 2
        width: 130
        height: 38
        radius: 4
        color: "#cc1a1a1a"
        border.color: "#555555"
        border.width: 1

        Row {
            anchors.centerIn: parent
            spacing: 6

            // 色块
            Rectangle {
                width: 16
                height: 16
                radius: 2
                color: magnifierPixelColor
                border.color: "#ffffff"
                border.width: 1
                anchors.verticalCenter: parent.verticalCenter
            }

            // RGB 文本
            Text {
                id: rgbText
                anchors.verticalCenter: parent.verticalCenter
                color: "#ffffff"
                font.pixelSize: 11
                font.family: "monospace"
                text: {
                    var c = magnifierPixelColor
                    var r = Math.round(c.r * 255)
                    var g = Math.round(c.g * 255)
                    var b = Math.round(c.b * 255)
                    return "R:" + r + " G:" + g + " B:" + b
                }
            }
        }
    }
}
// ── 放大镜 end ───────────────────────────────────────────
