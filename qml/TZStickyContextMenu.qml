import QtQuick
import Qt.labs.platform 1.0 as Platform
import QtCore
import QtQml

Item {
    id: root
    visible: false

    required property var menuPrefs
    required property string imageUrl
    required property real imgOpacity
    required property real zoomFactor
    required property bool toolbarVisible
    required property url saveFolder

    required property var overwriteWithEdits
    required property var copyImageToClipboard
    required property var openChatPanel
    required property var hideOrToggleToolbar
    required property var setOpacityValue
    required property var setZoomValue
    required property var rotateCurrent
    required property var mirrorCurrent
    required property var resetDisplaySize
    required property var resetWindowPosition
    required property var saveOriginalImage
    required property var saveMergedTo
    required property var closeWindow

    property bool contextMenuOpen: false

    function open() {
        contextMenu.open()
    }

    Platform.Menu {
        id: contextMenu
        onAboutToShow: root.contextMenuOpen = true
        onAboutToHide: root.contextMenuOpen = false

        Platform.MenuItem { text: qsTr("当前透明度 %1%").arg(Math.round(root.imgOpacity * 100)); enabled: false }
        Platform.MenuSeparator {}

        Platform.MenuItem { visible: root.menuPrefs.pinSaveMerged; text: qsTr("覆盖贴图(合并标注)"); onTriggered: root.overwriteWithEdits() }
        Platform.MenuItem { visible: root.menuPrefs.pinCopy; text: qsTr("复制贴图"); onTriggered: root.copyImageToClipboard(root.imageUrl) }
        Platform.MenuItem { visible: root.menuPrefs.pinAi; text: qsTr("AI 编辑"); onTriggered: root.openChatPanel() }
        Platform.MenuItem {
            visible: root.menuPrefs.pinToolbar
            text: root.toolbarVisible ? qsTr("隐藏工具栏") : qsTr("显示工具栏")
            onTriggered: root.hideOrToggleToolbar()
        }
        Platform.MenuItem { visible: root.menuPrefs.pinClose; text: qsTr("关闭窗口"); onTriggered: root.closeWindow() }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("透明度 +5%"); onTriggered: root.setOpacityValue(root.imgOpacity + 0.05) }
        Platform.MenuItem { text: qsTr("透明度 -5%"); onTriggered: root.setOpacityValue(root.imgOpacity - 0.05) }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("缩小"); onTriggered: root.setZoomValue(root.zoomFactor - 0.1) }
        Platform.MenuItem { text: qsTr("放大"); onTriggered: root.setZoomValue(root.zoomFactor + 0.1) }
        Platform.MenuItem { text: qsTr("旋转 90°"); onTriggered: root.rotateCurrent(90) }
        Platform.MenuItem { text: qsTr("镜像"); onTriggered: root.mirrorCurrent() }
        Platform.MenuItem { text: qsTr("恢复原始大小"); onTriggered: root.resetDisplaySize() }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: qsTr("重置位置")
            onTriggered: root.resetWindowPosition()
        }

        Platform.MenuSeparator {}

        Platform.MenuItem {
            text: root.toolbarVisible ? qsTr("隐藏工具栏") : qsTr("显示工具栏")
            onTriggered: root.hideOrToggleToolbar()
        }

        Platform.MenuItem { text: qsTr("AI 编辑"); onTriggered: root.openChatPanel() }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("保存原图"); onTriggered: saveDialog.open() }
        Platform.MenuItem { text: qsTr("另存为(合并标注)"); onTriggered: mergedSaveDialog.open() }
        Platform.MenuItem { text: qsTr("覆盖贴图(合并标注)"); onTriggered: root.overwriteWithEdits() }
        Platform.MenuItem { text: qsTr("复制贴图"); onTriggered: root.copyImageToClipboard(root.imageUrl) }

        Platform.MenuSeparator {}

        Platform.Menu {
            title: qsTr("常用项置顶")
            Platform.MenuItem { text: qsTr("覆盖贴图"); checkable: true; checked: root.menuPrefs.pinSaveMerged; onTriggered: root.menuPrefs.pinSaveMerged = !root.menuPrefs.pinSaveMerged }
            Platform.MenuItem { text: qsTr("复制贴图"); checkable: true; checked: root.menuPrefs.pinCopy; onTriggered: root.menuPrefs.pinCopy = !root.menuPrefs.pinCopy }
            Platform.MenuItem { text: qsTr("AI 编辑"); checkable: true; checked: root.menuPrefs.pinAi; onTriggered: root.menuPrefs.pinAi = !root.menuPrefs.pinAi }
            Platform.MenuItem { text: qsTr("显示/隐藏工具栏"); checkable: true; checked: root.menuPrefs.pinToolbar; onTriggered: root.menuPrefs.pinToolbar = !root.menuPrefs.pinToolbar }
            Platform.MenuItem { text: qsTr("关闭窗口"); checkable: true; checked: root.menuPrefs.pinClose; onTriggered: root.menuPrefs.pinClose = !root.menuPrefs.pinClose }
        }

        Platform.MenuSeparator {}

        Platform.MenuItem { text: qsTr("关闭窗口"); onTriggered: root.closeWindow() }
    }

    Platform.FileDialog {
        id: saveDialog
        title: qsTr("选择保存位置")
        folder: root.saveFolder
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: root.saveOriginalImage(root.imageUrl, saveDialog.file)
    }

    Platform.FileDialog {
        id: mergedSaveDialog
        title: qsTr("另存为(合并标注)")
        folder: root.saveFolder
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: ["PNG (*.png)", "JPG (*.jpg)", "BMP (*.bmp)"]
        onAccepted: root.saveMergedTo(mergedSaveDialog.file)
    }
}
