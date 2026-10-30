import QtQuick
import QtQuick.Controls.Basic

// The look every dialog shares: a modal card in the middle of the window over a dimmed background.
Popup {
    id: root

    parent: Overlay.overlay
    modal: true
    dim: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    anchors.centerIn: Overlay.overlay
    padding: Theme.dialogPadding

    Overlay.modal: Rectangle { color: Theme.scrim }

    background: Rectangle {
        color: Theme.dialogBg
        border.color: Theme.menuBorder
        radius: Theme.dialogRadius
    }
}
