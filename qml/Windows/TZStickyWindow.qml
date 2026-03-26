import QtQuick
import QtQuick.Window
import Qt5Compat.GraphicalEffects
import Qt.labs.platform 1.0 as Platform
import "../Capture"
import CustomComponents 1.0

Window {
    id: stickyWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"
    visible: true

    property string imageUrl: ""
    property rect imgRect: Qt.rect(0, 0, 0, 0)
    property bool cleanedUp: false
    property size sourceImageSize: Qt.size(1, 1)
    property bool toolbarVisible: true
    property real zoomFactor: 1.0
    property int windowWidthValue: 1
    property int windowHeightValue: 1
    readonly property int shadowPadding: 12
    readonly property color shadowColor: "#2F8FFF"
    readonly property int toolbarHeight: toolBar.implicitHeight
    readonly property int toolbarGap: 8
    readonly property int toolbarTipReserve: 30
    readonly property int markToolGap: 8
    readonly property int contentX: stickyWin.shadowPadding
    readonly property int contentY: stickyWin.shadowPadding
    readonly property int contentWidth: Math.max(1, stickyWin.width - stickyWin.shadowPadding * 2)
    readonly property int contentHeight: Math.max(1, stickyWin.height - stickyWin.shadowPadding * 2
                                                  - (stickyWin.toolbarVisible ? (stickyWin.toolbarHeight + stickyWin.toolbarGap + stickyWin.toolbarTipReserve) : 0))

    width: windowWidthValue
    height: windowHeightValue
    minimumWidth: windowWidthValue
    maximumWidth: windowWidthValue
    minimumHeight: windowHeightValue
    maximumHeight: windowHeightValue

    function updateImageSize() {
        sourceImageSize = O_StickyViewModel.getImageSizeByUrl(imageUrl)
        if (!sourceImageSize || sourceImageSize.width <= 0 || sourceImageSize.height <= 0)
            sourceImageSize = Qt.size(1, 1)
        updateWindowSize()
    }

    function updateWindowSize() {
        var imageWidth = Math.max(1, Math.floor(sourceImageSize.width * zoomFactor))
        var imageHeight = Math.max(1, Math.floor(sourceImageSize.height * zoomFactor))
        windowWidthValue = imageWidth + shadowPadding * 2
        windowHeightValue = imageHeight + shadowPadding * 2
                        + (toolbarVisible ? (toolbarHeight + toolbarGap + toolbarTipReserve) : 0)
        Qt.callLater(applyNativeSize)
    }

    function applyNativeSize() {
        O_StickyViewModel.resizeStickyWindow(stickyWin, windowWidthValue, windowHeightValue)
    }

    function setZoomFactor(value) {
        zoomFactor = Math.max(0.3, Math.min(4.0, value))
        updateWindowSize()
    }

    function resetDisplaySize() {
        setZoomFactor(1.0)
    }

    function syncInitialPosition() {
        O_StickyViewModel.positionStickyWindow(stickyWin,
                                              Qt.rect(imgRect.x - shadowPadding,
                                                      imgRect.y - shadowPadding,
                                                      imgRect.width + shadowPadding * 2,
                                                      imgRect.height + shadowPadding * 2))
    }

    function cleanup() {
        if (cleanedUp)
            return
        cleanedUp = true
        chatPanel.close()
        O_StickyViewModel.releaseImage(imageUrl)
        imageUrl = ""
    }

    function syncPaintBoardBackground() {
        if (!imageUrl) {
            paintBoard.reset()
            return
        }

        var bg = O_StickyViewModel.getImageByUrl(imageUrl)
        paintBoard.reset()
        paintBoard.setBackgroundImg(bg)
    }

    function openChatPanel() {
        var targetScreen = stickyWin.screen
        if (!targetScreen || !targetScreen.availableGeometry) {
            chatPanel.open(stickyWin.x, stickyWin.y + stickyWin.height + 6, targetScreen)
            return
        }

        var available = targetScreen.availableGeometry
        var panelX = stickyWin.x
        var panelY = stickyWin.y + stickyWin.height + 6

        if (panelY + chatPanel.win.height > available.y + available.height)
            panelY = stickyWin.y - chatPanel.win.height - 6

        panelX = Math.max(available.x, Math.min(panelX, available.x + available.width - chatPanel.win.width))
        panelY = Math.max(available.y, Math.min(panelY, available.y + available.height - chatPanel.win.height))

        chatPanel.open(Math.round(panelX), Math.round(panelY), targetScreen)
    }

    Component.onCompleted: {
        updateImageSize()
        syncPaintBoardBackground()
        syncInitialPosition()
        applyNativeSize()
        stickyWin.raise()
        stickyWin.requestActivate()
    }

    onImageUrlChanged: {
        updateImageSize()
        syncPaintBoardBackground()
    }

    function onClosing(close) {
        close.accepted = true
        cleanup()
        Qt.callLater(function() { stickyWin.destroy() })
    }

    RectangularGlow {
        id: stickyShadow
        x: stickyWin.contentX
        y: stickyWin.contentY
        width: stickyWin.contentWidth
        height: stickyWin.contentHeight
        glowRadius: 10
        spread: 0.08
        color: stickyWin.shadowColor
        cornerRadius: 2
        opacity: 0.45
        cached: true
    }

    PaintBoard {
        id: paintBoard
        x: stickyWin.contentX
        y: stickyWin.contentY
        width: stickyWin.contentWidth
        height: stickyWin.contentHeight
        z: 5
        visible: true
        enabled: toolBar.activeTool !== ""
        shapeType: toolBar.shapeType
        penColor: markToolInfo.currentColor
        penSize: markToolInfo.currentSize
    }

    TZTool {
        id: toolBar
        visible: stickyWin.toolbarVisible
        x: Math.max(0, Math.floor((stickyWin.width - width) / 2))
        y: stickyWin.contentY + stickyWin.contentHeight + stickyWin.toolbarGap
        showScreenshotButton: false
        showCancelButton: false
        showSaveButton: false
        showStickyButton: false
        showGifRecordButton: false
        showLongScreenshotButton: false
        showHighlightButton: false
        showTextButton: false
        showNumberButton: false
        showRectButton: false
        showCircleButton: false
        showArrowButton: false
        showMosaicButton: false
        showPencilButton: true
        showUndoButton: true

        onSignalOcr: {}
        onSignalUndo: paintBoard.undo()
    }

    TZMarkToolInfo {
        id: markToolInfo
        visible: toolBar.visible && toolBar.markToolInfoVisible && toolBar.activeTool === "pencil"
        z: 6
        x: Math.max(stickyWin.shadowPadding, Math.min(stickyWin.width - width - stickyWin.shadowPadding, toolBar.x))
        y: Math.max(stickyWin.shadowPadding, toolBar.y - height - stickyWin.markToolGap)
        toolName: toolBar.activeTool
        showColorRow: true
    }

    TZChat {
        id: chatPanel
        imageUrl: stickyWin.imageUrl

        onSignalSendPrompt: function(text) {
            O_AIViewModel.sendPrompt(text, stickyWin.imageUrl)
        }
    }

    Connections {
        target: O_AIViewModel

        function onSignalRequestComplete(oldImageUrl, newImageUrl) {
            if (oldImageUrl !== stickyWin.imageUrl)
                return
            var old = oldImageUrl
            stickyWin.imageUrl = newImageUrl
            syncPaintBoardBackground()
            Qt.callLater(function() { O_StickyViewModel.releaseImage(old) })
        }

        function onSignalRequestFailed(errorMsg) {
            console.warn("[TZStickyWindow] AI request failed: " + errorMsg)
        }
    }

    Platform.Menu {
        id: contextMenu

        Platform.MenuItem {
            text: qsTr("复制")
            onTriggered: O_StickyViewModel.copyImageToClipboard(stickyWin.imageUrl)
        }

        Platform.MenuItem {
            text: qsTr("AI 编辑")
            onTriggered: stickyWin.openChatPanel()
        }

        Platform.MenuItem {
            text: stickyWin.toolbarVisible ? qsTr("隐藏工具栏") : qsTr("显示工具栏")
            onTriggered: {
                stickyWin.toolbarVisible = !stickyWin.toolbarVisible
                updateWindowSize()
            }
        }

        Platform.MenuItem {
            text: qsTr("恢复原图大小")
            onTriggered: stickyWin.resetDisplaySize()
        }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: qsTr("保存原图")
            onTriggered: saveDialog.open()
        }

        Platform.MenuItem {
            text: qsTr("关闭")
            onTriggered: stickyWin.close()
        }
    }

    Platform.FileDialog {
        id: saveDialog
        title: qsTr("选择保存位置")
        folder: O_ImageSaver.savePath
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: O_StickyViewModel.saveImage(stickyWin.imageUrl, saveDialog.file)
    }

    MouseArea {
        id: dragArea
        x: stickyWin.contentX
        y: stickyWin.contentY
        width: stickyWin.contentWidth
        height: stickyWin.contentHeight
        enabled: toolBar.activeTool === ""
        acceptedButtons: Qt.LeftButton
        property point pressOffset: Qt.point(0, 0)

        onPressed: function(mouse) {
            pressOffset = Qt.point(mouse.x, mouse.y)
        }

        onPositionChanged: function(mouse) {
            if (pressed && (mouse.buttons & Qt.LeftButton)) {
                stickyWin.x += mouse.x - pressOffset.x
                stickyWin.y += mouse.y - pressOffset.y
            }
        }

        onWheel: function(wheel) {
            if (wheel.angleDelta.y > 0)
                stickyWin.setZoomFactor(stickyWin.zoomFactor + 0.1)
            else if (wheel.angleDelta.y < 0)
                stickyWin.setZoomFactor(stickyWin.zoomFactor - 0.1)
            wheel.accepted = true
        }
    }

    MouseArea {
        x: stickyWin.contentX
        y: stickyWin.contentY
        width: stickyWin.contentWidth
        height: stickyWin.contentHeight + (stickyWin.toolbarVisible ? (stickyWin.toolbarHeight + stickyWin.toolbarGap + stickyWin.toolbarTipReserve) : 0)
        acceptedButtons: Qt.RightButton

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton)
                contextMenu.open()
        }
    }
}
