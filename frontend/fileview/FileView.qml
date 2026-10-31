import QtQuick
import QtQuick.Controls.Basic

// The file list of one tab. Each tab has its own, so scrolling stays where it was when you come back.
FocusScope {
    id: root

    required property var session
    signal rowMenuRequested(int row, bool isDirectory, string path, string name, real sceneX, real sceneY)
    signal backgroundMenuRequested(real sceneX, real sceneY)
    signal restoreAllRequested()
    signal deleteAllRequested()

    readonly property bool gridMode: SettingsController.viewMode === 1
    readonly property var view: gridMode ? grid : list
    readonly property int gridColumns: Math.max(1, Math.floor(grid.width / grid.cellWidth))
    readonly property int rowStep: gridMode ? gridColumns : 1
    readonly property int pageRows: gridMode ? Math.max(1, Math.floor(grid.height / grid.cellHeight)) * gridColumns
                                             : Math.max(1, Math.floor(list.height / Theme.rowHeight) - 1)
    readonly property int keyboardModifiers: Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier

    function keepCurrentInView() {
        if (session.currentRow >= 0) view.positionViewAtIndex(session.currentRow, gridMode ? GridView.Contain : ListView.Contain)
    }

    function scrollTo(y) {
        const lowest = view.originY + Math.max(0, view.contentHeight - view.height)
        view.contentY = Math.max(view.originY, Math.min(lowest, y))
    }

    Connections {
        target: root.session
        property real savedY: 0
        function onAboutToRefresh() { savedY = root.view.contentY }
        function onRefreshed() { root.scrollTo(savedY); Qt.callLater(root.keepCurrentInView) }
        function onSelectionChanged() { Qt.callLater(root.keepCurrentInView) }
        function onLocationChanged() { root.view.contentY = root.view.originY }
    }

    // ---- the keyboard ----
    Keys.onPressed: (event) => {
        const shift = (event.modifiers & Qt.ShiftModifier) !== 0
        const plain = (event.modifiers & keyboardModifiers) === 0
        switch (event.key) {
        case Qt.Key_Up:       session.moveCurrent(-rowStep, shift); break
        case Qt.Key_Down:     session.moveCurrent(rowStep, shift); break
        case Qt.Key_Left:     if (gridMode) session.moveCurrent(-1, shift); else return; break
        case Qt.Key_Right:    if (gridMode) session.moveCurrent(1, shift); else return; break
        case Qt.Key_PageUp:   session.moveCurrent(-pageRows, shift); break
        case Qt.Key_PageDown: session.moveCurrent(pageRows, shift); break
        case Qt.Key_Home:     session.moveCurrentTo(0, shift); break
        case Qt.Key_End:      session.moveCurrentTo(session.itemCount - 1, shift); break
        case Qt.Key_Return:
        case Qt.Key_Enter:    session.openCurrent(); break
        case Qt.Key_Backspace: session.goUp(); break
        case Qt.Key_Escape:   session.clearSelection(); break
        default:
            if (event.matches(StandardKey.SelectAll)) session.selectAll()
            else if (plain && event.text.length > 0 && event.text.charCodeAt(0) > 32) session.typeAhead(event.text)
            else return
        }
        event.accepted = true
    }

    ColumnHeader {
        id: header
        visible: !root.gridMode
        width: parent.width
        kind: root.session.headerKind
        sortKey: root.session.sortKey
        sortAscending: root.session.sortAscending
        onSortRequested: (key) => root.session.sortBy(key)
    }

    // ---- what a row and a grid cell do with the mouse ----
    function pressed(item, button, modifiers, sceneX, sceneY) {
        root.forceActiveFocus()
        if (button === Qt.LeftButton) {
            // a press on a selected item may start dragging all of them, so it waits for the release
            item.wasSelected = item.isSelected && modifiers === 0
            if (!item.wasSelected) session.click(item.index, modifiers)
            item.dragText = session.selectedUrlText()
        } else if (button === Qt.RightButton) {
            session.contextClick(item.index)
            rowMenuRequested(item.index, item.isDirectory, item.path, item.name, sceneX, sceneY)
        } else if (item.isDirectory) {
            TabsController.openInBackground(item.path)
        }
    }

    function wheel(event) {
        if (event.modifiers & Qt.ControlModifier) {
            SettingsController.zoomGrid(event.angleDelta.y > 0 ? 1 : -1)
            return
        }
        const step = event.pixelDelta.y !== 0 ? event.pixelDelta.y
                                              : event.angleDelta.y / 120 * Theme.wheelRows * Theme.rowHeight
        scrollTo(view.contentY - step)
    }

    ListView {
        id: list
        visible: !root.gridMode
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: trashBar.top
        anchors.topMargin: 2
        clip: true
        interactive: false                 // the mouse selects here; the wheel and the scrollbar scroll
        boundsBehavior: Flickable.StopAtBounds
        model: root.gridMode ? null : root.session.model
        cacheBuffer: 0

        delegate: FileRow {
            id: row
            property bool wasSelected: false
            onPressedWith: (button, modifiers, sceneX, sceneY) => root.pressed(row, button, modifiers, sceneX, sceneY)
            onReleasedWithoutDrag: (modifiers) => { if (wasSelected) root.session.click(index, modifiers) }
            onDoubleClicked: root.session.openRow(index)
            onUrlsDropped: (urls) => OperationsController.dropUrls(urls, path)
        }

        ViewBackground {
            session: root.session
            view: list
            rowsIn: (left, top, right, bottom) => {
                const rows = []
                const last = Math.min(root.session.itemCount - 1, Math.floor(bottom / Theme.rowHeight))
                for (let row = Math.floor(top / Theme.rowHeight); row <= last; ++row) rows.push(row)
                return rows
            }
            onMenuRequested: (sceneX, sceneY) => root.backgroundMenuRequested(sceneX, sceneY)
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.KeyboardModifierMask
            onWheel: (event) => root.wheel(event)
        }

        ScrollBar.vertical: ThinScrollBar {}
    }

    GridView {
        id: grid
        visible: root.gridMode
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: trashBar.top
        anchors.margins: Theme.rowMargin
        clip: true
        interactive: false
        boundsBehavior: Flickable.StopAtBounds
        model: root.gridMode ? root.session.model : null
        cacheBuffer: 0
        cellWidth: Math.max(SettingsController.gridSize + 2 * Theme.gridPadding, width / Math.max(1, Math.floor(width / (SettingsController.gridSize + 2 * Theme.gridPadding))))
        cellHeight: SettingsController.gridSize + Theme.gridTextHeight

        delegate: FileCell {
            id: cell
            property bool wasSelected: false
            onPressedWith: (button, modifiers, sceneX, sceneY) => root.pressed(cell, button, modifiers, sceneX, sceneY)
            onReleasedWithoutDrag: (modifiers) => { if (wasSelected) root.session.click(index, modifiers) }
            onDoubleClicked: root.session.openRow(index)
            onUrlsDropped: (urls) => OperationsController.dropUrls(urls, path)
        }

        ViewBackground {
            session: root.session
            view: grid
            rowsIn: (left, top, right, bottom) => {
                const rows = []
                const columns = root.gridColumns
                for (let y = Math.floor(top / grid.cellHeight); y <= Math.floor(bottom / grid.cellHeight); ++y)
                    for (let x = Math.floor(left / grid.cellWidth); x <= Math.floor(right / grid.cellWidth) && x < columns; ++x) {
                        const row = y * columns + x
                        if (row < root.session.itemCount) rows.push(row)
                    }
                return rows
            }
            onMenuRequested: (sceneX, sceneY) => root.backgroundMenuRequested(sceneX, sceneY)
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            acceptedModifiers: Qt.KeyboardModifierMask
            onWheel: (event) => root.wheel(event)
        }

        ScrollBar.vertical: ThinScrollBar {}
    }

    // ---- empty folder, or a folder that can't be opened ----
    Column {
        anchors.centerIn: parent
        spacing: 10
        visible: root.session.itemCount === 0 && !root.session.loading

        Text {
            visible: root.session.error !== ""
            anchors.horizontalCenter: parent.horizontalCenter
            text: Icons.lock
            font.family: Icons.fontFamily
            font.pixelSize: 28
            color: Theme.textDim
        }
        Text {
    // ---- the buttons at the bottom of the Trash ----
    Item {
        id: trashBar
        visible: root.session.isTrash
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: visible ? Theme.dialogButtonHeight + 2 * Theme.toolbarMargin : 0

        Row {
            anchors.right: parent.right
            anchors.rightMargin: Theme.toolbarMargin
            anchors.verticalCenter: parent.verticalCenter
            spacing: 8

            DialogButton {
                text: "Restore All"
                active: root.session.itemCount > 0
                onClicked: root.restoreAllRequested()
            }
            DialogButton {
                text: "Delete All"
                danger: true
                active: root.session.itemCount > 0
                onClicked: root.deleteAllRequested()
            }
        }
    }

            width: Math.min(implicitWidth, root.width - 40)
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.session.error !== "" ? root.session.error
                : root.session.isSearching ? "Nothing found."
                : root.session.isTrash ? "The Trash is empty." : "This folder is empty."
            font.pixelSize: Theme.fontSize
            color: Theme.textMuted
        }
    }
}
