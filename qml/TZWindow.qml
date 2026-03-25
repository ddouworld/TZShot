import QtQuick
import QtQuick.Window
import QtQuick.Shapes
import QtQuick.Layouts
import QtQuick.Dialogs
import CustomComponents 1.0

Window {
    id: mainWin
    color: "transparent";

    // Virtual desktop offset injected by main.qml
    property int screenVirtualX: 0
    property int screenVirtualY: 0

    property rect initialScreenshotArea: Qt.rect(0, 0, 0, 0)
    property rect screenshotArea: Qt.rect(0, 0, 0, 0)
    property var  offset: Qt.point(0, 0)
    property string screenShotUrl: ""

    property rect highlightRect: Qt.rect(0, 0, 0, 0)
    property point pendingTextPoint: Qt.point(0, 0)
    property string pendingTextValue: ""
    property bool captureUiReady: false

    property var stickyWinObj: null
    property var stickyWinList: []
    property var stickyComponent: null
    property bool gifRecordMode: false
    readonly property bool gifAreaLocked: O_GifRecordVM.isRecording || O_GifRecordVM.isEncoding
    property var ocrComponent: null
    property var ocrWinList: []
    property bool spaceHeld: false
    property bool hotkeyGestureEnabled: false
    property bool hotkeyGestureArmed: false
    property bool hotkeyLongPressed: false
    property string hotkeyMainToken: ""
    property bool textClickPending: false
    property bool textPressMoved: false
    property point textClickAbsPoint: Qt.point(0, 0)
    property point textClickLocalPoint: Qt.point(0, 0)
    property string dragMode: "none" // none | select | move

    property alias tool:      tool
    property alias paintBoard: paintBoard
    property alias magnifier: magnifier
    property alias gifBarObj: gifBar

    function hasSelectionArea() {
        return Math.abs(screenshotArea.width) > 2 && Math.abs(screenshotArea.height) > 2
    }

    function parseMainKeyToken(shortcutSeq) {
        if (!shortcutSeq || shortcutSeq.length === 0)
            return ""
        var parts = shortcutSeq.split("+")
        if (parts.length === 0)
            return ""
        return parts[parts.length - 1].trim().toUpperCase()
    }

    function keyMatchesToken(key, token) {
        if (!token || token.length === 0)
            return false
        if (token.length === 1) {
            if (token >= "A" && token <= "Z")
                return key === (Qt.Key_A + (token.charCodeAt(0) - "A".charCodeAt(0)))
            if (token >= "0" && token <= "9")
                return key === (Qt.Key_0 + (token.charCodeAt(0) - "0".charCodeAt(0)))
        }
        if (token[0] === "F") {
            var fn = Number(token.slice(1))
            if (fn >= 1 && fn <= 24)
                return key === (Qt.Key_F1 + fn - 1)
        }
        if (token === "SPACE")
            return key === Qt.Key_Space
        if (token === "ESC" || token === "ESCAPE")
            return key === Qt.Key_Escape
        if (token === "ENTER" || token === "RETURN")
            return key === Qt.Key_Return || key === Qt.Key_Enter
        return false
    }

    function beginHotkeyGesture(shortcutSeq) {
        hotkeyGestureEnabled = true
        hotkeyGestureArmed = false
        hotkeyLongPressed = false
        hotkeyMainToken = parseMainKeyToken(shortcutSeq)
        hotkeyLongPressTimer.stop()
    }

    function clearHotkeyGesture() {
        hotkeyGestureEnabled = false
        hotkeyGestureArmed = false
        hotkeyLongPressed = false
        hotkeyMainToken = ""
        hotkeyLongPressTimer.stop()
    }

    function maybeArmHotkeyGesture() {
        if (!hotkeyGestureEnabled || !hasSelectionArea())
            return
        hotkeyGestureArmed = true
        hotkeyLongPressed = false
        hotkeyLongPressTimer.restart()
    }

    function selectionRectFromDrag(startX, startY, endX, endY, withShift, withAlt) {
        var x = startX
        var y = startY
        var w = endX - startX
        var h = endY - startY

        if (withAlt) {
            x = startX - (endX - startX)
            y = startY - (endY - startY)
            w = (endX - startX) * 2
            h = (endY - startY) * 2
        }

        if (withShift) {
            var absW = Math.abs(w)
            var absH = Math.abs(h)
            var side = Math.min(absW, absH)
            w = w < 0 ? -side : side
            h = h < 0 ? -side : side
        }

        return Qt.rect(x, y, w, h)
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

        if (ocrWinList.length > 0) {
            var topWin = ocrWinList[ocrWinList.length - 1]
            if (topWin)
                topWin.close()
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

    function handleMousePressed(ax, ay) {
        if (screenshotArea.width !== 0 || screenshotArea.height !== 0) {
            if (isPointInRect(screenshotArea, ax, ay)) {
                dragMode = "move"
                offset.x = ax
                offset.y = ay
                initialScreenshotArea = screenshotArea
            } else {
                dragMode = "select"
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
                initialScreenshotArea = Qt.rect(ax, ay, 0, 0)
                screenshotArea        = Qt.rect(ax, ay, 0, 0)
            }
        } else {
            dragMode = "select"
            paintBoard.reset()
            tool.activeTool = ""
            initialScreenshotArea = Qt.rect(ax, ay, 0, 0)
            screenshotArea        = Qt.rect(ax, ay, 0, 0)
        }
    }

    function handleMouseMoved(ax, ay, isPressed, modifiers) {
        if(isPressed)
            resizeHandleGroup.visible = false;
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
            let dx = ax - offset.x
            let dy = ay - offset.y
            screenshotArea = Qt.rect(
                initialScreenshotArea.x + dx,
                initialScreenshotArea.y + dy,
                initialScreenshotArea.width, initialScreenshotArea.height)
            return
        }

        if (dragMode === "select") {
            var useShift = (modifiers & Qt.ShiftModifier) !== 0
            var useAlt = (modifiers & Qt.AltModifier) !== 0
            screenshotArea = selectionRectFromDrag(
                initialScreenshotArea.x, initialScreenshotArea.y, ax, ay, useShift, useAlt)
            if (tool.visible) tool.visible = false
        }
    }

    function handleMouseReleased() {
        dragMode = "none"
        if (Math.abs(screenshotArea.width) <= 5 && Math.abs(screenshotArea.height) <= 5) {
            if (highlightRect.width > 0 && highlightRect.height > 0) {
                screenshotArea        = highlightRect
                initialScreenshotArea = highlightRect
                resizeHandleGroup.area        = localScreenshotArea
                resizeHandleGroup.initialArea = Qt.rect(
                    initialScreenshotArea.x - screenVirtualX,
                    initialScreenshotArea.y - screenVirtualY,
                    initialScreenshotArea.width, initialScreenshotArea.height)
                highlightRect = Qt.rect(0, 0, 0, 0)
                Qt.callLater(function() { tool.visible = true })
            }
            resizeHandleGroup.visible = (highlightRect.width > 0 && highlightRect.height > 0)
            return
        }
        var bounds = getNormalizedRectBounds(screenshotArea)
        if (bounds.isValid) {
            screenshotArea = Qt.rect(bounds.left, bounds.top, bounds.width, bounds.height)
        }
        initialScreenshotArea = screenshotArea
        resizeHandleGroup.initialArea = Qt.rect(
            initialScreenshotArea.x - screenVirtualX,
            initialScreenshotArea.y - screenVirtualY,
            initialScreenshotArea.width, initialScreenshotArea.height)
        if (Math.abs(screenshotArea.width) > 2 && Math.abs(screenshotArea.height) > 2) {
            Qt.callLater(function() { tool.visible = true })
        }
        resizeHandleGroup.area = localScreenshotArea
        resizeHandleGroup.visible = true;

    }
    // 本地坐标 -> 虚拟桌面绝对坐标
    function toAbsX(localX) { return localX + screenVirtualX }
    function toAbsY(localY) { return localY + screenVirtualY }

    // screenshotArea 在本窗口本地坐标中的等价矩形（用于绘制）
    readonly property rect localScreenshotArea: Qt.rect(
        screenshotArea.x - screenVirtualX,
        screenshotArea.y - screenVirtualY,
        screenshotArea.width,
        screenshotArea.height
    )
    readonly property rect localNormalizedScreenshotArea: {
        let b = getNormalizedRectBounds(localScreenshotArea)
        return b.isValid ? Qt.rect(b.left, b.top, b.width, b.height) : Qt.rect(0, 0, 0, 0)
    }
    readonly property rect localHighlightRect: Qt.rect(
        highlightRect.x - screenVirtualX,
        highlightRect.y - screenVirtualY,
        highlightRect.width,
        highlightRect.height
    )

    function getNormalizedRectBounds(rect) {
        if (rect.width === 0 || rect.height === 0) {
            return { left: 0, right: 0, top: 0, bottom: 0, isValid: false };
        }
        let left   = Math.min(rect.x, rect.x + rect.width);
        let right  = Math.max(rect.x, rect.x + rect.width);
        let top    = Math.min(rect.y, rect.y + rect.height);
        let bottom = Math.max(rect.y, rect.y + rect.height);
        return { left: left, right: right, top: top, bottom: bottom,
                 width: right - left, height: bottom - top, isValid: true };
    }

    function isPointInRect(rect, x, y) {
        let bounds = getNormalizedRectBounds(rect);
        if (!bounds.isValid) return false;
        return (x >= bounds.left && x <= bounds.right)
            && (y >= bounds.top  && y <= bounds.bottom);
    }

    function createStickyWindow(imageUrl, imgRect) {
        if (!imageUrl || imageUrl.length === 0) {
            console.error("Failed to create sticky window: invalid image URL")
            return
        }
        if (stickyComponent === null)
            stickyComponent = Qt.createComponent("qrc:/qml/TZStickyWindow.qml");

        var newWinObj = null;
        if (stickyComponent.status === Component.Ready) {
            newWinObj = stickyComponent.createObject(null, {
                "imageUrl": imageUrl, "imgRect": imgRect });
        } else if (stickyComponent.status === Component.Error) {
            console.error("Failed to load TZStickyWindow.qml:", stickyComponent.errorString());
        }
        if (newWinObj) {
            newWinObj.visible = true;
            newWinObj.raise()
            newWinObj.requestActivate()
            var newList = stickyWinList.slice();
            newList.push(newWinObj);
            stickyWinList = newList;
            var winRef = newWinObj;
            function onWinClosing(closeEvent) {
                var idx = stickyWinList.indexOf(winRef);
                if (idx !== -1) {
                    var updatedList = stickyWinList.slice();
                    updatedList.splice(idx, 1);
                    stickyWinList = updatedList;
                }
                winRef.closing.disconnect(onWinClosing);
                winRef = null;
            }
            newWinObj.closing.connect(onWinClosing);
        } else {
            console.error("Failed to create sticky window object");
        }
    }

    function createOcrResultWindow() {
        if (ocrWinList.length > 0) {
            var existing = ocrWinList[ocrWinList.length - 1]
            if (existing) {
                existing.show()
                existing.raise()
                existing.requestActivate()
                return
            }
        }

        if (ocrComponent === null)
            ocrComponent = Qt.createComponent("qrc:/qml/TZOcrResult.qml")
        if (ocrComponent.status === Component.Ready) {
            var win = ocrComponent.createObject(null)
            if (win) {
                win.show()
                win.raise()
                win.requestActivate()
                var list = ocrWinList.slice()
                list.push(win)
                ocrWinList = list
                var winRef = win
                function onOcrClosing(closeEvent) {
                    var idx = ocrWinList.indexOf(winRef)
                    if (idx !== -1) {
                        var updatedList = ocrWinList.slice()
                        updatedList.splice(idx, 1)
                        ocrWinList = updatedList
                    }
                    winRef.closing.disconnect(onOcrClosing)
                    winRef = null
                }
                win.closing.connect(onOcrClosing)
            }
        } else if (ocrComponent.status === Component.Error) {
            console.error("Failed to load TZOcrResult.qml:", ocrComponent.errorString())
        }
    }

    function openTextEditor(localX, localY) {
        pendingTextPoint = Qt.point(localX, localY)
        pendingTextValue = markToolInfo.annotationText
        textInput.text = pendingTextValue
        textEditor.visible = true
        textEditor.x = Math.max(0, Math.min(localX, mainWin.width - textEditor.width))
        textEditor.y = Math.max(0, Math.min(localY + 8, mainWin.height - textEditor.height))
        textInput.forceActiveFocus()
        textInput.selectAll()
    }

    function hideTextEditor() {
        textEditor.visible = false
    }

    function submitTextEditor() {
        var text = textInput.text.trim()
        if (text.length === 0) {
            hideTextEditor()
            return
        }
        markToolInfo.annotationText = text
        paintBoard.addTextAnnotation(Math.round(pendingTextPoint.x), Math.round(pendingTextPoint.y), text)
        hideTextEditor()
    }

    function resetArea() {
        screenshotArea        = Qt.rect(0, 0, 0, 0)
        initialScreenshotArea = Qt.rect(0, 0, 0, 0)
        resizeHandleGroup.area        = Qt.rect(0, 0, 0, 0)
        resizeHandleGroup.initialArea = Qt.rect(0, 0, 0, 0)
        gifResizeHandleGroup.area        = Qt.rect(0, 0, 0, 0)
        gifResizeHandleGroup.initialArea = Qt.rect(0, 0, 0, 0)
        screenShotUrl = ""
        paintBoard.reset()
        tool.visible = false
        tool.activeTool = ""
        tool.markToolInfoVisible = false
        hideTextEditor()
        gifRecordMode = false
        gifBar.visible = false
        highlightRect = Qt.rect(0, 0, 0, 0)
        textClickPending = false
        textPressMoved = false
        textClickAbsPoint = Qt.point(0, 0)
        textClickLocalPoint = Qt.point(0, 0)
        dragMode = "none"
        clearHotkeyGesture()
        O_ScreenCapture.releaseDesktopSnapshot()
        root.visibility = Window.Hidden;
    }

    Timer {
        id: hotkeyLongPressTimer
        interval: 280
        repeat: false
        onTriggered: {
            if (hotkeyGestureArmed && hasSelectionArea()) {
                hotkeyLongPressed = true
                tool.visible = true
            }
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
        x: {
            let areaLeft = localNormalizedScreenshotArea.x
            let preferX  = areaLeft + localNormalizedScreenshotArea.width - width
            return Math.max(0, Math.min(preferX, mainWin.width - width))
        }
        y: {
            let areaTop = localNormalizedScreenshotArea.y
            let belowY  = areaTop + localNormalizedScreenshotArea.height + 6
            return belowY + height > mainWin.height ? areaTop - height - 6 : belowY
        }
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

        Shape {
            id: shape
            ShapePath {
                id: path
                strokeWidth: -1
                fillColor: "#99000000"
                startX: 0; startY: 0
                // 外框（整个窗口）
                PathLine { x: mainWin.width; y: 0 }
                PathLine { x: mainWin.width; y: mainWin.height }
                PathLine { x: 0;            y: mainWin.height }
                PathLine { x: 0;            y: 0 }
                // 镂空（选区，本地坐标）
                PathMove {
                    x: localNormalizedScreenshotArea.x
                    y: localNormalizedScreenshotArea.y
                }
                PathLine {
                    x: localNormalizedScreenshotArea.x + localNormalizedScreenshotArea.width
                    y: localNormalizedScreenshotArea.y
                }
                PathLine {
                    x: localNormalizedScreenshotArea.x + localNormalizedScreenshotArea.width
                    y: localNormalizedScreenshotArea.y + localNormalizedScreenshotArea.height
                }
                PathLine {
                    x: localNormalizedScreenshotArea.x
                    y: localNormalizedScreenshotArea.y + localNormalizedScreenshotArea.height
                }
                PathLine {
                    x: localNormalizedScreenshotArea.x
                    y: localNormalizedScreenshotArea.y
                }
            }
        }

        TZLable {
            width: 80; height: 20
            visible: screenshotArea.width !== 0 && screenshotArea.height !== 0
            x: localNormalizedScreenshotArea.x
            y: localNormalizedScreenshotArea.y - 25
            lableText: qsTr("%1 * %2").arg(Math.abs(screenshotArea.width)).arg(Math.abs(screenshotArea.height))
        }

        TZTool {
            id: tool
            width: 656; height: 44
            z: 30

            // Toolbar position (local coordinates)
            x: {
                let areaLeft = localNormalizedScreenshotArea.x
                let preferX  = areaLeft + localNormalizedScreenshotArea.width - width
                return Math.max(0, Math.min(preferX, mainWin.width - width))
            }
            y: {
                let areaTop = localNormalizedScreenshotArea.y
                let belowY  = areaTop + localNormalizedScreenshotArea.height + 6
                return belowY + height > mainWin.height ? areaTop - height - 6 : belowY
            }

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
                    var url = O_ScreenCapture.captureRectToStickyUrl(paintBoard, screenshotArea)
                    O_StickyViewModel.requestSticky(url, screenshotArea)
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

        Rectangle {
            id: textEditor
            visible: false
            width: 260
            height: 88
            radius: 8
            color: "#FFFFFF"
            border.color: "#CBD5E1"
            border.width: 1
            z: 40

            Column {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 8

                Rectangle {
                    width: parent.width
                    height: 34
                    radius: 6
                    color: "#F8FAFC"
                    border.color: textInput.activeFocus ? "#3B82F6" : "#E2E8F0"
                    border.width: 1

                    TextInput {
                        id: textInput
                        anchors.fill: parent
                        anchors.margins: 8
                        verticalAlignment: TextInput.AlignVCenter
                        color: "#0F172A"
                        font.pixelSize: 13
                        selectByMouse: true
                        text: pendingTextValue
                        onAccepted: submitTextEditor()
                    }
                }

                Row {
                    spacing: 8

                    Rectangle {
                        width: 72
                        height: 28
                        radius: 6
                        color: cancelInputArea.containsMouse ? "#F1F5F9" : "transparent"
                        border.color: "#CBD5E1"
                        border.width: 1

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("Cancel")
                            color: "#334155"
                            font.pixelSize: 12
                        }

                        MouseArea {
                            id: cancelInputArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onPressed: hideTextEditor()
                        }
                    }

                    Rectangle {
                        width: 72
                        height: 28
                        radius: 6
                        color: confirmInputArea.containsMouse ? "#2563EB" : "#3B82F6"

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("OK")
                            color: "#FFFFFF"
                            font.pixelSize: 12
                        }

                        MouseArea {
                            id: confirmInputArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onPressed: submitTextEditor()
                        }
                    }
                }
            }
        }

        TZResizeHandleGroup {
            id: resizeHandleGroup
            // Handle control points use local coordinates
            area:        localScreenshotArea
            initialArea: Qt.rect(
                initialScreenshotArea.x - screenVirtualX,
                initialScreenshotArea.y - screenVirtualY,
                initialScreenshotArea.width,
                initialScreenshotArea.height)
            z: 2
            onAreaChanged: {
                // 转回绝对坐标
                screenshotArea = Qt.rect(
                    area.x + screenVirtualX,
                    area.y + screenVirtualY,
                    area.width, area.height)
            }
            onInitialAreaChanged: {
                initialScreenshotArea = Qt.rect(
                    initialArea.x + screenVirtualX,
                    initialArea.y + screenVirtualY,
                    initialArea.width, initialArea.height)
            }
        }
    }

    TZResizeHandleGroup {
        id: gifResizeHandleGroup
        area: localScreenshotArea
        initialArea: Qt.rect(
            initialScreenshotArea.x - screenVirtualX,
            initialScreenshotArea.y - screenVirtualY,
            initialScreenshotArea.width,
            initialScreenshotArea.height)
        visible: gifRecordMode && !gifAreaLocked
        enabled: !gifAreaLocked
        z: 15
        onAreaChanged: {
            screenshotArea = Qt.rect(
                area.x + screenVirtualX,
                area.y + screenVirtualY,
                area.width, area.height)
            O_GifRecordVM.updateCaptureRect(
                screenshotArea.x, screenshotArea.y,
                screenshotArea.width, screenshotArea.height
            )
        }
        onInitialAreaChanged: {
            initialScreenshotArea = Qt.rect(
                initialArea.x + screenVirtualX,
                initialArea.y + screenVirtualY,
                initialArea.width, initialArea.height)
        }
    }

    // Window highlight rectangle (local coordinates)
    Rectangle {
        id: windowHighlight
        visible: captureUiReady && screenshotArea.width === 0 && screenshotArea.height === 0
                 && highlightRect.width > 0 && highlightRect.height > 0
        x: localHighlightRect.x
        y: localHighlightRect.y
        width:  localHighlightRect.width
        height: localHighlightRect.height
        color: "transparent"
        border.color: "#3B82F6"
        border.width: 2
        z: 5

        Rectangle {
            anchors.fill: parent
            color: "#153B82F6"
        }

        Repeater {
            model: [
                { ax: 0,              ay: 0,              bx: 1, by: 0, cx: 0, cy: 1 },
                { ax: parent.width,   ay: 0,              bx:-1, by: 0, cx: 0, cy: 1 },
                { ax: 0,              ay: parent.height,  bx: 1, by: 0, cx: 0, cy:-1 },
                { ax: parent.width,   ay: parent.height,  bx:-1, by: 0, cx: 0, cy:-1 }
            ]
            delegate: Item {
                x: modelData.ax - (modelData.bx > 0 ? 0 : 3)
                y: modelData.ay - (modelData.cy > 0 ? 0 : 3)
                Rectangle { x:0; y:0; width:12; height:3; color:"#3B82F6"; transform: Scale { xScale: modelData.bx } }
                Rectangle { x:0; y:0; width:3; height:12; color:"#3B82F6"; transform: Scale { yScale: modelData.cy } }
            }
        }
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
            var ax = toAbsX(mouseX), ay = toAbsY(mouseY)
            textClickPending = false
            textPressMoved = false
            if (gifRecordMode) {
                if (gifAreaLocked) {
                    return
                }
                if (isPointInRect(screenshotArea, ax, ay)) {
                    offset.x = ax
                    offset.y = ay
                    initialScreenshotArea = screenshotArea
                }
                return
            }
            if (spaceHeld && hasSelectionArea()) {
                offset.x = ax
                offset.y = ay
                initialScreenshotArea = screenshotArea
                return
            }
            if (tool.activeTool === "text" && screenshotArea.width !== 0 && screenshotArea.height !== 0
                    && isPointInRect(screenshotArea, ax, ay)) {
                textClickPending = true
                textPressMoved = false
                textClickAbsPoint = Qt.point(ax, ay)
                textClickLocalPoint = Qt.point(mouseX, mouseY)
                offset.x = ax
                offset.y = ay
                initialScreenshotArea = screenshotArea
                return
            }
            if (textEditor.visible) {
                hideTextEditor()
            }
            handleMousePressed(ax, ay)
        }

        onPositionChanged: function(mouse) {
            var ax = toAbsX(mouseX), ay = toAbsY(mouseY)
            if (pressed && textClickPending) {
                var tdx = ax - textClickAbsPoint.x
                var tdy = ay - textClickAbsPoint.y
                if (Math.abs(tdx) > 3 || Math.abs(tdy) > 3) {
                    textPressMoved = true
                    textClickPending = false
                    if (textEditor.visible)
                        hideTextEditor()
                } else {
                    return
                }
            }
            if (gifRecordMode) {
                if (gifAreaLocked) {
                    return
                }
                if (pressed && isPointInRect(screenshotArea, ax, ay)) {
                    var dx = ax - offset.x
                    var dy = ay - offset.y
                    screenshotArea = Qt.rect(
                        initialScreenshotArea.x + dx,
                        initialScreenshotArea.y + dy,
                        screenshotArea.width, screenshotArea.height)
                    O_GifRecordVM.updateCaptureRect(
                        screenshotArea.x, screenshotArea.y,
                        screenshotArea.width, screenshotArea.height
                    )
                }
                return
            }
            if (pressed && spaceHeld && hasSelectionArea()) {
                var sdx = ax - offset.x
                var sdy = ay - offset.y
                screenshotArea = Qt.rect(
                    initialScreenshotArea.x + sdx,
                    initialScreenshotArea.y + sdy,
                    screenshotArea.width, screenshotArea.height)
                return
            }
            // Magnifier follows mouse in local coordinates
            magnifier.magnifierMouseX = mouseX
            magnifier.magnifierMouseY = mouseY
            magnifier.magnifierAbsX = ax
            magnifier.magnifierAbsY = ay
            handleMouseMoved(ax, ay, pressed, mouse.modifiers)
        }

        onReleased: function(mouse) {
            if (textClickPending && !textPressMoved) {
                openTextEditor(textClickLocalPoint.x, textClickLocalPoint.y)
                textClickPending = false
                textPressMoved = false
                return
            }
            textClickPending = false
            textPressMoved = false
            if (gifRecordMode) {
                if (gifAreaLocked) {
                    return
                }
                initialScreenshotArea = screenshotArea
                gifResizeHandleGroup.initialArea = Qt.rect(
                    initialScreenshotArea.x - screenVirtualX,
                    initialScreenshotArea.y - screenVirtualY,
                    initialScreenshotArea.width, initialScreenshotArea.height)
                return
            }
            handleMouseReleased()
        }

        onWheel: function(wheel) { wheel.accepted = true }

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Space) {
                spaceHeld = true
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
                spaceHeld = false
                event.accepted = true
                return
            }

            if (hotkeyGestureArmed && keyMatchesToken(event.key, hotkeyMainToken)) {
                hotkeyLongPressTimer.stop()
                if (!hotkeyLongPressed && hasSelectionArea())
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


