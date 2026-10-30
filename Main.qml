import QtQuick
import QtQuick.Layouts

// The window: tabs on top, then the toolbar, the sidebar with one file list per tab, and the status bar.
// The app draws its own title bar, because the tabs live in it.
Window {
    id: window

    readonly property var session: TabsController.currentSession

    function focusFiles() {
        const view = fileViews.itemAt(TabsController.currentIndex)
        if (view) view.forceActiveFocus()
    }

    function showRowMenu(isDirectory, path, sceneX, sceneY) {
        const entries = [{text: "Open", icon: Icons.open, shortcut: "Enter", action: () => window.session.openCurrent()}]
        if (isDirectory)
            entries.push({text: "Open in New Tab", icon: Icons.newTab, action: () => TabsController.openInBackground(path)})
        entries.push({separator: true})
        entries.push({text: "Copy Path", icon: Icons.copy, action: () => window.session.copyPaths()})
        rowMenu.entries = entries
        rowMenu.openAt(sceneX, sceneY)
    }

    function showBackgroundMenu(sceneX, sceneY) {
        rowMenu.entries = [
            {text: "Refresh", icon: Icons.refresh, shortcut: "F5", action: () => window.session.refresh()},
            {text: "Show Hidden Files", icon: Icons.eye, shortcut: "Ctrl+H", checked: SettingsController.showHidden,
             action: () => SettingsController.showHidden = !SettingsController.showHidden},
            {separator: true},
            {text: "Copy Path", icon: Icons.copy, action: () => window.session.copyPaths()},
        ]
        rowMenu.openAt(sceneX, sceneY)
    }

    function showSettingsMenu(sceneX, sceneY) {
        const mode = SettingsController.themeMode
        settingsMenu.entries = [
            {header: "Theme"},
            {text: "System", checked: mode === 0, action: () => SettingsController.themeMode = 0},
            {text: "Dark", checked: mode === 1, action: () => SettingsController.themeMode = 1},
            {text: "Light", checked: mode === 2, action: () => SettingsController.themeMode = 2},
            {separator: true},
            {text: "Show Hidden Files", icon: Icons.eye, shortcut: "Ctrl+H", checked: SettingsController.showHidden,
             action: () => SettingsController.showHidden = !SettingsController.showHidden},
            {text: "Show Window Buttons", checked: SettingsController.windowButtonsVisible,
             action: () => SettingsController.windowButtons = SettingsController.windowButtonsVisible ? 2 : 1},
        ]
        settingsMenu.openAt(sceneX - Theme.menuMinWidth - 2 * Theme.menuPadding, sceneY + 2)
    }

    width: Theme.windowWidth
    height: Theme.windowHeight
    minimumWidth: Theme.windowMinWidth
    minimumHeight: Theme.windowMinHeight
    visible: true
    flags: Qt.Window | Qt.FramelessWindowHint
    title: session ? session.title + " - ChaoticFiles" : "ChaoticFiles"
    color: Theme.windowBg

    onActiveFocusItemChanged: if (!activeFocusItem) Qt.callLater(focusFiles)
    Component.onCompleted: Qt.callLater(focusFiles)

    Connections {
        target: TabsController
        function onCurrentChanged() { Qt.callLater(window.focusFiles) }
        function onMessage(text) { statusBar.showMessage(text) }
    }

    // ---- shortcuts ----
    Shortcut { sequences: ["Ctrl+T"]; onActivated: TabsController.newTab() }
    Shortcut { sequences: ["Ctrl+W"]; onActivated: TabsController.closeCurrent() }
    Shortcut { sequences: ["Ctrl+Tab", "Ctrl+PgDown"]; onActivated: TabsController.nextTab() }
    Shortcut { sequences: ["Ctrl+Shift+Tab", "Ctrl+PgUp"]; onActivated: TabsController.previousTab() }
    Shortcut { sequences: ["Ctrl+Shift+T"]; onActivated: TabsController.reopenTab() }
    Shortcut { sequences: ["Ctrl+L", "Alt+D"]; onActivated: navigationBar.addressBar.startEditing() }
    Shortcut { sequences: ["Alt+Left"]; onActivated: window.session.goBack() }
    Shortcut { sequences: ["Alt+Right"]; onActivated: window.session.goForward() }
    Shortcut { sequences: ["Alt+Up"]; onActivated: window.session.goUp() }
    Shortcut { sequences: ["F5", "Ctrl+R"]; onActivated: window.session.refresh() }
    Shortcut { sequences: ["Ctrl+H"]; onActivated: SettingsController.showHidden = !SettingsController.showHidden }
    Shortcut { sequences: ["Ctrl+Q"]; onActivated: Qt.quit() }

    // the side buttons of a mouse
    TapHandler {
        acceptedButtons: Qt.BackButton | Qt.ForwardButton
        onTapped: (point, button) => {
            if (button === Qt.BackButton) window.session.goBack()
            else window.session.goForward()
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        BrowserTabBar {
            Layout.fillWidth: true
            targetWindow: window
        }

        NavigationBar {
            id: navigationBar
            Layout.fillWidth: true
            session: window.session
            onSettingsRequested: (sceneX, sceneY) => window.showSettingsMenu(sceneX, sceneY)
            onAddressFinished: window.focusFiles()
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.divider
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            Sidebar {
                Layout.fillHeight: true
                session: window.session
            }

            Rectangle {
                Layout.fillHeight: true
                implicitWidth: 1
                color: Theme.divider
            }

            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                Repeater {
                    id: fileViews
                    model: TabsController
                    delegate: FileView {
                        required property bool isCurrent
                        anchors.fill: parent
                        visible: isCurrent
                        onRowMenuRequested: (row, isDirectory, path, sceneX, sceneY) => {
                            window.showRowMenu(isDirectory, path, sceneX, sceneY)
                        }
                        onBackgroundMenuRequested: (sceneX, sceneY) => window.showBackgroundMenu(sceneX, sceneY)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.divider
        }

        StatusBar {
            id: statusBar
            Layout.fillWidth: true
            session: window.session
        }
    }

    ResizeHandles {
        anchors.fill: parent
        targetWindow: window
        visible: window.visibility === Window.Windowed
    }

    PopupMenu {
        id: rowMenu
        onClosed: window.focusFiles()
    }
    PopupMenu {
        id: settingsMenu
        onClosed: window.focusFiles()
    }
}
