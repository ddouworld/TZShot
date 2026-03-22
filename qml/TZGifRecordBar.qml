import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.5

// GIF 录制控制栏：在截图选区确定后显示
Rectangle {
    id: gifBar
    radius: 10
    color: "#FFFFFF"
    border.color: "#E2E8F0"
    border.width: 1
    layer.enabled: true

    // 外部传入的录制区域
    property rect captureRect: Qt.rect(0, 0, 0, 0)

    signal signalCancel
    signal signalRecordFinished

    readonly property bool isRecording: O_GifRecordVM.isRecording
    readonly property bool isEncoding: O_GifRecordVM.isEncoding
    readonly property int elapsedSecs: O_GifRecordVM.elapsedSecs
    readonly property int frameCount: O_GifRecordVM.frameCount

    Connections {
        target: O_GifRecordVM

        function onEncodingFinished(savedPath) {
            gifBar.signalRecordFinished()
        }

        function onEncodingFailed(error) {
            console.warn("[GifRecordBar] 录制失败：", error)
            gifBar.signalRecordFinished()
        }
    }

    SequentialAnimation {
        id: blinkAnim
        running: gifBar.isRecording
        loops: Animation.Infinite
        PropertyAnimation { target: recDot; property: "opacity"; to: 0.2; duration: 500 }
        PropertyAnimation { target: recDot; property: "opacity"; to: 1.0; duration: 500 }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 6
        layoutDirection: Qt.RightToLeft
        spacing: 4

        // 取消按钮
        Rectangle {
            width: 32; height: 32; radius: 7
            visible: !gifBar.isEncoding
            color: cancelArea.containsMouse ? "#FEE2E2" : "transparent"
            Behavior on color { ColorAnimation { duration: 100 } }
            Image {
                anchors.centerIn: parent
                source: "qrc:/resource/img/lc_x.svg"
                width: 18; height: 18
                fillMode: Image.PreserveAspectFit
                smooth: true
            }
            MouseArea {
                id: cancelArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onPressed: {
                    O_GifRecordVM.cancelRecording()
                    gifBar.signalCancel()
                }
            }
        }

        // 停止录制按钮
        Rectangle {
            width: 32; height: 32; radius: 7
            visible: gifBar.isRecording && !gifBar.isEncoding
            color: stopArea.containsMouse ? "#FEF3C7" : "transparent"
            Behavior on color { ColorAnimation { duration: 100 } }
            Image {
                anchors.centerIn: parent
                source: "qrc:/resource/img/lc_gif_stop.svg"
                width: 18; height: 18
                fillMode: Image.PreserveAspectFit
                smooth: true
                layer.enabled: true
                layer.effect: null
            }
            MouseArea {
                id: stopArea
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onPressed: O_GifRecordVM.stopRecording()
            }
        }

        Rectangle {
            width: 1; height: 20
            visible: gifBar.isRecording && !gifBar.isEncoding
            color: "#E2E8F0"
            Layout.alignment: Qt.AlignVCenter
        }

        // 录制中：时间 + 帧数
        Item {
            visible: gifBar.isRecording && !gifBar.isEncoding
            Layout.fillWidth: true
            height: 32
            implicitWidth: recRow.implicitWidth + 8

            Row {
                id: recRow
                anchors.centerIn: parent
                spacing: 6

                Rectangle {
                    id: recDot
                    width: 8; height: 8; radius: 4
                    color: "#EF4444"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: {
                        var m = Math.floor(gifBar.elapsedSecs / 60)
                        var s = gifBar.elapsedSecs % 60
                        return (m < 10 ? "0" + m : m) + ":" + (s < 10 ? "0" + s : s)
                    }
                    font.pixelSize: 13
                    font.family: "monospace"
                    color: "#1E293B"
                    anchors.verticalCenter: parent.verticalCenter
                }

                Text {
                    text: gifBar.frameCount + "f"
                    font.pixelSize: 11
                    color: "#94A3B8"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }

        // 编码中：进度提示
        Item {
            visible: gifBar.isEncoding
            Layout.fillWidth: true
            height: 32
            implicitWidth: encodingRow.implicitWidth + 16

            Row {
                id: encodingRow
                anchors.centerIn: parent
                spacing: 8

                Rectangle {
                    width: 16; height: 16
                    color: "transparent"
                    anchors.verticalCenter: parent.verticalCenter
                    Rectangle {
                        width: 16; height: 16
                        radius: 8
                        color: "transparent"
                        border.color: "#3B82F6"
                        border.width: 2
                        Rectangle {
                            width: 6; height: 6
                            color: "#3B82F6"
                            radius: 3
                            x: 5; y: -1
                        }
                        RotationAnimation on rotation {
                            running: gifBar.isEncoding
                            from: 0; to: 360
                            duration: 800
                            loops: Animation.Infinite
                        }
                    }
                }

                Text {
                    text: qsTr("正在生成 GIF...")
                    font.pixelSize: 13
                    color: "#1E293B"
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
        }
    }
}
