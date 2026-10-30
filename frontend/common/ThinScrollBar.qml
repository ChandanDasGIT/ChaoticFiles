import QtQuick
import QtQuick.Controls.Basic

// The scrollbar used everywhere: a thin handle, no track.
ScrollBar {
    id: root

    policy: ScrollBar.AsNeeded
    contentItem: Rectangle {
        implicitWidth: Theme.scrollbarWidth / 2
        radius: width / 2
        color: root.hovered || root.pressed ? Theme.scrollbarHandleHover : Theme.scrollbarHandle
        opacity: root.size < 1 ? 1 : 0
    }
    background: Item {}
}
