import QtQuick
import QtQuick.Window
import QtQuick.Shapes
import QtQuick.Layouts
import com.Tz.tray 1.0

TZWindow {
    id: root
    visibility: Window.Hidden
    title: qsTr("TZshot")
    color: "transparent"
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

    property var settingWinObj: null
    property var aboutWinObj: null

    property var overlayList: []
    property var overlayComponent: null

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
        _destroyOverlays()
        _prepareSessionState()

        Qt.callLater(function() {
            O_ScreenCapture.captureDesktop()

            var screens = Qt.application.screens
            if (screens.length === 0)
                return

            var primaryIdx = 0
            for (var i = 0; i < screens.length; i++) {
                if (screens[i].virtualX === 0 && screens[i].virtualY === 0) {
                    primaryIdx = i
                    break
                }
            }
            var primary = screens[primaryIdx]

            root.screenVirtualX = primary.virtualX
            root.screenVirtualY = primary.virtualY
            root.x = primary.virtualX
            root.y = primary.virtualY
            root.width = primary.width
            root.height = primary.height

            O_ScreenCapture.grabToPaintBoard(
                root.paintBoard,
                Qt.rect(root.screenVirtualX, root.screenVirtualY, root.width, root.height)
            )

            if (overlayComponent === null)
                overlayComponent = Qt.createComponent("qrc:/qml/TZScreenOverlay.qml")

            var newList = []
            for (var j = 0; j < screens.length; j++) {
                if (j === primaryIdx)
                    continue

                var sc = screens[j]
                if (overlayComponent.status === Component.Ready) {
                    var ov = overlayComponent.createObject(null, {
                        "delegate": root,
                        "screenVX": sc.virtualX,
                        "screenVY": sc.virtualY,
                        "x": sc.virtualX,
                        "y": sc.virtualY,
                        "width": sc.width,
                        "height": sc.height,
                        "visibility": Window.Windowed
                    })
                    if (ov) {
                        ov.raise()
                        newList.push(ov)
                    }
                } else if (overlayComponent.status === Component.Error) {
                    console.error("TZScreenOverlay load failed:", overlayComponent.errorString())
                }
            }
            overlayList = newList

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

    function _destroyOverlays() {
        for (var i = 0; i < overlayList.length; i++)
            overlayList[i].destroy()
        overlayList = []
    }

    function resetArea() {
        _destroyOverlays()
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
        if (settingWinObj !== null) {
            settingWinObj.raise()
            settingWinObj.requestActivate()
            return
        }
        var component = Qt.createComponent("qrc:/qml/Setting/TZSettingWin.qml")
        if (component.status === Component.Ready)
            settingWinObj = component.createObject(null)
        else if (component.status === Component.Error)
            console.error("Setting window load failed:", component.errorString())

        if (settingWinObj !== null) {
            settingWinObj.visible = true
            settingWinObj.raise()
            settingWinObj.requestActivate()
            settingWinObj.closing.connect(function() {
                settingWinObj.destroy(100)
                settingWinObj = null
            })
        }
    }

    Connections {
        target: O_StickyViewModel
        function onStickyReady(imageUrl, imgRect) {
            root.createStickyWindow(imageUrl, imgRect)
        }
    }

    Connections {
        target: O_OcrVM
        function onIsRecognizingChanged() {
            if (O_OcrVM.isRecognizing)
                root.createOcrResultWindow()
        }
        function onResultReady(text) {
            root.createOcrResultWindow()
        }
        function onRecognizeFailed(errorMessage) {
            trayHelper.showMessage(qsTr("OCR 识别失败"), errorMessage)
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
