import QtQuick
import QtQuick.Window
import Qt.labs.platform 1.0 as Platform
import QtQuick.Effects
import QtQuick.Controls.Basic
import QtCore
import CustomComponents 1.0
import ".."
import "../Capture"

Window {
    id: stickyWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"

    property string imageUrl: ""
    property rect imgRect: Qt.rect(0, 0, 0, 0)
    property bool contentActive: true
    property bool cleanedUp: false

    property real imgOpacity: 1.0
    property string activeTool: ""
    property bool toolbarVisible: true
    property string toolbarHoverTipText: ""
    property real toolbarHoverTipCenterX: 0
    property bool contextMenuOpen: stickyContextMenu.contextMenuOpen

    property int imageRevision: 0
    property real zoomFactor: 1.0
    property real dpiScale: 1.0
    property size sourceImageSize: Qt.size(1, 1)
    property int imageLogicalWidth: 1
    property int imageLogicalHeight: 1

    readonly property int toolbarHeight: 40
    readonly property int toolbarGap: 8
    readonly property int toolbarTipGap: 6
    readonly property int toolbarTipHeight: 24
    readonly property int shadowRadius: 12
    readonly property int shadowOffset: 4
    readonly property int displayW: Math.max(1, Math.round(imageLogicalWidth * zoomFactor))
    readonly property int displayH: Math.max(1, Math.round(imageLogicalHeight * zoomFactor))
    readonly property int extraToolbarWidth: stickyExtraBar.width
    readonly property int toolbarContentWidth: stickyTool.implicitWidth + extraToolbarWidth + 6
    readonly property int toolbarBlockX: Math.max(8, Math.min(stickyWin.width - toolbarContentWidth - 8, imgContainer.x + (imgContainer.width - toolbarContentWidth) / 2))
    readonly property int toolbarTopY: imgContainer.y + imgContainer.height + stickyWin.toolbarGap

    readonly property int stickyShapeType: {
        switch (activeTool) {
        case "pencil": return 0
        case "rect": return 1
        case "circle": return 2
        case "arrow": return 3
        case "mosaic": return 4
        default: return 1
        }
    }

    width: Math.max(displayW, toolbarContentWidth + 16)
    height: displayH + toolbarGap + toolbarHeight + toolbarTipGap + toolbarTipHeight + 4
    function syncInitialPosition() {
        O_StickyViewModel.positionStickyWindow(stickyWin, imgRect)
    }

    function updateDpiScale() {
        dpiScale = (screen && screen.devicePixelRatio > 0) ? screen.devicePixelRatio : 1.0
    }

    function updateImageLogicalSize() {
        sourceImageSize = O_StickyViewModel.getImageSizeByUrl(stickyWin.imageUrl)
        if (sourceImageSize.width > 0 && sourceImageSize.height > 0) {
            imageLogicalWidth = Math.max(1, Math.round(sourceImageSize.width / dpiScale))
            imageLogicalHeight = Math.max(1, Math.round(sourceImageSize.height / dpiScale))
        }
    }

    Settings {
        id: menuPrefs
        category: "sticky_menu"
        property bool pinSaveMerged: true
        property bool pinCopy: true
        property bool pinAi: false
        property bool pinToolbar: false
        property bool pinClose: false
    }

    function showToolbarTip(text, item) {
        toolbarHoverTipText = text
        if (item) {
            var p = stickyExtraBar.mapFromItem(item, item.width / 2, item.height)
            toolbarHoverTipCenterX = p.x
        } else {
            toolbarHoverTipCenterX = stickyExtraBar.width / 2
        }
    }

    function hideToolbarTip(text) {
        if (text === undefined || toolbarHoverTipText === text)
            toolbarHoverTipText = ""
    }

    function displaySourceUrl() {
        if (typeof stickyWin.imageUrl !== "string" || stickyWin.imageUrl.length === 0)
            return ""
        return stickyWin.imageUrl + "?rev=" + stickyWin.imageRevision
    }

    function setOpacityValue(v) {
        stickyWin.imgOpacity = Math.max(0.1, Math.min(1.0, v))
    }

    function setZoomValue(v) {
        zoomFactor = Math.max(0.3, Math.min(4.0, v))
    }

    function resetDisplaySize() {
        zoomFactor = 1.0
    }

    function updateImageMetricsFromStore() {
        var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
        if (img && img.width > 0 && img.height > 0) {
            stickyPaintBoard.setBackgroundImg(img)
        } else {
            syncPaintboardBackgroundFromDisplay()
        }
    }

    function syncPaintboardBackgroundFromDisplay() {
        if (!stickyWin.visible || !stickyWin.contentActive)
            return
        if (stickyImage.status !== Image.Ready)
            return
        stickyImage.grabToImage(function(result) {
            if (!result || !result.image || result.image.width <= 0 || result.image.height <= 0)
                return
            stickyPaintBoard.setBackgroundImg(result.image)
        }, Qt.size(Math.max(1, Math.abs(imgRect.width)), Math.max(1, Math.abs(imgRect.height))))
    }

    function refreshAfterImageChanged() {
        imageRevision += 1
        stickyPaintBoard.reset()
        updateImageMetricsFromStore()
    }

    function overwriteWithEdits() {
        var layer = stickyPaintBoard.renderToImage()
        var ok = O_StickyViewModel.overwriteWithAnnotations(stickyWin.imageUrl, layer)
        if (ok)
            refreshAfterImageChanged()
    }

    function rotateCurrent(degrees) {
        if (O_StickyViewModel.rotateImage(stickyWin.imageUrl, degrees))
            refreshAfterImageChanged()
    }

    function mirrorCurrent() {
        if (O_StickyViewModel.mirrorImage(stickyWin.imageUrl, true, false))
            refreshAfterImageChanged()
    }

    function saveMergedTo(pathUrl) {
        var layer = stickyPaintBoard.renderToImage()
        O_StickyViewModel.saveMergedImage(stickyWin.imageUrl, layer, pathUrl)
    }

    function completeEditsAndCopy() {
        var layer = stickyPaintBoard.renderToImage()
        if (layer && layer.width > 0 && layer.height > 0)
            overwriteWithEdits()
        O_StickyViewModel.copyImageToClipboard(stickyWin.imageUrl)
        stickyTool.activeTool = ""
        stickyWin.activeTool = ""
        stickyWin.toolbarVisible = false
    }

    function cleanup() {
        if (cleanedUp)
            return
        cleanedUp = true

        stickyWin.visible = false
        contentActive = false
        stickyWin.releaseResources()
        O_StickyViewModel.releaseImage(imageUrl)
        imageUrl = ""
    }

    Component.onCompleted: {
        updateDpiScale()
        syncInitialPosition()
        updateImageMetricsFromStore()
        updateImageLogicalSize()
        stickyWin.raise()
        stickyWin.requestActivate()
    }
    onImageUrlChanged: {
        imageRevision = 0
        sourceImageSize = Qt.size(1, 1)
        imageLogicalWidth = 1
        imageLogicalHeight = 1
        resetDisplaySize()
        refreshAfterImageChanged()
    }

    onActiveToolChanged: {
        if (activeTool === "mosaic")
            updateImageMetricsFromStore()
    }

    onScreenChanged: {
        updateDpiScale()
        updateImageLogicalSize()
    }

    onImgRectChanged: syncInitialPosition()

    Component.onDestruction: cleanup()

    function onClosing(close) {
        close.accepted = true
        chatPanel.close()
        cleanup()
        Qt.callLater(function() { stickyWin.destroy() })
    }

    Rectangle {
        id: imgContainer
        visible: stickyWin.contentActive
        z: 10
        x: 0
        y: 0
        width: stickyWin.displayW
        height: stickyWin.displayH
        radius: 2
        clip: true
        color: "transparent"

        Item {
            id: visualStage
            anchors.centerIn: parent
            width: stickyWin.imageLogicalWidth
            height: stickyWin.imageLogicalHeight
            scale: stickyWin.zoomFactor

            Image {
                id: stickyImage
                anchors.fill: parent
                source: displaySourceUrl()
                fillMode: Image.Stretch
                opacity: stickyWin.imgOpacity
                cache: false
                onStatusChanged: {
                    if (status === Image.Ready)
                        stickyWin.updateImageLogicalSize()
                    if (status === Image.Ready && stickyWin.activeTool === "mosaic" && !stickyPaintBoard.hasBackgroundImg())
                        stickyWin.syncPaintboardBackgroundFromDisplay()
                }
            }

            PaintBoard {
                id: stickyPaintBoard
                anchors.fill: parent
                z: 20
                visible: stickyWin.contentActive
                enabled: stickyWin.activeTool !== ""
                shapeType: stickyWin.stickyShapeType
                penColor: stickyMarkTool.currentColor
                penSize: stickyMarkTool.currentSize
                mosaicBlurLevel: stickyMarkTool.mosaicBlurLevel
            }
        }
    }

    MultiEffect {
        visible: stickyWin.contentActive
        z: 5
        source: stickyImage
        anchors.fill: imgContainer
        shadowEnabled: true
        shadowColor: "#55000000"
        shadowBlur: stickyWin.shadowRadius / 20.0
        shadowVerticalOffset: stickyWin.shadowOffset
        shadowHorizontalOffset: 0
        opacity: stickyWin.imgOpacity
    }

    TZLoadingSpinner {
        anchors.centerIn: imgContainer
        width: stickyWin.displayW
        height: stickyWin.displayH
        visible: O_AIViewModel.isLoading && stickyWin.contentActive
    }

    TZMarkToolInfo {
        id: stickyMarkTool
        z: 32
        visible: stickyWin.toolbarVisible && stickyWin.activeTool !== ""
        toolName: stickyWin.activeTool
        showColorRow: stickyWin.activeTool !== "mosaic"
        x: Math.max(8, toolbarBlockX)
        y: stickyTool.y - height - 6
    }

    TZTool {
        id: stickyTool
        z: 30
        visible: stickyWin.toolbarVisible
        width: implicitWidth
        height: stickyWin.toolbarHeight + 4
        x: stickyWin.toolbarBlockX
        y: stickyWin.toolbarTopY
        showScreenshotButton: false
        showCancelButton: false
        showSaveButton: false
        showStickyButton: false
        showGifRecordButton: false
        showLongScreenshotButton: false
        showHighlightButton: false
        showTextButton: false
        showNumberButton: false

        onActiveToolChanged: {
            stickyWin.activeTool = activeTool
            if (activeTool === "mosaic" && !stickyPaintBoard.hasBackgroundImg()) {
                Qt.callLater(function() {
                    var retryImg = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
                    if (retryImg && retryImg.width > 0 && retryImg.height > 0)
                        stickyPaintBoard.setBackgroundImg(retryImg)
                    if (!stickyPaintBoard.hasBackgroundImg())
                        stickyWin.syncPaintboardBackgroundFromDisplay()
                })
            }
        }
        onSignalOcr: {
            var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
            O_OcrVM.recognize(img)
        }
        onSignalUndo: stickyPaintBoard.undo()
    }

    TZStickyExtraToolBar {
        id: stickyExtraBar
        z: 30
        visible: stickyWin.toolbarVisible
        height: stickyWin.toolbarHeight
        x: stickyWin.toolbarBlockX + stickyTool.width + 6
        y: stickyTool.y + 2
        zoomFactor: stickyWin.zoomFactor
        onZoomOut: setZoomValue(stickyWin.zoomFactor - 0.1)
        onZoomIn: setZoomValue(stickyWin.zoomFactor + 0.1)
        onRotate90: rotateCurrent(90)
        onMirror: mirrorCurrent()
        onResetSize: resetDisplaySize()
        onDoneAndCopy: completeEditsAndCopy()
        onHideToolbar: {
            stickyTool.activeTool = ""
            stickyWin.activeTool = ""
            stickyWin.toolbarVisible = false
        }
        onHoverEntered: function(text, targetItem) { stickyWin.showToolbarTip(text, targetItem) }
        onHoverExited: function(text) { stickyWin.hideToolbarTip(text) }
    }

    Rectangle {
        id: stickyToolbarTip
        parent: stickyWin.contentItem
        visible: stickyWin.toolbarVisible && stickyWin.toolbarHoverTipText.length > 0
        z: 40
        width: Math.min(stickyWin.width - 8, Math.max(64, stickyToolbarTipText.implicitWidth + 16))
        height: 24
        radius: 6
        color: "#0F172A"
        opacity: 0.92
        x: Math.max(4, Math.min(stickyWin.width - width - 4, stickyExtraBar.x + stickyWin.toolbarHoverTipCenterX - width / 2))
        y: Math.min(stickyWin.height - height - 4, stickyExtraBar.y + stickyExtraBar.height + stickyWin.toolbarTipGap)

        Text {
            id: stickyToolbarTipText
            anchors.centerIn: parent
            text: stickyWin.toolbarHoverTipText
            color: "#FFFFFF"
            font.pixelSize: 11
        }
    }

    readonly property int chatPanelGap: 6

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
            Qt.callLater(function() { O_StickyViewModel.releaseImage(old) })
        }
        function onSignalRequestFailed(errorMsg) {
            console.warn("[TZStickyWindow] AI request failed: " + errorMsg)
        }
    }

    function openChatPanel() {
        var imgLeft = stickyWin.x
        var imgTop = stickyWin.y
        var imgBottom = imgTop + stickyWin.displayH

        var screenH = Screen.desktopAvailableHeight
        var screenW = Screen.desktopAvailableWidth

        var cx = imgLeft
        if (cx + chatPanel.win.width > screenW)
            cx = imgLeft + stickyWin.displayW - chatPanel.win.width

        var cy
        if (imgBottom + chatPanelGap + chatPanel.win.height <= screenH)
            cy = imgBottom + chatPanelGap
        else
            cy = imgTop - chatPanelGap - chatPanel.win.height

        chatPanel.open(Math.max(0, cx), Math.max(0, cy))
    }

    TZStickyContextMenu {
        id: stickyContextMenu
        menuPrefs: menuPrefs
        imageUrl: stickyWin.imageUrl
        imgOpacity: stickyWin.imgOpacity
        zoomFactor: stickyWin.zoomFactor
        toolbarVisible: stickyWin.toolbarVisible
        saveFolder: O_ImageSaver.savePath
        overwriteWithEdits: function() { stickyWin.overwriteWithEdits() }
        copyImageToClipboard: function(url) { O_StickyViewModel.copyImageToClipboard(url) }
        openChatPanel: function() { stickyWin.openChatPanel() }
        hideOrToggleToolbar: function() {
            stickyWin.toolbarVisible = !stickyWin.toolbarVisible
            if (!stickyWin.toolbarVisible) {
                stickyTool.activeTool = ""
                stickyWin.activeTool = ""
            }
        }
        setOpacityValue: function(value) { stickyWin.setOpacityValue(value) }
        setZoomValue: function(value) { stickyWin.setZoomValue(value) }
        rotateCurrent: function(degrees) { stickyWin.rotateCurrent(degrees) }
        mirrorCurrent: function() { stickyWin.mirrorCurrent() }
        resetDisplaySize: function() { stickyWin.resetDisplaySize() }
        resetWindowPosition: function() { stickyWin.syncInitialPosition() }
        saveOriginalImage: function(url, pathUrl) { O_StickyViewModel.saveImage(url, pathUrl) }
        saveMergedTo: function(pathUrl) { stickyWin.saveMergedTo(pathUrl) }
        closeWindow: function() { stickyWin.close() }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton
        enabled: stickyWin.activeTool === "" && !stickyWin.contextMenuOpen
        property point dragPos: Qt.point(0, 0)
        property bool leftDragActive: false
        z: 0

        function inImageArea(mx, my) {
            return mx >= 0
                    && mx <= stickyWin.displayW
                    && my >= 0
                    && my <= stickyWin.displayH
        }

        onPressed: function(mouse) {
            if (mouse.button === Qt.LeftButton) {
                dragPos = Qt.point(mouseX, mouseY)
                leftDragActive = true
            } else {
                leftDragActive = false
            }
        }

        onPositionChanged: function(mouse) {
            if (pressed && leftDragActive && (mouse.buttons & Qt.LeftButton) && stickyWin.activeTool === "" && !stickyWin.contextMenuOpen) {
                stickyWin.x += mouseX - dragPos.x
                stickyWin.y += mouseY - dragPos.y
            }
        }

        onReleased: function(mouse) {
            if (!(mouse.buttons & Qt.LeftButton))
                leftDragActive = false
        }

        onDoubleClicked: function(mouse) {
            if (mouse.button === Qt.LeftButton && inImageArea(mouseX, mouseY))
                resetDisplaySize()
        }

        onClicked: function(mouse) {
            if (mouse.button === Qt.RightButton) {
                leftDragActive = false
                stickyContextMenu.open()
            }
        }

        onWheel: function(wheel) {
            if (wheel.angleDelta.y > 0)
                setZoomValue(stickyWin.zoomFactor + 0.1)
            else if (wheel.angleDelta.y < 0)
                setZoomValue(stickyWin.zoomFactor - 0.1)
            wheel.accepted = true
        }
    }

}
