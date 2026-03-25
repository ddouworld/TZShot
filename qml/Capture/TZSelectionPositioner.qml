import QtQuick

QtObject {
    id: positioner

    property int gap: 6

    function clamp(value, minValue, maxValue) {
        return Math.max(minValue, Math.min(value, maxValue))
    }

    function alignLeft(areaRect, itemWidth, viewportWidth) {
        return clamp(areaRect.x, 0, Math.max(0, viewportWidth - itemWidth))
    }

    function alignRight(areaRect, itemWidth, viewportWidth) {
        const preferredX = areaRect.x + areaRect.width - itemWidth
        return clamp(preferredX, 0, Math.max(0, viewportWidth - itemWidth))
    }

    function placeBelowOrAbove(areaRect, itemHeight, viewportHeight, customGap) {
        const spacing = customGap === undefined ? gap : customGap
        const belowY = areaRect.y + areaRect.height + spacing
        if (belowY + itemHeight <= viewportHeight)
            return belowY
        return areaRect.y - itemHeight - spacing
    }

    function placeAbove(areaRect, itemHeight, viewportHeight, customGap) {
        const spacing = customGap === undefined ? gap : customGap
        const preferredY = areaRect.y - itemHeight - spacing
        return clamp(preferredY, 0, Math.max(0, viewportHeight - itemHeight))
    }
}
