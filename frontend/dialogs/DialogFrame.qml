import QtQuick
import QtQuick.Controls.Basic

// The look every dialog shares. Each is a window of its own over the one that opened it, as in a desktop app:
// the window manager places it, it can be dragged by any empty part, and the window under it is not dimmed.
// It draws its own frame, like the main window. Esc closes it unless closePolicy says otherwise.
Window {
    id: root

    default property alias content: body.data
    property int padding: Theme.dialogPadding
    property real contentHeight: -1          // set it to size the window to its content; -1 measures the children
    property int closePolicy: Popup.CloseOnEscape

    // the main window, also for a dialog opened from another dialog
    readonly property var host: {
        let window = transientParent
        while (window && window.transientParent) window = window.transientParent
        return window
    }
    readonly property real maxWidth: (host ? host.width : Theme.windowWidth) - 2 * Theme.dialogMargin
    readonly property real maxHeight: (host ? host.height : Theme.windowHeight) - 2 * Theme.dialogMargin

    signal aboutToShow()
    signal opened()
    signal closed()

    function open() {
        aboutToShow()
        show()
        requestActivate()
    }

    flags: Qt.Dialog | Qt.FramelessWindowHint
    modality: Qt.WindowModal
    color: "transparent"
    height: (contentHeight >= 0 ? contentHeight : body.childrenRect.height) + 2 * padding

    // where the desktop lets an app choose, the middle of the window that opened it
    onVisibleChanged: {
        if (visible) {
            if (transientParent) {
                x = transientParent.x + (transientParent.width - width) / 2
                y = transientParent.y + (transientParent.height - height) / 2
            }
            opened()
        } else {
            closed()
            if (transientParent) transientParent.requestActivate()
        }
    }

    Shortcut {
        sequence: "Escape"
        enabled: root.visible && (root.closePolicy & Popup.CloseOnEscape)
        onActivated: root.close()
    }

    Rectangle {
        anchors.fill: parent
        color: Theme.dialogBg
        border.color: Theme.menuBorder
        radius: Theme.dialogRadius

        DragHandler {
            target: null
            onActiveChanged: if (active) root.startSystemMove()
        }
    }

    Item {
        id: body
        anchors.fill: parent
        anchors.margins: root.padding
    }
}
