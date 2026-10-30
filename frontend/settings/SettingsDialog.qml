import QtQuick
import QtQuick.Controls.Basic

// The settings window (Ctrl+,). Every change applies at once.
DialogFrame {
    id: root

    property bool capturing: false         // while a shortcut is being set, Escape belongs to it

    closePolicy: capturing ? Popup.NoAutoClose : Popup.CloseOnEscape
    width: Math.min(Theme.settingsWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.settingsHeight, parent.height - 2 * Theme.dialogMargin)

    Text {
        id: title
        text: "Settings"
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
        anchors.bottom: parent.bottom
        width: parent.width
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ThinScrollBar {}

        Column {
            id: column
            width: parent.width - Theme.scrollbarWidth
            spacing: 4

            SettingsSection {
                width: parent.width
                title: "Theme"
                choices: ["System", "Dark", "Light"]
                current: SettingsController.themeMode
                onChosen: (index) => SettingsController.themeMode = index
            }
            SettingsSection {
                width: parent.width
                title: "View"
                choices: ["Details", "Grid"]
                current: SettingsController.viewMode
                onChosen: (index) => SettingsController.viewMode = index
            }
            SettingsSection {
                width: parent.width
                title: "Hidden files"
                choices: ["Hide", "Show"]
                current: SettingsController.showHidden ? 1 : 0
                onChosen: (index) => SettingsController.showHidden = index === 1
            }
            SettingsSection {
                width: parent.width
                title: "Thumbnails"
                choices: ["Off", "On"]
                current: SettingsController.showThumbnails ? 1 : 0
                onChosen: (index) => SettingsController.showThumbnails = index === 1
            }
            SettingsSection {
                width: parent.width
                title: "Preview panel"
                choices: ["Off", "On"]
                current: SettingsController.showPreview ? 1 : 0
                onChosen: (index) => SettingsController.showPreview = index === 1
            }
            SettingsSection {
                width: parent.width
                title: "Window buttons"
                choices: ["Automatic", "Show", "Hide"]
                current: SettingsController.windowButtons
                onChosen: (index) => SettingsController.windowButtons = index
            }

            Item {
                width: parent.width
                height: Theme.settingsRowHeight

                Text {
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 6
                    text: "Keyboard shortcuts"
                    font.pixelSize: Theme.fontSize
                    font.weight: Font.DemiBold
                    color: Theme.text
                }
                DialogButton {
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    text: "Restore Defaults"
                    onClicked: ShortcutsController.resetAll()
                }
            }

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
}
