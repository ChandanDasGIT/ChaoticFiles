import QtQuick
import QtQuick.Controls.Basic

// The file list of one tab. Each tab has its own, so scrolling stays where it was when you come back.
FocusScope {
    id: root

    required property var session
    signal rowMenuRequested(int row, bool isDirectory, string path, string name, real sceneX, real sceneY)
    signal backgroundMenuRequested(real sceneX, real sceneY)

    readonly property int pageRows: Math.max(1, Math.floor(list.height / Theme.rowHeight) - 1)
    readonly property int keyboardModifiers: Qt.ControlModifier | Qt.AltModifier | Qt.MetaModifier

    function keepCurrentInView() {
        if (session.currentRow >= 0) list.positionViewAtIndex(session.currentRow, ListView.Contain)
    }

    function scrollTo(y) {
        const lowest = list.originY + Math.max(0, list.contentHeight - list.height)
        list.contentY = Math.max(list.originY, Math.min(lowest, y))
    }

    Connections {
        target: root.session
        property real savedY: 0
        function onAboutToRefresh() { savedY = list.contentY }
        function onRefreshed() { root.scrollTo(savedY); Qt.callLater(root.keepCurrentInView) }
        function onSelectionChanged() { Qt.callLater(root.keepCurrentInView) }
        function onLocationChanged() { list.contentY = list.originY }
    }

    // ---- the keyboard ----
    Keys.onPressed: (event) => {
        const shift = (event.modifiers & Qt.ShiftModifier) !== 0
        const plain = (event.modifiers & keyboardModifiers) === 0
        switch (event.key) {
        case Qt.Key_Up:       session.moveCurrent(-1, shift); break
        case Qt.Key_Down:     session.moveCurrent(1, shift); break
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
        width: parent.width
        inTrash: root.session.isTrash
        sortKey: root.session.sortKey
        sortAscending: root.session.sortAscending
        onSortRequested: (key) => root.session.sortBy(key)
    }

    ListView {
        id: list
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.topMargin: 2
        clip: true
        interactive: false                 // the mouse selects here; the wheel and the scrollbar scroll
        boundsBehavior: Flickable.StopAtBounds
        model: root.session.model
        cacheBuffer: 0

        delegate: FileRow {
            onPressedWith: (button, modifiers, sceneX, sceneY) => {
                root.forceActiveFocus()
                if (button === Qt.LeftButton) {
                    // a press on a selected row may start dragging all of them, so it waits for the release
                    wasSelected = isSelected && modifiers === 0
                    if (!wasSelected) root.session.click(index, modifiers)
                    dragText = root.session.selectedUrlText()
                } else if (button === Qt.RightButton) {
                    root.session.contextClick(index)
                    root.rowMenuRequested(index, isDirectory, path, name, sceneX, sceneY)
                } else if (isDirectory) {
                    TabsController.openInBackground(path)
                }
            }
            property bool wasSelected: false
            onReleasedWithoutDrag: (modifiers) => { if (wasSelected) root.session.click(index, modifiers) }
            onDoubleClicked: root.session.openRow(index)
            onUrlsDropped: (urls) => OperationsController.dropUrls(urls, path)
        }

        // the space under the last row
        MouseArea {
            anchors.fill: parent
            z: -1
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onPressed: (mouse) => {
                root.forceActiveFocus()
                root.session.clearSelection()
                if (mouse.button === Qt.RightButton) {
                    const scene = mapToItem(null, mouse.x, mouse.y)
                    root.backgroundMenuRequested(scene.x, scene.y)
                }
            }
        }

        // files dragged onto the empty part go into this folder
        DropArea {
            anchors.fill: parent
            z: -2
            onEntered: (drag) => { drag.accepted = drag.hasUrls && root.session.canWrite }
            onDropped: (drop) => {
                drop.accept(Qt.CopyAction)
                OperationsController.dropUrls(drop.urls, root.session.path)
            }
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                const step = event.pixelDelta.y !== 0 ? event.pixelDelta.y
                                                      : event.angleDelta.y / 120 * Theme.wheelRows * Theme.rowHeight
                root.scrollTo(list.contentY - step)
            }
        }

        ScrollBar.vertical: ThinScrollBar {}
    }

    // ---- empty folder, or a folder that can't be opened ----
    Column {
        anchors.centerIn: list
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
            width: Math.min(implicitWidth, root.width - 40)
            anchors.horizontalCenter: parent.horizontalCenter
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.Wrap
            text: root.session.error !== "" ? root.session.error
                : root.session.isTrash ? "The Trash is empty." : "This folder is empty."
            font.pixelSize: Theme.fontSize
            color: Theme.textMuted
        }
    }
}
