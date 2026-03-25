import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Dialogs
import CustomComponents 1.0

Window {
    id: mainWin
    color: "transparent";

    // Virtual desktop offset injected by main.qml
    property int screenVirtualX: 0
    property int screenVirtualY: 0

    property alias initialScreenshotArea: selectionOverlay.initialArea
    property alias screenshotArea: selectionOverlay.area
    property alias offset: selectionOverlay.dragOffset
    property alias dragMode: selectionOverlay.dragMode
    property string screenShotUrl: ""

    property rect highlightRect: Qt.rect(0, 0, 0, 0)
    property point pendingTextPoint: Qt.point(0, 0)
    property string pendingTextValue: ""
    property bool captureUiReady: false

    property bool gifRecordMode: false
    readonly property bool gifAreaLocked: O_GifRecordVM.isRecording || O_GifRecordVM.isEncoding

    property alias tool:      tool
    property alias paintBoard: paintBoard
    property alias magnifier: magnifier
    property alias gifBarObj: gifBar

    function hasSelectionArea() {
        return selectionOverlay.hasArea()
    }

    function beginHotkeyGesture(shortcutSeq) {
        hotkeyGesture.beginGesture(shortcutSeq)
    }

    function clearHotkeyGesture() {
        hotkeyGesture.clearGesture()
    }

    function maybeArmHotkeyGesture() {
        if (!hotkeyGesture.enabled || !hasSelectionArea())
            return
        hotkeyGesture.armGesture()
    }

    function confirmSelectionToClipboard() {
        root.visibility = Window.Hidden
        Qt.callLater(function() {
            var success = O_ScreenCapture.captureRectToClipboard(paintBoard, screenshotArea)
            resetArea()
            if (success)
                trayHelper.showMessage(qsTr("Capture done"), qsTr("Copied to clipboard"))
            else
                trayHelper.showMessage(qsTr("Capture failed"), qsTr("Failed to copy image"))
        })
    }

    function handleEscAction() {
        if (textEditor.visible) {
            hideTextEditor()
            return true
        }

        if (windowFactory.closeTopOcrWindow()) {
            return true
        }

        if (gifRecordMode) {
            O_GifRecordVM.cancelRecording()
            gifBar.visible = false
            gifRecordMode = false
            if (hasSelectionArea())
                tool.visible = true
            return true
        }

        if (tool.activeTool !== "") {
            tool.activeTool = ""
            return true
        }

        if (tool.markToolInfoVisible) {
            tool.markToolInfoVisible = false
            return true
        }

        if (tool.visible) {
            tool.visible = false
            return true
        }

        if (hasSelectionArea()) {
            resetArea()
            return true
        }

        return false
    }

    function beginSelectionInteraction(ax, ay) {
        if (screenshotArea.width !== 0 || screenshotArea.height !== 0) {
            if (selectionOverlay.containsCurrentPoint(ax, ay)) {
                selectionOverlay.beginMove(ax, ay)
                return
            }

            paintBoard.reset()
            tool.activeTool = ""
            tool.markToolInfoVisible = false
            tool.visible = false
            resizeHandleGroup.visible = false
            hideTextEditor()
            var winRect = O_ScreenCapture.windowAtPoint(ax, ay)
            highlightRect = winRect.width > 0
                ? Qt.rect(winRect.x, winRect.y, winRect.width, winRect.height)
                : Qt.rect(0, 0, 0, 0)
            selectionOverlay.beginSelection(ax, ay)
            return
        }

        paintBoard.reset()
        tool.activeTool = ""
        selectionOverlay.beginSelection(ax, ay)
    }

    function updateSelectionInteraction(ax, ay, isPressed, modifiers) {
        if (isPressed)
            resizeHandleGroup.visible = false
        var c = O_ScreenCapture.getPixelColor(ax, ay)
        if (c !== undefined && c !== null)
            magnifier.magnifierPixelColor = c

        if (!isPressed && screenshotArea.width === 0 && screenshotArea.height === 0) {
            var winRect = O_ScreenCapture.windowAtPoint(ax, ay)
            highlightRect = winRect.width > 0
                ? Qt.rect(winRect.x, winRect.y, winRect.width, winRect.height)
                : Qt.rect(0, 0, 0, 0)
            return
        }
        if (!isPressed) {
            return
        }

        if (dragMode === "move") {
            selectionOverlay.updateMove(ax, ay)
            return
        }

        if (dragMode === "select") {
            selectionOverlay.updateSelection(ax, ay, modifiers)
            if (tool.visible) tool.visible = false
        }
    }

    function finalizeSelectionInteraction() {
        selectionOverlay.endInteraction()
        if (Math.abs(screenshotArea.width) <= 5 && Math.abs(screenshotArea.height) <= 5) {
            if (highlightRect.width > 0 && highlightRect.height > 0) {
                screenshotArea        = highlightRect
                console.log("screenshotArea2-->",screenshotArea)
                initialScreenshotArea = highlightRect
                resizeHandleGroup.area        = localScreenshotArea
                resizeHandleGroup.initialArea = toLocalRect(initialScreenshotArea)
                highlightRect = Qt.rect(0, 0, 0, 0)
                Qt.callLater(function() { tool.visible = true })
            }
            resizeHandleGroup.visible = (highlightRect.width > 0 && highlightRect.height > 0)
            return
        }
        selectionOverlay.normalizeCurrentArea()
        resizeHandleGroup.initialArea = toLocalRect(initialScreenshotArea)
        if (Math.abs(screenshotArea.width) > 2 && Math.abs(screenshotArea.height) > 2) {
            Qt.callLater(function() { tool.visible = true })
        }
        resizeHandleGroup.area = localScreenshotArea
        resizeHandleGroup.visible = true;

    }
    function toAbsolutePoint(localX, localY) {
        return screenshotRect.mapToGlobal(localX, localY)
    }
    function toLocalRect(absRect) {
        var topLeft = screenshotRect.mapFromGlobal(absRect.x, absRect.y)
        return Qt.rect(topLeft.x, topLeft.y, absRect.width, absRect.height)
    }
    function toAbsoluteRect(localRect) {
        var topLeft = screenshotRect.mapToGlobal(localRect.x, localRect.y)
        return Qt.rect(topLeft.x, topLeft.y, localRect.width, localRect.height)
    }
    function syncResizeHandles() {
        resizeHandleGroup.area = localScreenshotArea
        resizeHandleGroup.initialArea = toLocalRect(initialScreenshotArea)
    }
    function syncGifResizeHandles() {
        gifResizeHandleGroup.area = localScreenshotArea
        gifResizeHandleGroup.initialArea = toLocalRect(initialScreenshotArea)
    }
    function normalizeRect(rectValue) {
        var left = Math.min(rectValue.x, rectValue.x + rectValue.width)
        var right = Math.max(rectValue.x, rectValue.x + rectValue.width)
        var top = Math.min(rectValue.y, rectValue.y + rectValue.height)
        var bottom = Math.max(rectValue.y, rectValue.y + rectValue.height)
        return Qt.rect(left, top, right - left, bottom - top)
    }

    // screenshotArea 在本窗口本地坐标中的等价矩形（用于绘制）
    readonly property rect localScreenshotArea: toLocalRect(screenshotArea)
    readonly property rect localNormalizedScreenshotArea: {
        return normalizeRect(localScreenshotArea)
    }
    readonly property rect normalizedScreenshotArea: {
        return selectionOverlay.normalizedArea
    }
    readonly property rect physicalScreenshotArea: {
        if (screenshotArea.width === 0 || screenshotArea.height === 0)
            return Qt.rect(0, 0, 0, 0)
        return O_ScreenCapture.mapLogicalRectToPhysicalRect(normalizedScreenshotArea)
    }
    readonly property rect localHighlightRect: Qt.rect(
        highlightRect.x - screenVirtualX,
        highlightRect.y - screenVirtualY,
        highlightRect.width,
        highlightRect.height
    )

    function createStickyWindow(imageUrl, imgRect) {
        console.log("imgRect-->",imgRect)
        return windowFactory.createStickyWindow(imageUrl, imgRect)
    }

    function createOcrResultWindow() {
        return windowFactory.createOcrResultWindow()
    }

    function openTextEditor(localX, localY) {
        pendingTextPoint = Qt.point(localX, localY)
        pendingTextValue = markToolInfo.annotationText
        textEditor.x = Math.max(0, Math.min(localX, mainWin.width - textEditor.width))
        textEditor.y = Math.max(0, Math.min(localY + 8, mainWin.height - textEditor.height))
        textEditor.beginEdit(pendingTextValue)
    }

    function hideTextEditor() {
        textEditor.visible = false
    }

    function submitTextEditor(textValue) {
        var text = textValue.trim()
        if (text.length === 0) {
            hideTextEditor()
            return
        }
        markToolInfo.annotationText = text
        paintBoard.addTextAnnotation(Math.round(pendingTextPoint.x), Math.round(pendingTextPoint.y), text)
        hideTextEditor()
    }

    function resetTextClickTracking() {
        inputController.resetTextClickTracking()
    }

    function beginGifInteraction(ax, ay) {
        if (!gifRecordMode || gifAreaLocked)
            return false
        if (selectionOverlay.containsCurrentPoint(ax, ay)) {
            offset.x = ax
            offset.y = ay
            initialScreenshotArea = screenshotArea
        }
        return true
    }

    function beginSpaceMoveInteraction(ax, ay) {
        if (!inputController.spaceHeld || !hasSelectionArea())
            return false
        offset.x = ax
        offset.y = ay
        initialScreenshotArea = screenshotArea
        return true
    }

    function beginTextToolInteraction(ax, ay, localX, localY) {
        if (tool.activeTool !== "text"
                || screenshotArea.width === 0
                || screenshotArea.height === 0
                || !selectionOverlay.containsCurrentPoint(ax, ay)) {
            return false
        }

        inputController.beginTextClick(ax, ay, localX, localY)
        offset.x = ax
        offset.y = ay
        initialScreenshotArea = screenshotArea
        return true
    }

    function updatePendingTextInteraction(ax, ay) {
        var stillPending = inputController.updatePendingTextClick(ax, ay, 3)
        if (!stillPending && inputController.textPressMoved && textEditor.visible)
            hideTextEditor()
        return stillPending
    }

    function updateGifInteraction(ax, ay, isPressed) {
        if (!gifRecordMode)
            return false
        if (gifAreaLocked)
            return true
        if (isPressed && selectionOverlay.containsCurrentPoint(ax, ay)) {
            screenshotArea = inputController.translatedRect(initialScreenshotArea, offset, ax, ay)
            O_GifRecordVM.updateCaptureRect(
                screenshotArea.x,
                screenshotArea.y,
                screenshotArea.width,
                screenshotArea.height
            )
        }
        return true
    }

    function updateSpaceMoveInteraction(ax, ay, isPressed) {
        if (!isPressed || !inputController.spaceHeld || !hasSelectionArea())
            return false

        screenshotArea = inputController.translatedRect(initialScreenshotArea, offset, ax, ay)
        return true
    }

    function updateMagnifierFromMouse(localX, localY, ax, ay) {
        magnifier.magnifierMouseX = localX
        magnifier.magnifierMouseY = localY
        magnifier.magnifierAbsX = ax
        magnifier.magnifierAbsY = ay
    }

    function finalizeTextToolInteraction() {
        if (inputController.shouldOpenTextEditor()) {
            openTextEditor(inputController.textClickLocalPoint.x,
                           inputController.textClickLocalPoint.y)
            resetTextClickTracking()
            return true
        }

        resetTextClickTracking()
        return false
    }

    function finalizeGifInteraction() {
        if (!gifRecordMode)
            return false
        if (gifAreaLocked)
            return true

        initialScreenshotArea = screenshotArea
        gifResizeHandleGroup.initialArea = toLocalRect(initialScreenshotArea)
        return true
    }

    function resetArea() {
        selectionOverlay.reset()
        syncResizeHandles()
        syncGifResizeHandles()
        screenShotUrl = ""
        paintBoard.reset()
        tool.visible = false
        tool.activeTool = ""
        tool.markToolInfoVisible = false
        hideTextEditor()
        gifRecordMode = false
        gifBar.visible = false
        highlightRect = Qt.rect(0, 0, 0, 0)
        resetTextClickTracking()
        clearHotkeyGesture()
        O_ScreenCapture.releaseDesktopSnapshot()
        root.visibility = Window.Hidden;
    }

    TZSelectionOverlay {
        id: selectionOverlay
        coordinateItem: screenshotRect
    }

    TZWindowFactory {
        id: windowFactory
    }

    TZSelectionPositioner {
        id: selectionPositioner
    }

    TZCaptureInputController {
        id: inputController
    }

    TZHotkeyGestureController {
        id: hotkeyGesture
        onLongPressTriggered: {
            if (hasSelectionArea())
                tool.visible = true
        }
    }

    FileDialog {
        id: saveDialog
        title: qsTr("Select save location")
        currentFolder: O_ImageSaver.savePath
        fileMode: FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: {
            var success = O_ImageSaver.saveImage("data:image/png;base64,"+screenShotUrl, saveDialog.selectedFile)
            if (success) {
                console.log("Save success:", saveDialog.currentFolder)
                trayHelper.showMessage(qsTr("Save success"), qsTr("Screenshot saved to file"))
            } else {
                console.log("Save failed")
                trayHelper.showMessage(qsTr("Save failed"), qsTr("Failed to save screenshot file"))
            }
            resetArea()
        }
    }

    TZGifRecordBar {
        id: gifBar
        width: 260
        height: 44
        visible: false
        z: 20
        captureRect: screenshotArea
        x: selectionPositioner.alignRight(localNormalizedScreenshotArea, width, mainWin.width)
        y: selectionPositioner.placeBelowOrAbove(localNormalizedScreenshotArea, height, mainWin.height)
        onSignalCancel: {
            gifBar.visible = false
            gifRecordMode = false
            tool.visible = true
        }
        onSignalRecordFinished: {
            resetArea()
        }
    }

    Rectangle {
        id: recordingBorder
        visible: O_GifRecordVM.isRecording
        // Draw border outside capture rect so it won't be included in recorded frames.
        x: localNormalizedScreenshotArea.x - 2
        y: localNormalizedScreenshotArea.y - 2
        width:  localNormalizedScreenshotArea.width + 4
        height: localNormalizedScreenshotArea.height + 4
        color: "transparent"
        border.color: "#EF4444"
        border.width: 2
        z: 10
    }

    PaintBoard {
        id: paintBoard
        anchors.fill: parent
        z: 1
        enabled: tool.activeTool !== "" && tool.activeTool !== "text"
        visible: captureUiReady && screenshotArea.width !== 0 && screenshotArea.height !== 0
        shapeType: tool.shapeType
        penColor:  markToolInfo.currentColor
        penSize:   markToolInfo.currentSize
        annotationText: markToolInfo.annotationText
        textBackgroundEnabled: markToolInfo.textBackgroundEnabled
        mosaicBlurLevel: markToolInfo.mosaicBlurLevel
        numberAutoIncrement: markToolInfo.numberAutoIncrement
        numberValue: markToolInfo.numberValue
    }

    Connections {
        target: paintBoard
        function onNumberValueChanged(value) {
            markToolInfo.numberValue = value
        }
    }

    Rectangle {
        id: screenshotRect
        anchors.fill: parent
        color: "transparent";
        focus: true
        z: 2
        visible: captureUiReady && !gifRecordMode

        TZRectMaskOverlay {
            anchors.fill: parent
            holeRect: localNormalizedScreenshotArea
            maskColor: "#99000000"
        }

        TZSelectionInfoLabel {
            visible: screenshotArea.width !== 0 && screenshotArea.height !== 0
            x: selectionPositioner.alignLeft(localNormalizedScreenshotArea, width, mainWin.width)
            y: selectionPositioner.placeAbove(localNormalizedScreenshotArea, height, mainWin.height, 5)
            logicalRect: normalizedScreenshotArea
            physicalRect: physicalScreenshotArea
        }

        TZTool {
            id: tool
            width: 656; height: 44
            z: 30

            // Toolbar position (local coordinates)
            x: selectionPositioner.alignRight(localNormalizedScreenshotArea, width, mainWin.width)
            y: selectionPositioner.placeBelowOrAbove(localNormalizedScreenshotArea, height, mainWin.height)

            onActiveToolChanged: {
                if (activeTool !== "text" && textEditor.visible) {
                    hideTextEditor()
                }

                if (activeTool === "mosaic" && !paintBoard.hasBackgroundImg()) {
                    Qt.callLater(function() {
                        O_ScreenCapture.grabToPaintBoard(
                            paintBoard,
                            Qt.rect(screenVirtualX, screenVirtualY, mainWin.width, mainWin.height)
                        )
                    })
                }
            }

            onSignalScreenshot: {
                confirmSelectionToClipboard()
            }

            onSignalCancel: { resetArea() }

            onSignalSave: {
                root.visibility = Window.Hidden
                Qt.callLater(function() {
                    screenShotUrl = O_ScreenCapture.captureRectToBase64(paintBoard, screenshotArea)
                    root.visibility = Window.Windowed
                    saveDialog.open()
                })
            }

            onSignalSticky: {
                root.visibility = Window.Hidden
                Qt.callLater(function() {
                    var captureRect = normalizedScreenshotArea
                    var url = O_ScreenCapture.captureRectToStickyUrl(paintBoard, captureRect)
                    console.log("[Sticky] imageUrl:", url)
                    O_StickyViewModel.requestSticky(url, captureRect)
                    resetArea()
                })
            }

            onSignalUndo: { paintBoard.undo() }

            onSignalGifRecord: {
                gifRecordMode = true
                tool.visible = false
                gifBar.visible = true
                Qt.callLater(function() {
                    O_GifRecordVM.startRecording(
                        screenshotArea.x, screenshotArea.y,
                        screenshotArea.width, screenshotArea.height
                    )
                })
            }

            onSignalLongScreenshot: {
                var captureRect = screenshotArea
                root.beginLongCapture(captureRect)
            }

            onSignalOcr: {
                var captureRect = screenshotArea
                Qt.callLater(function() {
                    var img = O_ScreenCapture.captureRectToImage(captureRect)
                    O_OcrVM.recognize(img)
                })
            }
        }

        TZMarkToolInfo {
            id: markToolInfo
            visible: tool.visible && tool.markToolInfoVisible
            z: 30
            x: tool.x
            y: tool.y + tool.height
            toolName: tool.activeTool
            showColorRow: tool.activeTool !== "mosaic"
        }

        TZTextAnnotationEditor {
            id: textEditor
            z: 40
            onAccepted: function(text) { submitTextEditor(text) }
            onCanceled: hideTextEditor()
        }

        TZResizeHandleGroup {
            id: resizeHandleGroup
            coordinateItem: screenshotRect
            // Handle control points use local coordinates
            area:        localScreenshotArea
            initialArea: toLocalRect(initialScreenshotArea)
            z: 2
            onAreaChanged: {
                // 转回绝对坐标
                screenshotArea = toAbsoluteRect(area)
            }
            onInitialAreaChanged: {
                initialScreenshotArea = toAbsoluteRect(initialArea)
            }
        }
    }

    TZResizeHandleGroup {
        id: gifResizeHandleGroup
        coordinateItem: screenshotRect
        area: localScreenshotArea
        initialArea: toLocalRect(initialScreenshotArea)
        visible: gifRecordMode && !gifAreaLocked
        enabled: !gifAreaLocked
        z: 15
        onAreaChanged: {
            screenshotArea = toAbsoluteRect(area)
            O_GifRecordVM.updateCaptureRect(
                screenshotArea.x, screenshotArea.y,
                screenshotArea.width, screenshotArea.height
            )
        }
        onInitialAreaChanged: {
            initialScreenshotArea = toAbsoluteRect(initialArea)
        }
    }

    // Window highlight rectangle (local coordinates)
    TZWindowHighlightOverlay {
        id: windowHighlight
        visible: captureUiReady && screenshotArea.width === 0 && screenshotArea.height === 0
                 && highlightRect.width > 0 && highlightRect.height > 0
        x: localHighlightRect.x
        y: localHighlightRect.y
        width:  localHighlightRect.width
        height: localHighlightRect.height
        z: 5
    }

    TZMagnifyingGlass {
        id: magnifier
        width:  130
        height: 170
        z: 3
        color: "transparent"
        // Hide magnifier in GIF mode to avoid recording it
        visible: captureUiReady && mainWin.visible && !tool.visible && !gifRecordMode
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        enabled: captureUiReady
        hoverEnabled: true
        focus: true
        cursorShape: gifRecordMode
                     ? (gifAreaLocked ? Qt.ArrowCursor : Qt.SizeAllCursor)
                     : Qt.CrossCursor

        onPressed: function(mouse) {
            var globalPos = screenshotRect.mapToGlobal(mouseX, mouseY)

            var ax = globalPos.x, ay = globalPos.y
            resetTextClickTracking()

            if (beginGifInteraction(ax, ay)) {
                return
            }

            if (beginSpaceMoveInteraction(ax, ay)) {
                return
            }

            if (beginTextToolInteraction(ax, ay, mouseX, mouseY)) {
                return
            }

            if (textEditor.visible) {
                hideTextEditor()
            }
            beginSelectionInteraction(ax, ay)
        }

        onPositionChanged: function(mouse) {
            var globalPos = screenshotRect.mapToGlobal(mouseX, mouseY)
            var ax = globalPos.x, ay = globalPos.y

            if (pressed && updatePendingTextInteraction(ax, ay)) {
                return
            }

            if (updateGifInteraction(ax, ay, pressed)) {
                return
            }

            if (updateSpaceMoveInteraction(ax, ay, pressed)) {
                return
            }

            updateMagnifierFromMouse(mouseX, mouseY, ax, ay)
            updateSelectionInteraction(ax, ay, pressed, mouse.modifiers)
        }

        onReleased: function(mouse) {
            if (finalizeTextToolInteraction()) {
                return
            }

            if (finalizeGifInteraction()) {
                return
            }

            finalizeSelectionInteraction()
        }

        onWheel: function(wheel) { wheel.accepted = true }

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Space) {
                inputController.spaceHeld = true
                event.accepted = true
                return
            }
            if (event.key === Qt.Key_Escape) {
                if (handleEscAction())
                    event.accepted = true
                return
            }
        }

        Keys.onReleased: function(event) {
            if (event.key === Qt.Key_Space) {
                inputController.spaceHeld = false
                event.accepted = true
                return
            }

            if (hotkeyGesture.handleKeyRelease(event.key)) {
                if (!hotkeyGesture.longPressed && hasSelectionArea())
                    confirmSelectionToClipboard()
                clearHotkeyGesture()
                event.accepted = true
            }
        }
    }
    onVisibilityChanged:
    {
        if(root.visibility === Window.Hidden) {
            resizeHandleGroup.visible = false;
            gifResizeHandleGroup.visible = false;
        }
    }

}
