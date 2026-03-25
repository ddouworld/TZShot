import QtQuick

Rectangle {
//    id:leftTop
//    x: screenshotArea.x - handleSize/2
//    y: screenshotArea.y - handleSize/2
    property int handleSize: 8
    width: handleSize
    height: handleSize
    radius: handleSize/2
    color: "#0088FF"
    border.color: "white"
    border.width: 1
}
