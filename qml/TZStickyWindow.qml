import QtQuick
import QtQuick.Window
import Qt.labs.platform 1.0 as Platform
import QtQuick.Effects
import QtQuick.Controls.Basic
import QtCore
import CustomComponents 1.0

Window {
    id: stickyWin
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint | Qt.Tool
    color: "transparent"

    readonly property real minOpacity: 0.1
    readonly property real maxOpacity: 1.0
    readonly property real minZoomFactor: 0.3
    readonly property real maxZoomFactor: 4.0
    readonly property int toolbarButtonSize: 30
    readonly property int toolbarIconSize: 18
    readonly property color toolbarHoverColor: "#F1F5F9"
    readonly property color toolbarActiveColor: "#EFF6FF"
    readonly property color toolbarActiveBorderColor: "#3B82F6"

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
    readonly property int imageLeft: imgRect.width >= 0 ? imgRect.x : imgRect.x + imgRect.width
    readonly property int imageTop: imgRect.height >= 0 ? imgRect.y : imgRect.y + imgRect.height

    readonly property int displayW: Math.max(1, Math.round(imageLogicalWidth * zoomFactor))
    readonly property int displayH: Math.max(1, Math.round(imageLogicalHeight * zoomFactor))
    readonly property int toolbarContentWidth: Math.max(560, stickyRow.implicitWidth + 16)
    readonly property string displaySourceUrl: {
        if (typeof stickyWin.imageUrl !== "string" || stickyWin.imageUrl.length === 0)
            return ""
        return stickyWin.imageUrl + "?rev=" + stickyWin.imageRevision
    }
    readonly property var stickyShapeMap: ({
        pencil: 0,
        rect: 1,
        circle: 2,
        arrow: 3,
        mosaic: 4
    })
    readonly property var drawToolButtons: [
        { tool: "pencil", tip: qsTr("Pencil"), icon: "qrc:/resource/img/lc_pencil.svg" },
        { tool: "rect", tip: qsTr("Rectangle"), icon: "qrc:/resource/img/lc_square.svg" },
        { tool: "circle", tip: qsTr("Circle"), icon: "qrc:/resource/img/lc_circle.svg" },
        { tool: "arrow", tip: qsTr("Arrow"), icon: "qrc:/resource/img/lc_arrow.svg" },
        { tool: "mosaic", tip: qsTr("Mosaic"), icon: "qrc:/resource/img/lc_mosaic.svg" }
    ]
    readonly property var zoomActionButtons: [
        { action: "zoomOut", tip: qsTr("Zoom out"), text: "-", textSize: 16 },
        { action: "zoomIn", tip: qsTr("Zoom in"), text: "+", textSize: 16 }
    ]
    readonly property var transformActionButtons: [
        { action: "rotate", tip: qsTr("Rotate 90°"), text: "↻", textSize: 14 },
        { action: "mirror", tip: qsTr("Mirror"), text: "⇋", textSize: 14 },
        { action: "resetSize", tip: qsTr("Reset size"), text: "1:1", textSize: 10 }
    ]
    readonly property var editActionButtons: [
        { action: "doneAndCopy", tip: qsTr("Done and copy"), icon: "qrc:/resource/img/lc_check.svg", hoverColor: "#E0F2FE" },
        { action: "undo", tip: qsTr("Undo"), icon: "qrc:/resource/img/lc_undo.svg", hoverColor: toolbarActiveColor },
        { action: "hideToolbar", tip: qsTr("Hide toolbar"), icon: "qrc:/resource/img/lc_x.svg", hoverColor: "#FEE2E2" }
    ]

    readonly property int stickyShapeType: {
        return stickyShapeMap[activeTool] !== undefined ? stickyShapeMap[activeTool] : 1
    }

    width: Math.max(displayW + shadowRadius * 2, toolbarContentWidth + 16)
    height: displayH + shadowRadius * 2 + shadowOffset + toolbarGap + toolbarHeight + toolbarTipGap + toolbarTipHeight + 4
    x: imageLeft - shadowRadius
    y: imageTop - shadowRadius

    Settings {
        id: menuPrefs
        category: "sticky_menu"
        property bool pinSaveMerged: true
        property bool pinCopy: true
        property bool pinAi: false
        property bool pinToolbar: false
        property bool pinClose: false
    }

    function setActiveTool(name) {
        if (activeTool === name)
            activeTool = ""
        else
            activeTool = name
    }

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(maxValue, value))
    }

    function hasValidImage(image) {
        return image && image.width > 0 && image.height > 0
    }

    function clearActiveTool() {
        stickyWin.activeTool = ""
    }

    function setToolbarVisible(visible) {
        stickyWin.toolbarVisible = visible
        if (!visible)
            clearActiveTool()
    }

    function toggleToolbarVisible() {
        setToolbarVisible(!stickyWin.toolbarVisible)
    }

    function showToolbarTip(text, item) {
        toolbarHoverTipText = text
        if (item) {
            var p = stickyToolbar.mapFromItem(item, item.width / 2, item.height)
            toolbarHoverTipCenterX = p.x
        } else {
            toolbarHoverTipCenterX = stickyToolbar.width / 2
        }
    }

    function hideToolbarTip(text) {
        if (text === undefined || toolbarHoverTipText === text)
            toolbarHoverTipText = ""
    }

    function setOpacityValue(v) {
        stickyWin.imgOpacity = clamp(v, minOpacity, maxOpacity)
    }

    function setZoomValue(v) {
        zoomFactor = clamp(v, minZoomFactor, maxZoomFactor)
    }

    function resetDisplaySize() {
        zoomFactor = 1.0
    }

    function updateImageMetricsFromStore() {
        var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
        if (hasValidImage(img)) {
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
            if (!hasValidImage(result && result.image))
                return
            stickyPaintBoard.setBackgroundImg(result.image)
        }, Qt.size(Math.max(1, Math.abs(imgRect.width)), Math.max(1, Math.abs(imgRect.height))))
    }

    function refreshAfterImageChanged() {
        imageRevision += 1
        stickyPaintBoard.reset()
        updateImageMetricsFromStore()
    }

    function renderAnnotationLayer() {
        return stickyPaintBoard.renderToImage()
    }

    function overwriteWithEdits() {
        var layer = renderAnnotationLayer()
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
        var layer = renderAnnotationLayer()
        O_StickyViewModel.saveMergedImage(stickyWin.imageUrl, layer, pathUrl)
    }

    function completeEditsAndCopy() {
        var layer = renderAnnotationLayer()
        if (hasValidImage(layer))
            overwriteWithEdits()
        O_StickyViewModel.copyImageToClipboard(stickyWin.imageUrl)
        setToolbarVisible(false)
    }

    function ensureMosaicBackground() {
        var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
        if (hasValidImage(img))
            stickyPaintBoard.setBackgroundImg(img)

        setActiveTool("mosaic")
        if (stickyWin.activeTool !== "mosaic" || stickyPaintBoard.hasBackgroundImg())
            return

        Qt.callLater(function() {
            var retryImg = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
            if (hasValidImage(retryImg))
                stickyPaintBoard.setBackgroundImg(retryImg)
            if (!stickyPaintBoard.hasBackgroundImg())
                stickyWin.syncPaintboardBackgroundFromDisplay()
        })
    }

    function handleToolbarAction(action) {
        switch (action) {
        case "zoomOut":
            setZoomValue(stickyWin.zoomFactor - 0.1)
            break
        case "zoomIn":
            setZoomValue(stickyWin.zoomFactor + 0.1)
            break
        case "rotate":
            rotateCurrent(90)
            break
        case "mirror":
            mirrorCurrent()
            break
        case "resetSize":
            resetDisplaySize()
            break
        case "doneAndCopy":
            completeEditsAndCopy()
            break
        case "undo":
            stickyPaintBoard.undo()
            break
        case "hideToolbar":
            setToolbarVisible(false)
            break
        case "ocr":
            var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
            O_OcrVM.recognize(img)
            break
        }
    }

    function handleDrawTool(tool) {
        if (tool === "mosaic") {
            ensureMosaicBackground()
            return
        }
        setActiveTool(tool)
    }

    function resetWindowPosition() {
        stickyWin.x = stickyWin.imageLeft - stickyWin.shadowRadius
        stickyWin.y = stickyWin.imageTop - stickyWin.shadowRadius
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

    Component.onCompleted: {
        updateDpiScale()
        updateImageMetricsFromStore()
        updateImageLogicalSize()
        stickyWin.raise()
        stickyWin.requestActivate()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (stickyWin.activeTool !== "") {
                clearActiveTool()
                return
            }
            if (stickyWin.toolbarVisible) {
                setToolbarVisible(false)
                return
            }
            stickyWin.close()
        }
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
        x: stickyWin.shadowRadius
        y: stickyWin.shadowRadius + stickyWin.shadowOffset / 2
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
                source: stickyWin.displaySourceUrl
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
        x: Math.max(8, stickyToolbar.x)
        y: stickyToolbar.y - height - 6
    }

    Rectangle {
        id: stickyToolbar
        z: 30
        visible: stickyWin.toolbarVisible
        width: stickyWin.toolbarContentWidth
        height: stickyWin.toolbarHeight
        radius: 10
        color: "#FFFFFF"
        border.color: "#E2E8F0"
        border.width: 1
        x: Math.max(8, Math.min(stickyWin.width - width - 8, stickyWin.shadowRadius + (stickyWin.displayW - width) / 2))
        y: stickyWin.shadowRadius + stickyWin.shadowOffset / 2 + stickyWin.displayH + stickyWin.toolbarGap

        Row {
            id: stickyRow
            anchors.centerIn: parent
            spacing: 6

            Rectangle {
                width: stickyWin.toolbarButtonSize; height: stickyWin.toolbarButtonSize; radius: 7
                color: ocrArea.containsMouse ? "#F0FDF4" : "transparent"
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_ocr.svg"; width: stickyWin.toolbarIconSize; height: stickyWin.toolbarIconSize }
                MouseArea {
                    id: ocrArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: stickyWin.showToolbarTip(qsTr("OCR"), parent)
                    onExited: stickyWin.hideToolbarTip(qsTr("OCR"))
                    onPressed: stickyWin.handleToolbarAction("ocr")
                }
            }

            Rectangle { width: 1; height: 18; color: "#E2E8F0"; anchors.verticalCenter: parent.verticalCenter }

            Repeater {
                model: stickyWin.drawToolButtons

                delegate: Rectangle {
                    property var buttonData: modelData
                    width: stickyWin.toolbarButtonSize
                    height: stickyWin.toolbarButtonSize
                    radius: 7
                    color: stickyWin.activeTool === buttonData.tool ? stickyWin.toolbarActiveColor : (hoverArea.containsMouse ? stickyWin.toolbarHoverColor : "transparent")
                    border.color: stickyWin.activeTool === buttonData.tool ? stickyWin.toolbarActiveBorderColor : "transparent"
                    border.width: 1

                    Image {
                        anchors.centerIn: parent
                        source: buttonData.icon
                        width: stickyWin.toolbarIconSize
                        height: stickyWin.toolbarIconSize
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: stickyWin.showToolbarTip(parent.buttonData.tip, parent)
                        onExited: stickyWin.hideToolbarTip(parent.buttonData.tip)
                        onPressed: stickyWin.handleDrawTool(parent.buttonData.tool)
                    }
                }
            }

            Rectangle { width: 1; height: 18; color: "#E2E8F0"; anchors.verticalCenter: parent.verticalCenter }

            Repeater {
                model: [stickyWin.zoomActionButtons[0]]

                delegate: Rectangle {
                    property var buttonData: modelData
                    width: stickyWin.toolbarButtonSize
                    height: stickyWin.toolbarButtonSize
                    radius: 7
                    color: hoverArea.containsMouse ? stickyWin.toolbarHoverColor : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: parent.buttonData.text
                        color: "#334155"
                        font.pixelSize: parent.buttonData.textSize
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: stickyWin.showToolbarTip(parent.buttonData.tip, parent)
                        onExited: stickyWin.hideToolbarTip(parent.buttonData.tip)
                        onPressed: stickyWin.handleToolbarAction(parent.buttonData.action)
                    }
                }
            }

            Rectangle { width: 40; height: 30; radius: 7; color: "transparent"; Text { anchors.centerIn: parent; text: Math.round(stickyWin.zoomFactor * 100) + "%"; color: "#64748B"; font.pixelSize: 11 } }

            Repeater {
                model: [stickyWin.zoomActionButtons[1]].concat(stickyWin.transformActionButtons)

                delegate: Rectangle {
                    property var buttonData: modelData
                    width: stickyWin.toolbarButtonSize
                    height: stickyWin.toolbarButtonSize
                    radius: 7
                    color: hoverArea.containsMouse ? stickyWin.toolbarHoverColor : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: parent.buttonData.text
                        color: "#334155"
                        font.pixelSize: parent.buttonData.textSize
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: stickyWin.showToolbarTip(parent.buttonData.tip, parent)
                        onExited: stickyWin.hideToolbarTip(parent.buttonData.tip)
                        onPressed: stickyWin.handleToolbarAction(parent.buttonData.action)
                    }
                }
            }

            Repeater {
                model: stickyWin.editActionButtons

                delegate: Rectangle {
                    property var buttonData: modelData
                    width: stickyWin.toolbarButtonSize
                    height: stickyWin.toolbarButtonSize
                    radius: 7
                    color: hoverArea.containsMouse ? buttonData.hoverColor : "transparent"

                    Image {
                        anchors.centerIn: parent
                        source: buttonData.icon
                        width: stickyWin.toolbarIconSize
                        height: stickyWin.toolbarIconSize
                    }

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: stickyWin.showToolbarTip(parent.buttonData.tip, parent)
                        onExited: stickyWin.hideToolbarTip(parent.buttonData.tip)
                        onPressed: stickyWin.handleToolbarAction(parent.buttonData.action)
                    }
                }
            }
        }
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
        x: Math.max(4, Math.min(stickyWin.width - width - 4, stickyToolbar.x + stickyWin.toolbarHoverTipCenterX - width / 2))
        y: Math.min(stickyWin.height - height - 4, stickyToolbar.y + stickyToolbar.height + stickyWin.toolbarTipGap)

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
        var imgLeft = stickyWin.x + stickyWin.shadowRadius
        var imgTop = stickyWin.y + stickyWin.shadowRadius
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
        hideOrToggleToolbar: function() { stickyWin.toggleToolbarVisible() }
        setOpacityValue: function(value) { stickyWin.setOpacityValue(value) }
        setZoomValue: function(value) { stickyWin.setZoomValue(value) }
        rotateCurrent: function(degrees) { stickyWin.rotateCurrent(degrees) }
        mirrorCurrent: function() { stickyWin.mirrorCurrent() }
        resetDisplaySize: function() { stickyWin.resetDisplaySize() }
        resetWindowPosition: function() { stickyWin.resetWindowPosition() }
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
            return mx >= stickyWin.shadowRadius
                    && mx <= stickyWin.shadowRadius + stickyWin.displayW
                    && my >= stickyWin.shadowRadius + stickyWin.shadowOffset / 2
                    && my <= stickyWin.shadowRadius + stickyWin.shadowOffset / 2 + stickyWin.displayH
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
