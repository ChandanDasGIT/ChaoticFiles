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

    function key(id) { return ShortcutsController.label(id) }

    function showRowMenu(isDirectory, path, name, sceneX, sceneY) {
        const count = window.session.selectedItems().length
        const a = actions
        let entries = []
        if (window.session.isTrash) {
            entries = [
                {text: "Restore", icon: Icons.restoreItem, action: a.restore},
                {text: "Delete Permanently", icon: Icons.trash, action: a.deletePermanently},
                {separator: true},
                {text: "Properties", icon: Icons.info, shortcut: key("properties"), action: a.properties},
            ]
        } else {
            entries.push({text: "Open", icon: Icons.open, shortcut: "Enter", action: a.open})
            if (isDirectory) {
                entries.push({text: "Open in New Tab", icon: Icons.newTab, action: () => TabsController.openInBackground(path)})
                entries.push({text: "Change Icon…", icon: Icons.palette, action: () => a.changeFolderIcon([])})
                if (!PlacesController.isBookmarked(path))
                    entries.push({text: "Add to Bookmarks", icon: Icons.bookmark, action: () => PlacesController.addBookmark(path)})
            } else
                entries.push({text: "Open With…", icon: Icons.open, action: a.openWith})
            entries.push({separator: true})
            entries.push({text: "Cut", icon: Icons.cut, shortcut: key("cut"), action: a.cut})
            entries.push({text: "Copy", icon: Icons.copy, shortcut: key("copy"), action: a.copy})
            entries.push({text: "Rename…", icon: Icons.rename, shortcut: key("rename"), enabled: count === 1, action: a.rename})
            entries.push({text: "Create Link", icon: Icons.link, action: a.createLinks})
            entries.push({separator: true})
            entries.push({text: "Compress…", icon: Icons.archive, action: a.compress})
            if (OperationsController.isArchive(name))
                entries.push({text: "Extract Here", icon: Icons.extract, action: a.extract})
            entries.push({separator: true})
            entries.push({text: "Move to Trash", icon: Icons.trash, shortcut: key("trash"), action: a.moveToTrash})
            entries.push({text: "Delete Permanently…", icon: Icons.warning, shortcut: key("deleteForever"), action: a.deletePermanently})
            entries.push({separator: true})
            entries.push({text: "Copy Path", icon: Icons.copy, action: () => window.session.copyPaths()})
            entries.push({text: "Properties", icon: Icons.info, shortcut: key("properties"), action: a.properties})
        }
        rowMenu.entries = entries
        rowMenu.openAt(sceneX, sceneY)
    }

    function showBackgroundMenu(sceneX, sceneY) {
        const a = actions
        if (window.session.isTrash) {
            rowMenu.entries = [
                {text: "Empty Trash…", icon: Icons.trash, action: a.emptyTrash},
                {text: "Refresh", icon: Icons.refresh, shortcut: key("refresh"), action: () => window.session.refresh()},
            ]
        } else {
            rowMenu.entries = [
                {text: "New Folder…", icon: Icons.newFolder, shortcut: key("newFolder"), enabled: window.session.canWrite, action: a.newFolder},
                {text: "New File…", icon: Icons.newFile, enabled: window.session.canWrite, action: a.newFile},
                {separator: true},
                {text: "Paste", icon: Icons.paste, shortcut: key("paste"), enabled: ClipboardController.canPaste && window.session.canWrite, action: a.paste},
                {text: OperationsController.undoText, icon: Icons.undo, shortcut: key("undo"), enabled: OperationsController.canUndo, action: () => OperationsController.undo()},
                {separator: true},
                {text: "Refresh", icon: Icons.refresh, shortcut: key("refresh"), action: () => window.session.refresh()},
                {text: "Show Hidden Files", icon: Icons.eye, shortcut: key("showHidden"), checked: SettingsController.showHidden,
                 action: () => SettingsController.showHidden = !SettingsController.showHidden},
                {text: "Open Terminal Here", icon: Icons.terminal, enabled: window.session.canWrite, action: a.openTerminal},
                {separator: true},
                {text: "Copy Path", icon: Icons.copy, action: () => window.session.copyPaths()},
                {text: "Properties", icon: Icons.info, shortcut: key("properties"), action: a.properties},
            ]
        }
        rowMenu.openAt(sceneX, sceneY)
    }

    function showPlaceMenu(place, sceneX, sceneY) {
        const entries = []
        if (place.kind !== "drive" || place.mounted)
            entries.push({text: "Open in New Tab", icon: Icons.newTab, action: () => TabsController.openInBackground(place.path)})
        if (place.kind === "bookmark")
            entries.push({text: "Remove Bookmark", icon: Icons.close, action: () => PlacesController.removeBookmark(place.path)})
        if (place.kind === "bookmark")
            entries.push({text: "Change Icon…", icon: Icons.palette, action: () => actions.changeFolderIcon([place.path])})
        if (place.kind === "drive" && place.mounted)
            entries.push({text: "Unmount", icon: Icons.eject, action: () => PlacesController.unmount(place.device)})
        if (place.kind === "drive" && !place.mounted)
            entries.push({text: "Mount", icon: Icons.drive, action: () => PlacesController.mount(place.device)})
        if (place.kind === "network")
            entries.push({text: "Disconnect", icon: Icons.eject, action: () => PlacesController.disconnectShare(place.uri)})
        if (place.kind === "trash")
            entries.push({text: "Empty Trash…", icon: Icons.trash, action: actions.emptyTrash})
        if (entries.length === 0) return
        rowMenu.entries = entries
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
            {text: "Show Hidden Files", icon: Icons.eye, shortcut: key("showHidden"), checked: SettingsController.showHidden,
             action: () => SettingsController.showHidden = !SettingsController.showHidden},
            {text: "Connect to Server…", icon: Icons.network, action: actions.connectToServer},
            {separator: true},
            {text: "Settings…", icon: Icons.settings, shortcut: key("settings"), action: () => settingsDialog.open()},
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
    Connections {
        target: PlacesController
        function onMounted(path) { window.session.navigate(path) }
        function onProblem(title, text) { actions.showError(title, text) }
    }
    Connections {
        target: OperationsController
        function onMessage(text) { statusBar.showMessage(text) }
        function onOperationFinished() { window.session.refresh() }
    }

    // ---- shortcuts: the keys are in ShortcutsController, and can be changed in the settings ----
    ActionShortcut { name: "newTab"; onActivated: TabsController.newTab() }
    ActionShortcut { name: "closeTab"; onActivated: TabsController.closeCurrent() }
    ActionShortcut { name: "nextTab"; onActivated: TabsController.nextTab() }
    ActionShortcut { name: "previousTab"; onActivated: TabsController.previousTab() }
    ActionShortcut { name: "reopenTab"; onActivated: TabsController.reopenTab() }
    ActionShortcut { name: "address"; onActivated: navigationBar.addressBar.startEditing() }
    ActionShortcut { name: "back"; onActivated: window.session.goBack() }
    ActionShortcut { name: "forward"; onActivated: window.session.goForward() }
    ActionShortcut { name: "up"; onActivated: window.session.goUp() }
    ActionShortcut { name: "refresh"; onActivated: window.session.refresh() }
    ActionShortcut { name: "search"; onActivated: searchBar.start() }
    ActionShortcut { name: "showHidden"; onActivated: SettingsController.showHidden = !SettingsController.showHidden }
    ActionShortcut { name: "detailsView"; onActivated: SettingsController.viewMode = 0 }
    ActionShortcut { name: "gridView"; onActivated: SettingsController.viewMode = 1 }
    ActionShortcut { name: "preview"; onActivated: SettingsController.showPreview = !SettingsController.showPreview }
    ActionShortcut { name: "properties"; onActivated: actions.properties() }
    ActionShortcut { name: "copy"; onActivated: actions.copy() }
    ActionShortcut { name: "cut"; onActivated: actions.cut() }
    ActionShortcut { name: "paste"; onActivated: actions.paste() }
    ActionShortcut { name: "undo"; onActivated: OperationsController.undo() }
    ActionShortcut { name: "rename"; onActivated: actions.rename() }
    ActionShortcut { name: "trash"; onActivated: actions.moveToTrash() }
    ActionShortcut { name: "deleteForever"; onActivated: actions.deletePermanently() }
    ActionShortcut { name: "newFolder"; onActivated: actions.newFolder() }
    ActionShortcut { name: "terminal"; onActivated: actions.openTerminal() }
    ActionShortcut { name: "settings"; onActivated: settingsDialog.open() }
    ActionShortcut { name: "quit"; onActivated: Qt.quit() }

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

        SearchBar {
            id: searchBar
            Layout.fillWidth: true
            session: window.session
            onFinished: window.focusFiles()
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
                onPlaceMenuRequested: (place, sceneX, sceneY) => window.showPlaceMenu(place, sceneX, sceneY)
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
                        onRowMenuRequested: (row, isDirectory, path, name, sceneX, sceneY) => {
                            window.showRowMenu(isDirectory, path, name, sceneX, sceneY)
                        }
                        onBackgroundMenuRequested: (sceneX, sceneY) => window.showBackgroundMenu(sceneX, sceneY)
                    }
                }
            }

            PreviewPanel {
                Layout.fillHeight: true
                visible: SettingsController.showPreview
                session: window.session
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

    OperationsPanel {
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: Theme.toolbarMargin
        anchors.bottomMargin: Theme.statusBarHeight + Theme.toolbarMargin
    }

    FileActions {
        id: actions
        session: window.session
        onDialogClosed: window.focusFiles()
    }

    PropertiesDialog {
        onClosed: window.focusFiles()
    }

    SettingsDialog {
        id: settingsDialog
        onClosed: window.focusFiles()
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
