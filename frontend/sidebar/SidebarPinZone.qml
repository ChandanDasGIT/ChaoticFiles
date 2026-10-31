import QtQuick

// The heading of Pinned Folders. Folders dropped here are pinned, and stay where they are on the disk.
// The heading itself asks for the drop during a drag, so nothing below it moves.
Rectangle {
    id: root

    signal urlsDropped(var urls)

    readonly property bool asking: DragState.active && DragState.hasFolders

    implicitHeight: Theme.sidebarHeaderHeight
    radius: Theme.controlRadius
    color: dropArea.containsDrag ? Theme.selectionBg : "transparent"

    Text {
        height: parent.height
        leftPadding: Theme.sidebarPadding
        verticalAlignment: Text.AlignBottom
        bottomPadding: 6
        text: root.asking ? "Drop here to pin" : "Pinned Folders"
        font.pixelSize: Theme.smallFontSize
        color: root.asking ? Theme.accent : Theme.textDim
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
