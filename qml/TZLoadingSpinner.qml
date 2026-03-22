import QtQuick

// ── AI 加载动画层 ─────────────────────────────────────
Rectangle {

    width: 100
    height: 100
    radius: 2
    color: "#99000000"   // 半透明黑色遮罩


    // 旋转圆弧
    Item {
        id: spinnerRoot
        anchors.centerIn: parent
        width:  48
        height: 48

        RotationAnimator {
            target: spinnerRoot
            from: 0; to: 360
            duration: 900
            loops: Animation.Infinite
            running: O_AIViewModel.isLoading
        }

        // 四个小圆点模拟旋转圆弧
        Repeater {
            model: 8
            Rectangle {
                width:  6
                height: 6
                radius: 3
                color: "white"
                opacity: (index + 1) / 8
                x: spinnerRoot.width  / 2 + 18 * Math.cos(index * Math.PI / 4) - width  / 2
                y: spinnerRoot.height / 2 + 18 * Math.sin(index * Math.PI / 4) - height / 2
            }
        }
    }

    // 提示文字
    Text {
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: spinnerRoot.bottom
        anchors.topMargin: 10
        text: "AI 生成中..."
        color: "white"
        font.pixelSize: 13
    }
}

