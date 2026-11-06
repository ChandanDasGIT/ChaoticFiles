import QtQuick

// One place in the sidebar. A pinned folder can also be dragged to a new spot, and takes folders dropped
// at its top or bottom edge, which go between the rows.
Rectangle {
    id: root

    property string name: ""
    property string icon: ""
    property color iconColor: Theme.textMuted
    property bool active: false
    property bool dimmed: false            // a drive that is not mounted yet
    property bool droppable: true
    property bool pinned: false
    property string path: ""
    signal clicked()
    signal middleClicked()
    signal menuRequested(real sceneX, real sceneY)
    signal urlsDropped(var urls)
    signal pinDropped(var urls, bool after)      // folders dropped between two pinned rows
    signal pinMoved(string path, bool after)     // a pinned row dropped on this one

    property int insertSide: 0                   // -1 above this row, 1 below it, while a drop would go there
    property bool draggingPin: false

    function updateInsertSide(drag) {
        if (drag.keys.includes(DragState.pinKey))
            insertSide = drag.y < height / 2 ? -1 : 1
        else if (root.pinned && (!DragState.active || DragState.hasFolders))
            insertSide = drag.y < Theme.pinInsertEdge ? -1 : drag.y > height - Theme.pinInsertEdge ? 1 : 0
    }

    implicitHeight: Theme.sidebarRowHeight
    radius: Theme.controlRadius
    z: draggingPin ? 1 : 0
    color: active || (dropArea.containsDrag && insertSide === 0) ? Theme.selectionBg
         : area.containsMouse ? Theme.hoverBg : "transparent"

    Text {
        id: iconText
        anchors.left: parent.left
        anchors.leftMargin: Theme.sidebarPadding
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.rowIconWidth - 4
        horizontalAlignment: Text.AlignHCenter
        text: root.icon
        font.family: Icons.fontFamily
        font.pixelSize: Theme.iconSize
        color: root.active ? Theme.accent : root.iconColor
    }
    Text {
        textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
        anchors.left: iconText.right
        anchors.leftMargin: 6
        anchors.right: parent.right
        anchors.rightMargin: Theme.sidebarPadding
        anchors.verticalCenter: parent.verticalCenter
        text: root.name
        elide: Text.ElideRight
        font.pixelSize: Theme.fontSize
        color: root.dimmed ? Theme.textMuted : Theme.text
    }
    DropArea {
        id: dropArea
        anchors.fill: parent
        enabled: root.droppable
        onEntered: (drag) => {
            drag.accepted = drag.hasUrls || (root.pinned && drag.keys.includes(DragState.pinKey))
            root.updateInsertSide(drag)
        }
        onPositionChanged: (drag) => root.updateInsertSide(drag)
        onExited: root.insertSide = 0
        onDropped: (drop) => {
            const after = root.insertSide > 0
            const between = root.insertSide !== 0
            root.insertSide = 0
            if (drop.keys.includes(DragState.pinKey)) {
                drop.accept(Qt.MoveAction)
                root.pinMoved(DragState.pinPath, after)
                return
            }
            drop.accept(Qt.CopyAction)
            if (between) root.pinDropped(drop.urls, after)
            else root.urlsDropped(drop.urls)
        }
    }
    Rectangle {
        visible: root.insertSide !== 0
        width: parent.width
        height: Theme.pinInsertLine
        y: root.insertSide < 0 ? 0 : parent.height - height
        color: Theme.accent
    }
    // the row as it follows the mouse
    Rectangle {
        id: pinGhost
        visible: root.draggingPin
        width: root.width
        height: root.height
        radius: Theme.controlRadius
        color: Theme.selectionBg
        opacity: Theme.pinDragOpacity
        Drag.active: root.draggingPin
        Drag.keys: [DragState.pinKey]
        Drag.source: root
        Drag.hotSpot.x: width / 2
        Drag.hotSpot.y: height / 2

        Text {
            textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
            x: Theme.sidebarPadding
            anchors.verticalCenter: parent.verticalCenter
            text: root.name
            font.pixelSize: Theme.fontSize
            color: Theme.text
        }
    }
    Connections {
        target: area.drag
        enabled: root.pinned
        function onActiveChanged() {
            if (!area.drag.active) return
            DragState.pinPath = root.path
            root.draggingPin = true
        }
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
        preventStealing: root.pinned               // the sidebar would take the drag to scroll
        drag.target: root.pinned && (pressedButtons & Qt.LeftButton) ? pinGhost : null
        drag.axis: Drag.YAxis
        drag.threshold: Theme.pinInsertEdge
        drag.smoothed: false                       // the row stays under the mouse, so it lands where it is aimed
        onReleased: {
            if (root.draggingPin) pinGhost.Drag.drop()
            root.draggingPin = false
            pinGhost.x = 0
            pinGhost.y = 0
        }
        onClicked: (mouse) => {
            if (mouse.button === Qt.MiddleButton) {
                root.middleClicked()
            } else if (mouse.button === Qt.RightButton) {
                const scene = mapToItem(null, mouse.x, mouse.y)
                root.menuRequested(scene.x, scene.y)
            } else {
                root.clicked()
            }
        }
    }
}
