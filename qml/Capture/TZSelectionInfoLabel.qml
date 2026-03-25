import QtQuick

TZLable {
    id: label

    property rect logicalRect: Qt.rect(0, 0, 0, 0)
    property rect physicalRect: Qt.rect(0, 0, 0, 0)

    width: 360
    height: 20
    lableText: qsTr("X:%1 Y:%2  P:%3*%4 L:%5*%6  ")
        .arg(logicalRect.x)
        .arg(logicalRect.y)
        .arg(Math.abs(physicalRect.x))
        .arg(Math.abs(physicalRect.y))
        .arg(Math.abs(logicalRect.width))
        .arg(Math.abs(logicalRect.height))

}
