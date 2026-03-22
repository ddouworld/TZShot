import QtQuick
import QtQuick.Window
import QtQuick.Layouts
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import TZControls 1.0

Window {
    width: 750
    height: 520
    title: qsTr("设置")
    visible: true
    color: "#F8FAFC"
    property string currentPage: "general"

    Component.onCompleted: centerWindow()
    onScreenChanged: centerWindow()

    function centerWindow() {
        if (Screen) {
            const screen = this.screen || Screen.primaryScreen
            x = (screen.width - width) / 2
            y = (screen.height - height) / 2
        }
    }

    FolderDialog {
        id: saveDialog
        title: qsTr("选择保存位置")
        currentFolder: O_ImageSaver.savePath
        onAccepted: {
            O_ImageSaver.setSavePath(saveDialog.selectedFolder)
        }
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 左侧导航栏
        TZRightPanel {
            Layout.preferredWidth: 160
            Layout.fillHeight: true
            onSignalSetting: {
                currentPage = "general"
            }
            onSignalHotkeySettings: {
                currentPage = "hotkey"
            }
        }

        // 分割线
        Rectangle {
            Layout.preferredWidth: 1
            Layout.fillHeight: true
            color: "#E2E8F0"
        }

        // 右侧内容区
        StackLayout {
            Layout.fillHeight: true
            Layout.fillWidth: true
            currentIndex: currentPage === "general" ? 0 : 1

            TZGeneralSettings {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }

            TZHotkeySettings {
                Layout.fillHeight: true
                Layout.fillWidth: true
            }
        }
    }
}

