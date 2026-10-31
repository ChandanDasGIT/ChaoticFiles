import QtQuick
import QtQuick.Controls.Basic

// The settings window (Ctrl+,). Every change applies at once.
DialogFrame {
    id: root

    width: Math.min(Theme.settingsWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.settingsHeight, parent.height - 2 * Theme.dialogMargin)

    function openAbout() {
        open()
        Qt.callLater(() => {
            flick.contentY = Math.max(0, flick.contentHeight - flick.height)
        })
    }

    function scrollToTop() {
        flick.contentY = 0
    }

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
        id: flick
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
                title: "Opens to"
                choices: ["Home", "Last Tabs", "A Folder"]
                current: SettingsController.startMode
                onChosen: (index) => {
                    if (index === 2 && SettingsController.startFolder === "") SettingsController.startFolder = TabsController.currentSession.path
                    SettingsController.startMode = index
                }
            }
            StartFolderRow {
                width: parent.width
                visible: SettingsController.startMode === 2
            }
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

            SettingsLink {
                width: parent.width
                title: "Keyboard shortcuts"
                onClicked: shortcutsDialog.open()
            }
            SettingsSection {
                width: parent.width
                title: "Default app changes"
                choices: ["System-wide", "This app only"]
                current: SettingsController.localDefaults ? 1 : 0
                onChosen: (index) => SettingsController.localDefaults = index === 1
            }
            SettingsLink {
                width: parent.width
                title: "Default apps"
                onClicked: defaultAppsDialog.open()
            }
            AboutSection {
                width: parent.width
            }
        }
    }

    ShortcutsDialog { id: shortcutsDialog }
    DefaultAppsDialog { id: defaultAppsDialog }
}
