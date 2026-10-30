import QtQuick

// The empty part of the list or grid: a click clears the selection, a drag selects with a rubber band,
// and files dropped here go into this folder. `rowsIn` turns a rectangle of the content into rows.
Item {
    id: root

    required property var session
    required property var view
    property var rowsIn: function (left, top, right, bottom) { return [] }
    signal menuRequested(real sceneX, real sceneY)

    anchors.fill: parent
    z: -1

    DropArea {
        anchors.fill: parent
        onEntered: (drag) => { drag.accepted = drag.hasUrls && root.session.canWrite }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            OperationsController.dropUrls(drop.urls, root.session.path)
        }
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        property point start
        property real startY: 0            // in the content, so scrolling doesn't move the corner
        property bool banding: false
        property bool adding: false

        onPressed: (mouse) => {
            root.view.parent.forceActiveFocus()
            adding = (mouse.modifiers & Qt.ControlModifier) !== 0
            if (!adding) root.session.clearSelection()
            if (mouse.button === Qt.RightButton) {
                const scene = mapToItem(null, mouse.x, mouse.y)
                root.menuRequested(scene.x, scene.y)
                return
            }
            start = Qt.point(mouse.x, mouse.y)
            startY = mouse.y + root.view.contentY
            banding = false
        }
        onPositionChanged: (mouse) => {
            if (!pressed || !(pressedButtons & Qt.LeftButton)) return
            const currentY = mouse.y + root.view.contentY
            if (!banding && Math.abs(mouse.x - start.x) + Math.abs(mouse.y - start.y) < 6) return
            banding = true
            const left = Math.min(start.x, mouse.x), right = Math.max(start.x, mouse.x)
            const top = Math.min(startY, currentY), bottom = Math.max(startY, currentY)
            band.x = left
            band.y = top - root.view.contentY
            band.width = right - left
            band.height = bottom - top
            band.visible = true
            root.session.selectRows(root.rowsIn(left, top, right, bottom), adding)
        }
        onReleased: band.visible = false
        onCanceled: band.visible = false
    }

    Rectangle {
        id: band
        visible: false
        z: 10
        parent: root.view
        color: Theme.bandFill
        border.color: Theme.accent
        radius: 2
    }
}
