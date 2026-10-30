import QtQuick
import QtQuick.Controls.Basic

// A small themed menu. Each entry of `entries` is one of:
//     {text, icon, shortcut, checked, enabled, action}     a row; action() runs when it is clicked
//     {header: "Theme"}                                    a small title
//     {separator: true}                                    a line
Popup {
    id: root

    property var entries: []

    function openAt(sceneX, sceneY) {
        x = sceneX
        y = sceneY
        open()
    }

    parent: Overlay.overlay
    padding: Theme.menuPadding
    margins: Theme.menuPadding
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    background: Rectangle {
        color: Theme.menuBg
        border.color: Theme.menuBorder
        radius: Theme.menuRadius
    }

    contentWidth: column.width
    contentHeight: column.height

    Column {
        id: column

        width: {
            let most = Theme.menuMinWidth
            for (let i = 0; i < repeater.count; ++i) {
                const item = repeater.itemAt(i)
                if (item) most = Math.max(most, item.implicitWidth + Theme.menuShortcutSpace)
            }
            return most
        }

        Repeater {
            id: repeater
            model: root.entries
            delegate: PopupMenuItem {
                required property var modelData
                width: column.width
                entry: modelData
                onTriggered: {
                    root.close()
                    if (entry.action) entry.action()
                }
            }
        }
    }
}
