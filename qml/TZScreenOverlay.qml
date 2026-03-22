import QtQuick
import QtQuick.Window
import QtQuick.Shapes

// TZScreenOverlay — 辅屏截图覆盖窗口
//
// 每块非主屏创建一个此窗口，覆盖该屏全部区域。
// 职责：
//   1. 显示半透明蒙层 + 选区镂空 + 窗口识别高亮
//   2. 将本地鼠标坐标转为虚拟桌面绝对坐标，委托给主窗口（delegate）处理
//
// 注入属性：
//   delegate  — 主窗口（root / TZWindow），提供共享截图状态和处理函数
//   screenVX  — 本屏 virtualX（虚拟桌面绝对 X）
//   screenVY  — 本屏 virtualY（虚拟桌面绝对 Y）

Window {
    id: overlay
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint
    visible: delegate ? (!delegate.gifRecordMode && delegate.captureUiReady) : true

    property var delegate: null
    property int screenVX: 0
    property int screenVY: 0

    // 从 delegate 读取共享状态
    property rect screenshotArea: delegate ? delegate.screenshotArea : Qt.rect(0,0,0,0)
    property rect highlightRect:  delegate ? delegate.highlightRect  : Qt.rect(0,0,0,0)

    function toAbsX(lx) { return lx + screenVX }
    function toAbsY(ly) { return ly + screenVY }

    // 绝对坐标 → 本窗口本地坐标（供绘制用）
    readonly property rect localArea: Qt.rect(
        screenshotArea.x - screenVX, screenshotArea.y - screenVY,
        screenshotArea.width, screenshotArea.height)
    readonly property rect localHighlight: Qt.rect(
        highlightRect.x - screenVX, highlightRect.y - screenVY,
        highlightRect.width, highlightRect.height)

    // ── 半透明蒙层 + 选区镂空 ─────────────────────────────
    Shape {
        anchors.fill: parent
        ShapePath {
            strokeWidth: -1
            fillColor: "#99000000"
            startX: 0; startY: 0
            PathLine { x: overlay.width;  y: 0 }
            PathLine { x: overlay.width;  y: overlay.height }
            PathLine { x: 0;              y: overlay.height }
            PathLine { x: 0;              y: 0 }
            PathMove { x: overlay.localArea.x;                           y: overlay.localArea.y }
            PathLine { x: overlay.localArea.x + overlay.localArea.width; y: overlay.localArea.y }
            PathLine { x: overlay.localArea.x + overlay.localArea.width; y: overlay.localArea.y + overlay.localArea.height }
            PathLine { x: overlay.localArea.x;                           y: overlay.localArea.y + overlay.localArea.height }
            PathLine { x: overlay.localArea.x;                           y: overlay.localArea.y }
        }
    }

    // ── 窗口识别高亮框 ────────────────────────────────────
    Rectangle {
        visible: screenshotArea.width === 0 && screenshotArea.height === 0
                 && highlightRect.width > 0 && highlightRect.height > 0
        x: overlay.localHighlight.x
        y: overlay.localHighlight.y
        width:  overlay.localHighlight.width
        height: overlay.localHighlight.height
        color: "transparent"
        border.color: "#3B82F6"
        border.width: 2
        z: 5
        Rectangle { anchors.fill: parent; color: "#153B82F6" }
    }

    // ── 鼠标输入：全部转为绝对坐标，委托给主窗口处理 ─────
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.CrossCursor

        onPressed: function(mouse) {
            if (!delegate) return
            var ax = toAbsX(mouseX), ay = toAbsY(mouseY)
            delegate.handleMousePressed(ax, ay)
        }

        onPositionChanged: function(mouse) {
            if (!delegate) return
            var ax = toAbsX(mouseX), ay = toAbsY(mouseY)
            delegate.handleMouseMoved(ax, ay, pressed)
        }

        onReleased: function(mouse) {
            if (!delegate) return
            delegate.handleMouseReleased()
        }

        onWheel: function(wheel) { wheel.accepted = true }
    }
}
