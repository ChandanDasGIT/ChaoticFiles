import QtQuick

// What a row and a grid cell share: the mouse, starting a drag, and taking a drop onto a folder.
Item {
    id: root

    property bool isDirectory: false
    property string dragText: ""           // file URLs, set when the mouse goes down
    readonly property bool containsMouse: area.containsMouse
    readonly property bool containsDrag: dropArea.containsDrag

    signal pressedWith(int button, int modifiers, real sceneX, real sceneY)
    signal releasedWithoutDrag(int modifiers)
    signal doubleClicked()
    signal urlsDropped(var urls)

    // what a drag carries: it stays still, and the system draws the drag
    Item {
        id: dragItem
        Drag.dragType: Drag.Automatic
        Drag.active: area.drag.active
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
        Drag.mimeData: ({"text/uri-list": root.dragText})
    }

    // files dragged onto a folder go into it
    DropArea {
        id: dropArea
        anchors.fill: parent
        enabled: root.isDirectory
        onEntered: (drag) => { drag.accepted = drag.hasUrls }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            root.urlsDropped(drop.urls)
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        drag.target: pressedButtons & Qt.LeftButton ? dragItem : null
        drag.threshold: 8
        onPressed: (mouse) => {
            const scene = mapToItem(null, mouse.x, mouse.y)
            root.pressedWith(mouse.button, mouse.modifiers, scene.x, scene.y)
        }
        onReleased: (mouse) => {
            if (mouse.button === Qt.LeftButton && !drag.active) root.releasedWithoutDrag(mouse.modifiers)
            dragItem.x = 0
            dragItem.y = 0
        }
        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) root.doubleClicked()
        }
    }
}
