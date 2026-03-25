import QtQuick

Item {
    id: controller

    visible: false

    property bool enabled: false
    property bool armed: false
    property bool longPressed: false
    property string mainToken: ""
    property int longPressInterval: 280

    signal longPressTriggered()

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

    function beginGesture(shortcutSeq) {
        enabled = true
        armed = false
        longPressed = false
        mainToken = parseMainKeyToken(shortcutSeq)
        longPressTimer.stop()
    }

    function clearGesture() {
        enabled = false
        armed = false
        longPressed = false
        mainToken = ""
        longPressTimer.stop()
    }

    function armGesture() {
        if (!enabled)
            return
        armed = true
        longPressed = false
        longPressTimer.restart()
    }

    function handleKeyRelease(key) {
        if (!armed || !keyMatchesToken(key, mainToken))
            return false

        longPressTimer.stop()
        return true
    }

    Timer {
        id: longPressTimer
        interval: controller.longPressInterval
        repeat: false
        onTriggered: {
            if (controller.armed) {
                controller.longPressed = true
                controller.longPressTriggered()
            }
        }
    }
}
