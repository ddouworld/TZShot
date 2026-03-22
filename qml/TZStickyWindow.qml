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

    property string imageUrl: ""
    property rect imgRect: Qt.rect(0, 0, 0, 0)
    property bool contentActive: true
    property bool cleanedUp: false

    property real imgOpacity: 1.0
    property string activeTool: ""
    property bool toolbarVisible: true
    property string toolbarHoverTipText: ""
    property real toolbarHoverTipCenterX: 0
    property bool contextMenuOpen: false

    property int imageRevision: 0
    property int imageBaseW: Math.max(1, Math.abs(imgRect.width))
    property int imageBaseH: Math.max(1, Math.abs(imgRect.height))
    property real zoomFactor: 1.0

    readonly property int toolbarHeight: 40
    readonly property int toolbarGap: 8
    readonly property int toolbarTipGap: 6
    readonly property int toolbarTipHeight: 24
    readonly property int shadowRadius: 12
    readonly property int shadowOffset: 4

    readonly property int displayW: Math.max(1, Math.round(imageBaseW * zoomFactor))
    readonly property int displayH: Math.max(1, Math.round(imageBaseH * zoomFactor))

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

    width: displayW + shadowRadius * 2
    height: displayH + shadowRadius * 2 + shadowOffset + toolbarGap + toolbarHeight + toolbarTipGap + toolbarTipHeight + 4
    x: (imgRect.width >= 0 ? imgRect.x : imgRect.x + imgRect.width) - shadowRadius
    y: (imgRect.height >= 0 ? imgRect.y : imgRect.y + imgRect.height) - shadowRadius

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
            imageBaseW = img.width
            imageBaseH = img.height
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
            if (imageBaseW <= 1 || imageBaseH <= 1) {
                imageBaseW = result.image.width
                imageBaseH = result.image.height
            }
            stickyPaintBoard.setBackgroundImg(result.image)
        }, Qt.size(Math.max(1, imageBaseW), Math.max(1, imageBaseH)))
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
        updateImageMetricsFromStore()
        stickyWin.raise()
        stickyWin.requestActivate()
    }

    Shortcut {
        sequence: "Esc"
        onActivated: {
            if (stickyWin.activeTool !== "") {
                stickyWin.activeTool = ""
                return
            }
            if (stickyWin.toolbarVisible) {
                stickyWin.toolbarVisible = false
                return
            }
            stickyWin.close()
        }
    }

    onImageUrlChanged: {
        imageRevision = 0
        resetDisplaySize()
        refreshAfterImageChanged()
    }

    onActiveToolChanged: {
        if (activeTool === "mosaic")
            updateImageMetricsFromStore()
    }

    Component.onDestruction: cleanup()

    onClosing: function(close) {
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
            width: stickyWin.imageBaseW
            height: stickyWin.imageBaseH
            scale: stickyWin.zoomFactor

            Image {
                id: stickyImage
                anchors.fill: parent
                source: displaySourceUrl()
                fillMode: Image.Stretch
                opacity: stickyWin.imgOpacity
                cache: false
                onStatusChanged: {
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
        width: Math.min(stickyWin.width - 16, Math.max(560, stickyRow.implicitWidth + 16))
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
                width: 30; height: 30; radius: 7
                color: ocrArea.containsMouse ? "#F0FDF4" : "transparent"
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_ocr.svg"; width: 18; height: 18 }
                MouseArea {
                    id: ocrArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: stickyWin.showToolbarTip(qsTr("OCR"), parent)
                    onExited: stickyWin.hideToolbarTip(qsTr("OCR"))
                    onPressed: {
                        var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
                        O_OcrVM.recognize(img)
                    }
                }
            }

            Rectangle { width: 1; height: 18; color: "#E2E8F0"; anchors.verticalCenter: parent.verticalCenter }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: (stickyWin.activeTool === "pencil") ? "#EFF6FF" : (pencilArea.containsMouse ? "#F1F5F9" : "transparent")
                border.color: (stickyWin.activeTool === "pencil") ? "#3B82F6" : "transparent"
                border.width: 1
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_pencil.svg"; width: 18; height: 18 }
                MouseArea { id: pencilArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Pencil"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Pencil")); onPressed: stickyWin.setActiveTool("pencil") }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: (stickyWin.activeTool === "rect") ? "#EFF6FF" : (rectArea.containsMouse ? "#F1F5F9" : "transparent")
                border.color: (stickyWin.activeTool === "rect") ? "#3B82F6" : "transparent"
                border.width: 1
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_square.svg"; width: 18; height: 18 }
                MouseArea { id: rectArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Rectangle"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Rectangle")); onPressed: stickyWin.setActiveTool("rect") }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: (stickyWin.activeTool === "circle") ? "#EFF6FF" : (circleArea.containsMouse ? "#F1F5F9" : "transparent")
                border.color: (stickyWin.activeTool === "circle") ? "#3B82F6" : "transparent"
                border.width: 1
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_circle.svg"; width: 18; height: 18 }
                MouseArea { id: circleArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Circle"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Circle")); onPressed: stickyWin.setActiveTool("circle") }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: (stickyWin.activeTool === "arrow") ? "#EFF6FF" : (arrowArea.containsMouse ? "#F1F5F9" : "transparent")
                border.color: (stickyWin.activeTool === "arrow") ? "#3B82F6" : "transparent"
                border.width: 1
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_arrow.svg"; width: 18; height: 18 }
                MouseArea { id: arrowArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Arrow"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Arrow")); onPressed: stickyWin.setActiveTool("arrow") }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: (stickyWin.activeTool === "mosaic") ? "#EFF6FF" : (mosaicArea.containsMouse ? "#F1F5F9" : "transparent")
                border.color: (stickyWin.activeTool === "mosaic") ? "#3B82F6" : "transparent"
                border.width: 1
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_mosaic.svg"; width: 18; height: 18 }
                MouseArea {
                    id: mosaicArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: stickyWin.showToolbarTip(qsTr("Mosaic"), parent)
                    onExited: stickyWin.hideToolbarTip(qsTr("Mosaic"))
                    onPressed: {
                        var img = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
                        if (img && img.width > 0 && img.height > 0)
                            stickyPaintBoard.setBackgroundImg(img)
                        stickyWin.activeTool = "mosaic"
                        if (stickyWin.activeTool === "mosaic" && !stickyPaintBoard.hasBackgroundImg()) {
                            Qt.callLater(function() {
                                var retryImg = O_StickyViewModel.getImageByUrl(stickyWin.imageUrl)
                                if (retryImg && retryImg.width > 0 && retryImg.height > 0)
                                    stickyPaintBoard.setBackgroundImg(retryImg)
                                if (!stickyPaintBoard.hasBackgroundImg())
                                    stickyWin.syncPaintboardBackgroundFromDisplay()
                            })
                        }
                    }
                }
            }

            Rectangle { width: 1; height: 18; color: "#E2E8F0"; anchors.verticalCenter: parent.verticalCenter }

            Rectangle { width: 30; height: 30; radius: 7; color: zoomOutArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "-"; color: "#334155"; font.pixelSize: 16 } MouseArea { id: zoomOutArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Zoom out"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Zoom out")); onPressed: setZoomValue(stickyWin.zoomFactor - 0.1) } }
            Rectangle { width: 40; height: 30; radius: 7; color: "transparent"; Text { anchors.centerIn: parent; text: Math.round(stickyWin.zoomFactor * 100) + "%"; color: "#64748B"; font.pixelSize: 11 } }
            Rectangle { width: 30; height: 30; radius: 7; color: zoomInArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "+"; color: "#334155"; font.pixelSize: 16 } MouseArea { id: zoomInArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Zoom in"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Zoom in")); onPressed: setZoomValue(stickyWin.zoomFactor + 0.1) } }
            Rectangle { width: 30; height: 30; radius: 7; color: rotateArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "↻"; color: "#334155"; font.pixelSize: 14 } MouseArea { id: rotateArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Rotate 90°"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Rotate 90°")); onPressed: rotateCurrent(90) } }
            Rectangle { width: 30; height: 30; radius: 7; color: mirrorArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "⇋"; color: "#334155"; font.pixelSize: 14 } MouseArea { id: mirrorArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Mirror"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Mirror")); onPressed: mirrorCurrent() } }
            Rectangle { width: 30; height: 30; radius: 7; color: resetSizeArea.containsMouse ? "#F1F5F9" : "transparent"; Text { anchors.centerIn: parent; text: "1:1"; color: "#334155"; font.pixelSize: 10 } MouseArea { id: resetSizeArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Reset size"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Reset size")); onPressed: resetDisplaySize() } }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: overwriteArea.containsMouse ? "#E0F2FE" : "transparent"
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_check.svg"; width: 18; height: 18 }
                MouseArea { id: overwriteArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Done and copy"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Done and copy")); onPressed: completeEditsAndCopy() }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: undoArea.containsMouse ? "#EFF6FF" : "transparent"
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_undo.svg"; width: 18; height: 18 }
                MouseArea { id: undoArea; anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor; onEntered: stickyWin.showToolbarTip(qsTr("Undo"), parent); onExited: stickyWin.hideToolbarTip(qsTr("Undo")); onPressed: stickyPaintBoard.undo() }
            }

            Rectangle {
                width: 30; height: 30; radius: 7
                color: clearToolArea.containsMouse ? "#FEE2E2" : "transparent"
                Image { anchors.centerIn: parent; source: "qrc:/resource/img/lc_x.svg"; width: 18; height: 18 }
                MouseArea {
                    id: clearToolArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onEntered: stickyWin.showToolbarTip(qsTr("Hide toolbar"), parent)
                    onExited: stickyWin.hideToolbarTip(qsTr("Hide toolbar"))
                    onPressed: {
                        stickyWin.activeTool = ""
                        stickyWin.toolbarVisible = false
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

    Platform.Menu {
        id: contextMenu
        onAboutToShow: stickyWin.contextMenuOpen = true
        onAboutToHide: stickyWin.contextMenuOpen = false

        Platform.MenuItem { text: qsTr("当前透明度 %1%").arg(Math.round(stickyWin.imgOpacity * 100)); enabled: false }
        Platform.MenuSeparator {}

        Platform.MenuItem { visible: menuPrefs.pinSaveMerged; text: qsTr("覆盖贴图(合并标注)"); onTriggered: overwriteWithEdits() }
        Platform.MenuItem { visible: menuPrefs.pinCopy; text: qsTr("复制贴图"); onTriggered: O_StickyViewModel.copyImageToClipboard(imageUrl) }
        Platform.MenuItem { visible: menuPrefs.pinAi; text: qsTr("AI 编辑"); onTriggered: openChatPanel() }
        Platform.MenuItem {
            visible: menuPrefs.pinToolbar
            text: stickyWin.toolbarVisible ? qsTr("隐藏工具栏") : qsTr("显示工具栏")
            onTriggered: {
                stickyWin.toolbarVisible = !stickyWin.toolbarVisible
                if (!stickyWin.toolbarVisible)
                    stickyWin.activeTool = ""
            }
        }
        Platform.MenuItem { visible: menuPrefs.pinClose; text: qsTr("关闭窗口"); onTriggered: stickyWin.close() }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("透明度 +5%"); onTriggered: setOpacityValue(stickyWin.imgOpacity + 0.05) }
        Platform.MenuItem { text: qsTr("透明度 -5%"); onTriggered: setOpacityValue(stickyWin.imgOpacity - 0.05) }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("缩小"); onTriggered: setZoomValue(stickyWin.zoomFactor - 0.1) }
        Platform.MenuItem { text: qsTr("放大"); onTriggered: setZoomValue(stickyWin.zoomFactor + 0.1) }
        Platform.MenuItem { text: qsTr("旋转 90°"); onTriggered: rotateCurrent(90) }
        Platform.MenuItem { text: qsTr("镜像"); onTriggered: mirrorCurrent() }
        Platform.MenuItem { text: qsTr("恢复原始大小"); onTriggered: resetDisplaySize() }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: qsTr("重置位置")
            onTriggered: {
                stickyWin.x = (imgRect.width >= 0 ? imgRect.x : imgRect.x + imgRect.width) - shadowRadius
                stickyWin.y = (imgRect.height >= 0 ? imgRect.y : imgRect.y + imgRect.height) - shadowRadius
            }
        }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: stickyWin.toolbarVisible ? qsTr("隐藏工具栏") : qsTr("显示工具栏")
            onTriggered: {
                stickyWin.toolbarVisible = !stickyWin.toolbarVisible
                if (!stickyWin.toolbarVisible)
                    stickyWin.activeTool = ""
            }
        }

        Platform.MenuItem { text: qsTr("AI 编辑"); onTriggered: openChatPanel() }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("保存原图"); onTriggered: saveDialog.open() }
        Platform.MenuItem { text: qsTr("另存为(合并标注)"); onTriggered: mergedSaveDialog.open() }
        Platform.MenuItem { text: qsTr("覆盖贴图(合并标注)"); onTriggered: overwriteWithEdits() }
        Platform.MenuItem { text: qsTr("复制贴图"); onTriggered: O_StickyViewModel.copyImageToClipboard(imageUrl) }

        Platform.MenuSeparator {}

        Platform.Menu {
            title: qsTr("常用项置顶")
            Platform.MenuItem { text: qsTr("覆盖贴图"); checkable: true; checked: menuPrefs.pinSaveMerged; onTriggered: menuPrefs.pinSaveMerged = !menuPrefs.pinSaveMerged }
            Platform.MenuItem { text: qsTr("复制贴图"); checkable: true; checked: menuPrefs.pinCopy; onTriggered: menuPrefs.pinCopy = !menuPrefs.pinCopy }
            Platform.MenuItem { text: qsTr("AI 编辑"); checkable: true; checked: menuPrefs.pinAi; onTriggered: menuPrefs.pinAi = !menuPrefs.pinAi }
            Platform.MenuItem { text: qsTr("显示/隐藏工具栏"); checkable: true; checked: menuPrefs.pinToolbar; onTriggered: menuPrefs.pinToolbar = !menuPrefs.pinToolbar }
            Platform.MenuItem { text: qsTr("关闭窗口"); checkable: true; checked: menuPrefs.pinClose; onTriggered: menuPrefs.pinClose = !menuPrefs.pinClose }
        }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("关闭窗口"); onTriggered: stickyWin.close() }
    }

    Platform.FileDialog {
        id: saveDialog
        title: qsTr("选择保存位置")
        folder: O_ImageSaver.savePath
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: O_StickyViewModel.saveImage(imageUrl, saveDialog.file)
    }

    Platform.FileDialog {
        id: mergedSaveDialog
        title: qsTr("另存为(合并标注)")
        folder: O_ImageSaver.savePath
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: saveMergedTo(mergedSaveDialog.file)
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
                contextMenu.open()
            }
        }

        onWheel: function(wheel) {
            if (wheel.modifiers & Qt.ControlModifier) {
                if (wheel.angleDelta.y > 0)
                    setZoomValue(stickyWin.zoomFactor + 0.1)
                else if (wheel.angleDelta.y < 0)
                    setZoomValue(stickyWin.zoomFactor - 0.1)
            } else {
                if (wheel.angleDelta.y > 0)
                    setOpacityValue(stickyWin.imgOpacity + 0.05)
                else if (wheel.angleDelta.y < 0)
                    setOpacityValue(stickyWin.imgOpacity - 0.05)
            }
            wheel.accepted = true
        }
    }
}

