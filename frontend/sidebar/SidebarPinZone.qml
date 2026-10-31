import QtQuick

// The heading of Pinned Folders. Folders dropped here are pinned, and stay where they are on the disk.
// A row appears under it during a drag, so there is a clear place to drop.
Rectangle {
    id: root

    signal urlsDropped(var urls)

    implicitHeight: column.implicitHeight
    radius: Theme.controlRadius
    color: dropArea.containsDrag ? Theme.selectionBg : "transparent"

    Column {
        id: column
        width: parent.width

        Text {
            height: Theme.sidebarHeaderHeight
            leftPadding: Theme.sidebarPadding
            verticalAlignment: Text.AlignBottom
            bottomPadding: 6
            text: "Pinned Folders"
            font.pixelSize: Theme.smallFontSize
            color: Theme.textDim
        }
        SidebarItem {
            visible: DragState.active && DragState.hasFolders
            width: parent.width
            name: "Drop here to pin"
            icon: Icons.pin
            droppable: false
        }
    }
    DropArea {
        id: dropArea
        anchors.fill: parent
        onEntered: (drag) => { drag.accepted = drag.hasUrls }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            root.urlsDropped(drop.urls)
        }
    }
}
