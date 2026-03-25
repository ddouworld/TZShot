import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

// TZOcrResult
// -----------
// 无边框悬浮窗，展示 OCR 识别结果。
// 由 TZWindow.qml 通过 Qt.createComponent 动态创建并显示。

Window {
    id: ocrWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    width:  420
    height: 320

    // 初始居中（创建后由调用方设置位置，此处给默认值）
    x: (Screen.width  - width)  / 2
    y: (Screen.height - height) / 2

    Component.onCompleted: {
        ocrWin.raise()
        ocrWin.requestActivate()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: ocrWin.close()
    }

    // ── 阴影容器 ──────────────────────────────────────────
    Rectangle {
        id: shadow
        anchors.fill: parent
        anchors.margins: 10
        radius: 12
        color: "white"
        layer.enabled: true
        border.color: "#D1D5DB"
        border.width: 1

        // ── 标题栏 ─────────────────────────────────────────
        Rectangle {
            id: titleBar
            anchors.top:    shadow.top
            anchors.left:   shadow.left
            anchors.right:  shadow.right
            height: 40
            radius: 12
            color:  "#F8FAFC"

            // 圆角只在上半部分（下方盖住）
            Rectangle {
                anchors.bottom: titleBar.bottom
                anchors.left:   titleBar.left
                anchors.right:  titleBar.right
                height: titleBar.radius
                color:  titleBar.color
            }

            // 标题文字
            Text {
                anchors.verticalCenter: titleBar.verticalCenter
                anchors.left: titleBar.left
                anchors.leftMargin: 14
                text: qsTr("OCR 识别结果")
                font.pixelSize: 14
                font.weight: Font.Medium
                color: "#1E293B"
            }

            // 关闭按钮
            Rectangle {
                id: closeBtn
                anchors.right:          titleBar.right
                anchors.rightMargin:    8
                anchors.verticalCenter: titleBar.verticalCenter
                width:  26; height: 26; radius: 5
                color: closeArea.containsMouse ? "#FEE2E2" : "transparent"
                Behavior on color { ColorAnimation { duration: 80 } }

                Text {
                    anchors.centerIn: parent
                    text:  "✕"
                    color: closeArea.containsMouse ? "#DC2626" : "#94A3B8"
                    font.pixelSize: 13
                }
                MouseArea {
                    id: closeArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape:  Qt.PointingHandCursor
                    onClicked: ocrWin.close()
                }
            }

            // 拖动标题栏移动窗口
            MouseArea {
                anchors.fill: titleBar
                anchors.rightMargin: 40
                property point pressPos
                onPressed: function(mouse) {
                    pressPos = Qt.point(mouse.x, mouse.y)
                }
                onPositionChanged: function(mouse) {
                    if (pressed) {
                        ocrWin.x += mouse.x - pressPos.x
                        ocrWin.y += mouse.y - pressPos.y
                    }
                }
            }
        }

        // ── 分割线 ─────────────────────────────────────────
        Rectangle {
            id: divider
            anchors.top:   titleBar.bottom
            anchors.left:  shadow.left
            anchors.right: shadow.right
            height: 1
            color:  "#E2E8F0"
        }

        // ── 内容区 ─────────────────────────────────────────
        Item {
            anchors.top:    divider.bottom
            anchors.left:   shadow.left
            anchors.right:  shadow.right
            anchors.bottom: bottomBar.top

            // Loading 遮罩
            Rectangle {
                anchors.fill: parent
                color: "#F8FAFC"
                visible: O_OcrVM.isRecognizing
                radius: 6

                Item {
                    id: spinnerRoot
                    anchors.centerIn: parent
                    width: 48; height: 48

                    RotationAnimator {
                        target: spinnerRoot
                        from: 0; to: 360
                        duration: 900
                        loops: Animation.Infinite
                        running: O_OcrVM.isRecognizing
                    }
                    Repeater {
                        model: 8
                        Rectangle {
                            width: 6; height: 6; radius: 3
                            color: "#3B82F6"
                            opacity: (index + 1) / 8
                            x: spinnerRoot.width  / 2 + 18 * Math.cos(index * Math.PI / 4) - width  / 2
                            y: spinnerRoot.height / 2 + 18 * Math.sin(index * Math.PI / 4) - height / 2
                        }
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: spinnerRoot.bottom
                    anchors.topMargin: 12
                    text: qsTr("正在识别中…")
                    color: "#64748B"
                    font.pixelSize: 13
                }
            }

            // 文本区域（识别完成后显示）
            ScrollView {
                anchors.fill: parent
                anchors.margins: 8
                visible: !O_OcrVM.isRecognizing
                clip: true

                TextArea {
                    id: resultArea
                    text: O_OcrVM.resultText
                    readOnly: false   // 允许用户二次编辑
                    wrapMode: TextEdit.Wrap
                    font.pixelSize: 14
                    color: "#1E293B"
                    background: null
                    padding: 4
                    selectByMouse: true
                    placeholderText: qsTr("识别结果将显示在这里…")
                    placeholderTextColor: "#94A3B8"
                }
            }
        }

        // ── 底部操作栏 ─────────────────────────────────────
        Rectangle {
            id: bottomBar
            anchors.bottom: shadow.bottom
            anchors.left:   shadow.left
            anchors.right:  shadow.right
            height: 44
            radius: 12
            color:  "#F8FAFC"

            // 圆角只在下半部分
            Rectangle {
                anchors.top:   bottomBar.top
                anchors.left:  bottomBar.left
                anchors.right: bottomBar.right
                height: bottomBar.radius
                color:  bottomBar.color
            }

            // 分割线
            Rectangle {
                anchors.top:   bottomBar.top
                anchors.left:  bottomBar.left
                anchors.right: bottomBar.right
                height: 1
                color:  "#E2E8F0"
            }

            Row {
                anchors.centerIn: parent
                spacing: 10

                // 一键复制
                Rectangle {
                    id: copyBtn
                    width: 100; height: 30; radius: 6
                    color: copyArea.containsMouse ? "#DBEAFE" : "#EFF6FF"
                    border.color: "#3B82F6"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 80 } }

                    Text {
                        anchors.centerIn: parent
                        text:  qsTr("复制全部")
                        color: "#3B82F6"
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: copyArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked: {
                            O_OcrVM.copyResultToClipboard()
                            copyFeedbackTimer.start()
                        }
                    }
                }

                // 关闭
                Rectangle {
                    width: 70; height: 30; radius: 6
                    color: closeBtn2Area.containsMouse ? "#F1F5F9" : "transparent"
                    border.color: "#CBD5E1"
                    border.width: 1
                    Behavior on color { ColorAnimation { duration: 80 } }

                    Text {
                        anchors.centerIn: parent
                        text:  qsTr("关闭")
                        color: "#64748B"
                        font.pixelSize: 13
                    }
                    MouseArea {
                        id: closeBtn2Area
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape:  Qt.PointingHandCursor
                        onClicked: ocrWin.close()
                    }
                }
            }
        }

        // 复制成功短暂提示
        Rectangle {
            id: copyFeedback
            anchors.horizontalCenter: shadow.horizontalCenter
            anchors.bottom: bottomBar.top
            anchors.bottomMargin: 6
            width: 100; height: 28; radius: 6
            color: "#22C55E"
            visible: false
            opacity: 0

            Text {
                anchors.centerIn: parent
                text: qsTr("已复制！")
                color: "white"
                font.pixelSize: 12
            }

            Timer {
                id: copyFeedbackTimer
                interval: 1200
                onTriggered: copyFeedback.visible = false
            }

            onVisibleChanged: {
                if (visible) opacity = 1
            }

            Behavior on opacity {
                NumberAnimation { duration: 300 }
            }
        }

        Connections {
            target: copyFeedbackTimer
            function onTriggered() {
                copyFeedback.opacity = 0
            }
        }
    }

    // 启动计时后立即显示反馈
    Connections {
        target: copyArea
        function onClicked() {
            copyFeedback.visible = true
        }
    }
}
