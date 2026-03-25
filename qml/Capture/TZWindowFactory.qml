import QtQuick

QtObject {
    id: factory

    property var stickyWindows: []
    property var ocrWindows: []
    property var stickyComponent: null
    property var ocrComponent: null

    function createStickyWindow(imageUrl, imgRect) {
        if (!imageUrl || imageUrl.length === 0) {
            console.error("Failed to create sticky window: invalid image URL")
            return null
        }

        if (stickyComponent === null)
            stickyComponent = Qt.createComponent("qrc:/qml/Windows/TZStickyWindow.qml")

        var newWindow = null
        if (stickyComponent.status === Component.Ready) {
            newWindow = stickyComponent.createObject(null, {
                "imageUrl": imageUrl,
                "imgRect": imgRect
            })
        } else if (stickyComponent.status === Component.Error) {
            console.error("Failed to load TZStickyWindow.qml:", stickyComponent.errorString())
        }

        if (!newWindow) {
            console.error("Failed to create sticky window object")
            return null
        }

        newWindow.visible = true
        newWindow.raise()
        newWindow.requestActivate()

        var stickyList = stickyWindows.slice()
        stickyList.push(newWindow)
        stickyWindows = stickyList

        var windowRef = newWindow
        function onWindowClosing(closeEvent) {
            var idx = stickyWindows.indexOf(windowRef)
            if (idx !== -1) {
                var updatedList = stickyWindows.slice()
                updatedList.splice(idx, 1)
                stickyWindows = updatedList
            }
            windowRef.closing.disconnect(onWindowClosing)
            windowRef = null
        }
        newWindow.closing.connect(onWindowClosing)
        return newWindow
    }

    function createOcrResultWindow() {
        if (ocrWindows.length > 0) {
            var existing = ocrWindows[ocrWindows.length - 1]
            if (existing) {
                existing.show()
                existing.raise()
                existing.requestActivate()
                return existing
            }
        }

        if (ocrComponent === null)
            ocrComponent = Qt.createComponent("qrc:/qml/Windows/TZOcrResult.qml")

        var win = null
        if (ocrComponent.status === Component.Ready) {
            win = ocrComponent.createObject(null)
        } else if (ocrComponent.status === Component.Error) {
            console.error("Failed to load TZOcrResult.qml:", ocrComponent.errorString())
        }

        if (!win)
            return null

        win.show()
        win.raise()
        win.requestActivate()

        var list = ocrWindows.slice()
        list.push(win)
        ocrWindows = list

        var winRef = win
        function onOcrClosing(closeEvent) {
            var idx = ocrWindows.indexOf(winRef)
            if (idx !== -1) {
                var updatedList = ocrWindows.slice()
                updatedList.splice(idx, 1)
                ocrWindows = updatedList
            }
            winRef.closing.disconnect(onOcrClosing)
            winRef = null
        }
        win.closing.connect(onOcrClosing)
        return win
    }

    function closeTopOcrWindow() {
        if (ocrWindows.length === 0)
            return false

        var topWin = ocrWindows[ocrWindows.length - 1]
        if (!topWin)
            return false

        topWin.close()
        return true
    }
}
