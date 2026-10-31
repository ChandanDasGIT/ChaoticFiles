import QtQuick
import QtQuick.Controls.Basic

// The keyboard shortcuts, opened from the settings. Every change applies at once.
DialogFrame {
    id: root

    property bool capturing: false         // while a shortcut is being set, Escape belongs to it

    closePolicy: capturing ? Popup.NoAutoClose : Popup.CloseOnEscape
    width: Math.min(Theme.settingsWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.settingsHeight, parent.height - 2 * Theme.dialogMargin)

    Text {
        text: "Keyboard Shortcuts"
        font.pixelSize: Theme.dialogTitleSize
        font.weight: Font.DemiBold
        color: Theme.text
    }
    ToolbarButton {
        id: closeButton
        anchors.right: parent.right
        anchors.top: parent.top
        icon: Icons.close
        onClicked: root.close()
    }

    Flickable {
        anchors.top: closeButton.bottom
        anchors.topMargin: 8
        anchors.bottom: footer.top
        anchors.bottomMargin: 10
        width: parent.width
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ThinScrollBar {}

        Column {
            id: column
            width: parent.width - Theme.scrollbarWidth
            spacing: 4

            Repeater {
                model: ShortcutsController.actions
                delegate: ShortcutRow {
                    required property var modelData
                    width: column.width
                    actionId: modelData.id
                    title: modelData.title
                    keys: modelData.keys
                    onCaptureChanged: (active) => root.capturing = active
                }
            }
        }
    }

    DialogButton {
        id: footer
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: "Restore Defaults"
        onClicked: ShortcutsController.resetAll()
    }
}
