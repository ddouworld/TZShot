
import QtQuick
import QtQuick.Window
import QtQuick.Shapes
import QtQuick.Layouts
import com.Tz.tray 1.0
import "Capture"

TZWindow {
    id: root
    visibility: Window.Hidden
    title: qsTr("TZshot")
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    property var settingWinObj: null
    property var aboutWinObj: null
    property var longCaptureBarObj: null
    property var longCaptureBarComponent: null
    property var longCapturePreviewObj: null
    property var longCapturePreviewComponent: null
    property var longCaptureFrameObj: null
    property var longCaptureFrameComponent: null
    property rect pendingLongCaptureRect: Qt.rect(0, 0, 0, 0)

    function _prepareSessionState() {
        captureUiReady = false
        root.opacity = 0.0
        screenshotArea = Qt.rect(0, 0, 0, 0)
        initialScreenshotArea = Qt.rect(0, 0, 0, 0)
        highlightRect = Qt.rect(0, 0, 0, 0)
        screenShotUrl = ""

        paintBoard.reset()
        tool.visible = false

        tool.activeTool = ""
        tool.markToolInfoVisible = false

        gifRecordMode = false
        if (gifBarObj)
            gifBarObj.visible = false

        if (typeof resizeHandleGroup !== "undefined") {
            resizeHandleGroup.area = Qt.rect(0, 0, 0, 0)
            resizeHandleGroup.initialArea = Qt.rect(0, 0, 0, 0)
            resizeHandleGroup.visible = false
        }
        if (typeof gifResizeHandleGroup !== "undefined") {
            gifResizeHandleGroup.area = Qt.rect(0, 0, 0, 0)
            gifResizeHandleGroup.initialArea = Qt.rect(0, 0, 0, 0)
            gifResizeHandleGroup.visible = false
        }

        if (typeof hideTextEditor === "function")
            hideTextEditor()
    }

    function startScreenshot() {
        root.visibility = Window.Hidden
        _prepareSessionState()

        Qt.callLater(function() {
            O_ScreenCapture.captureDesktop()
            var virtualRect = O_ScreenCapture.virtualGeometry()
            if (virtualRect.width <= 0 || virtualRect.height <= 0)
                return

            root.screenVirtualX = virtualRect.x
            root.screenVirtualY = virtualRect.y

            O_ScreenCapture.grabToPaintBoard(
                root.paintBoard,
                Qt.rect(root.screenVirtualX, root.screenVirtualY, root.width, root.height)
            )

            root.visibility = Window.Windowed
            root.raise()
            root.requestActivate()
            Qt.callLater(function() {
                root.captureUiReady = true
                Qt.callLater(function() {
                    root.opacity = 1.0
                })
            })
        })
    }

    function showLongCaptureBar() {
        if (longCaptureBarObj !== null) {
            longCaptureBarObj.visible = true
            longCaptureBarObj.raise()
            return
        }
        if (longCaptureBarComponent === null)
            longCaptureBarComponent = Qt.createComponent("qrc:/qml/LongCapture/TZLongCaptureBar.qml")
        if (longCaptureBarComponent.status === Component.Ready) {
            longCaptureBarObj = longCaptureBarComponent.createObject(null)
            if (longCaptureBarObj) {
                longCaptureBarObj.x = Math.round((Screen.width - longCaptureBarObj.width) / 2)
                longCaptureBarObj.y = 26
                longCaptureBarObj.visible = true
                longCaptureBarObj.raise()
                longCaptureBarObj.startRequested.connect(function() {
                    if (pendingLongCaptureRect.width === 0 || pendingLongCaptureRect.height === 0)
                        return
                    longCaptureBarObj.running = true
                    longCaptureBarObj.message = qsTr("滚动检测中，滑动内容会自动拼接")
                    O_ScrollCaptureVM.start(pendingLongCaptureRect)
                })
                longCaptureBarObj.stopRequested.connect(function() {
                    if (longCaptureBarObj.running) {
                        O_ScrollCaptureVM.stop()
                    } else {
                        hideLongCaptureBar()
                        root.resetArea()
                    }
                })
            }
        } else if (longCaptureBarComponent.status === Component.Error) {
            console.error("LongCaptureBar load failed:", longCaptureBarComponent.errorString())
        }
    }

    function showLongCapturePreview() {
        if (longCapturePreviewObj !== null) {
            longCapturePreviewObj.visible = true
            longCapturePreviewObj.raise()
            return
        }
        if (longCapturePreviewComponent === null)
            longCapturePreviewComponent = Qt.createComponent("qrc:/qml/LongCapture/TZLongCapturePreview.qml")
        if (longCapturePreviewComponent.status === Component.Ready) {
            longCapturePreviewObj = longCapturePreviewComponent.createObject(null)
            if (longCapturePreviewObj) {
                longCapturePreviewObj.x = Math.max(0, Screen.width - longCapturePreviewObj.width - 26)
                longCapturePreviewObj.y = Math.max(20, Math.round((Screen.height - longCapturePreviewObj.height) / 2))
                longCapturePreviewObj.visible = true
                longCapturePreviewObj.raise()
            }
        } else if (longCapturePreviewComponent.status === Component.Error) {
            console.error("LongCapturePreview load failed:", longCapturePreviewComponent.errorString())
        }
    }

    function showLongCaptureFrame(captureRect) {
        if (longCaptureFrameComponent === null)
            longCaptureFrameComponent = Qt.createComponent("qrc:/qml/LongCapture/TZLongCaptureFrame.qml")
        if (longCaptureFrameObj === null && longCaptureFrameComponent.status === Component.Ready) {
            longCaptureFrameObj = longCaptureFrameComponent.createObject(null)
        } else if (longCaptureFrameComponent.status === Component.Error) {
            console.error("LongCaptureFrame load failed:", longCaptureFrameComponent.errorString())
        }
        if (longCaptureFrameObj) {
            longCaptureFrameObj.captureRect = captureRect
            longCaptureFrameObj.visible = true
            longCaptureFrameObj.raise()
        }
    }

    function hideLongCaptureBar() {
        if (longCaptureBarObj !== null) {
            longCaptureBarObj.close()
            longCaptureBarObj.destroy(100)
            longCaptureBarObj = null
        }
        if (longCapturePreviewObj !== null) {
            longCapturePreviewObj.close()
            longCapturePreviewObj.destroy(100)
            longCapturePreviewObj = null
        }
        if (longCaptureFrameObj !== null) {
            longCaptureFrameObj.close()
            longCaptureFrameObj.destroy(100)
            longCaptureFrameObj = null
        }
        pendingLongCaptureRect = Qt.rect(0, 0, 0, 0)
    }

    function beginLongCapture(captureRect) {
        pendingLongCaptureRect = captureRect
        root.visibility = Window.Hidden
        showLongCaptureBar()
        showLongCapturePreview()
        showLongCaptureFrame(captureRect)
        if (longCaptureBarObj) {
            longCaptureBarObj.running = false
            longCaptureBarObj.frameCount = 0
            longCaptureBarObj.message = qsTr("点击开始后，滑动内容自动拼接")
        }
        if (longCapturePreviewObj) {
            longCapturePreviewObj.previewUrl = ""
            longCapturePreviewObj.frameCount = 0
            longCapturePreviewObj.statusText = qsTr("等待开始")
        }
    }

    function resetArea() {
        _prepareSessionState()
        O_ScreenCapture.releaseDesktopSnapshot()
        root.visibility = Window.Hidden
    }

    function showAboutWin() {
        if (aboutWinObj !== null) {
            aboutWinObj.raise()
            aboutWinObj.requestActivate()
            return
        }
        var component = Qt.createComponent("qrc:/qml/Setting/TZAboutWin.qml")
        if (component.status === Component.Ready)
            aboutWinObj = component.createObject(null)
        else if (component.status === Component.Error)
            console.error("About window load failed:", component.errorString())

        if (aboutWinObj !== null) {
            aboutWinObj.visible = true
            aboutWinObj.raise()
            aboutWinObj.requestActivate()
            aboutWinObj.closing.connect(function() {
                aboutWinObj.destroy(100)
                aboutWinObj = null
            })
        }
    }

    function showSettingWin() {
        O_WidgetWindows.showSettings()
    }

    Connections {
        target: O_StickyViewModel
        function onStickyReady(imageUrl, imgRect) {
            root.createStickyWindow(imageUrl, imgRect)
        }
    }

    Connections {
        target: O_OcrVM
        function onRecognizeFailed(errorMessage) {
            trayHelper.showMessage(qsTr("OCR 识别失败"), errorMessage)
        }
    }

    Connections {
        target: O_ScrollCaptureVM
        function onCaptureStarted() {
            if (longCaptureBarObj) {
                longCaptureBarObj.running = true
                longCaptureBarObj.message = qsTr("滚动检测中，滑动内容会自动拼接")
            }
            trayHelper.showMessage(qsTr("长截图开始"), qsTr("请直接滚动目标内容，系统会在滚动变化时自动拼接"))
        }
        function onCaptureSucceeded(savedPath) {
            if (longCaptureBarObj) {
                longCaptureBarObj.running = false
                longCaptureBarObj.message = qsTr("长截图完成")
            }
            root.hideLongCaptureBar()
            root.resetArea()
            trayHelper.showMessage(qsTr("长截图完成"), qsTr("已复制到剪贴板并保存到：") + savedPath)
        }
        function onCaptureFailed(errorMessage) {
            if (longCaptureBarObj) {
                longCaptureBarObj.running = false
                longCaptureBarObj.message = qsTr("长截图失败")
            }
            root.hideLongCaptureBar()
            root.resetArea()
            trayHelper.showMessage(qsTr("长截图失败"), errorMessage)
        }
        function onCapturedFramesChanged() {
            if (longCaptureBarObj)
                longCaptureBarObj.frameCount = O_ScrollCaptureVM.capturedFrames
            if (longCapturePreviewObj)
                longCapturePreviewObj.frameCount = O_ScrollCaptureVM.capturedFrames
        }
        function onStatusTextChanged() {
            if (longCaptureBarObj)
                longCaptureBarObj.message = O_ScrollCaptureVM.statusText
            if (longCapturePreviewObj)
                longCapturePreviewObj.statusText = O_ScrollCaptureVM.statusText
        }
        function onPreviewImageUrlChanged() {
            if (longCapturePreviewObj)
                longCapturePreviewObj.previewUrl = O_ScrollCaptureVM.previewImageUrl
        }
    }

    Connections {
        target: O_InstanceBridge
        function onActivationRequested() {
            if (root.visible) {
                root.raise()
                root.requestActivate()
                return
            }
            startScreenshot()
        }
    }

    TrayIconHelper {
        id: trayHelper
        function onTrayClicked(clickType) {
            if (clickType === 0 || clickType === 2)
                startScreenshot()
        }
        onScreenshotTriggered: startScreenshot()
        onShowSettingTriggered: { showSettingWin() }
        onShowAboutTriggered: { showAboutWin() }
        onExitTriggered: Qt.quit()
    }
    // MouseArea
    // {
    //     anchors.fill: parent;
    //     MouseArea {
    //         anchors.fill: parent
    //         onClicked: {
    //             console.log("x->", mouseX)
    //             console.log("y-->", mouseY)
    //         }
    //         onDoubleClicked: Qt.quit()
    //     }
    // }
    Component.onCompleted: {
        trayHelper.setIcon(":/resource/img/tray_icon.svg")
        trayHelper.show()

        O_GlobalShortcut.screenshotActivated.connect(function() { startScreenshot() })
        O_GlobalShortcut.screenshotSaveActivated.connect(function() { startScreenshot() })
        O_GlobalShortcut.stickyActivated.connect(function() { startScreenshot() })
        O_GlobalShortcut.toggleActivated.connect(function() {
            if (root.visible)
                resetArea()
            else
                startScreenshot()
        })
    }
}
