import QtQuick
import QtQuick.Shapes

// One tab, in the Windows 11 shape: rounded at the top, and the current tab flares out at the bottom
// and flows into the toolbar below it. It only draws and reports clicks.
Item {
    id: root

    property string title: ""
    property bool current: false
    property bool showDivider: false       // the thin line at the right end of a tab that is not selected

    signal clicked()
    signal closeRequested()
    signal dragStarted(real sceneX)
    signal dragMoved(real sceneX)
    signal dragEnded()
    signal urlsDropped(var urls)

    readonly property bool hovered: tabArea.containsMouse || closeArea.containsMouse

    // ---- background ----
    Rectangle {
        anchors.fill: parent
        topLeftRadius: Theme.tabRadius
        topRightRadius: Theme.tabRadius
        color: root.current ? Theme.toolbarBg : root.hovered ? Theme.hoverBg : "transparent"
    }

    // ---- the two small curves at the bottom corners of the current tab ----
    component Flare: Shape {
        width: Theme.tabRadius
        height: Theme.tabRadius
        visible: root.current
        anchors.bottom: parent.bottom
        preferredRendererType: Shape.CurveRenderer
        ShapePath {
            fillColor: Theme.toolbarBg
            strokeWidth: -1
            startX: Theme.tabRadius; startY: Theme.tabRadius
            PathLine { x: 0; y: Theme.tabRadius }
            PathArc {
                x: Theme.tabRadius; y: 0
                radiusX: Theme.tabRadius; radiusY: Theme.tabRadius
                direction: PathArc.Counterclockwise
            }
        }
    }
    Flare { anchors.right: parent.left }
    Flare {
        anchors.left: parent.right
        transform: Scale { xScale: -1; origin.x: Theme.tabRadius / 2 }
    }

    Rectangle {
        visible: root.showDivider
        anchors.right: parent.right
        anchors.rightMargin: -Theme.tabGap / 2 - 1
        anchors.verticalCenter: parent.verticalCenter
        width: 1
        height: parent.height / 2
        color: Theme.divider
    }

    // files dragged onto a tab go into its folder
    DropArea {
        anchors.fill: parent
        onEntered: (drag) => { drag.accepted = drag.hasUrls }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            root.urlsDropped(drop.urls)
        }
    }

    // ---- click: select. Middle click: close. Drag: reorder ----
    MouseArea {
        id: tabArea
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton

        property real pressSceneX: 0
        property bool dragging: false

        onPressed: (mouse) => {
            pressSceneX = mapToItem(null, mouse.x, 0).x
            dragging = false
            if (mouse.button === Qt.LeftButton) root.clicked()
        }
        onClicked: (mouse) => {
            if (mouse.button === Qt.MiddleButton) root.closeRequested()
        }
        onPositionChanged: (mouse) => {
            if (!pressed || !(pressedButtons & Qt.LeftButton)) return
            const sceneX = mapToItem(null, mouse.x, 0).x
            if (!dragging && Math.abs(sceneX - pressSceneX) > 6) {
                dragging = true
                root.dragStarted(pressSceneX)
            }
            if (dragging) root.dragMoved(sceneX)
        }
        onReleased: { if (dragging) { dragging = false; root.dragEnded() } }
        onCanceled: { if (dragging) { dragging = false; root.dragEnded() } }
    }

    // ---- icon and title ----
    Text {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: Theme.tabPadding
        anchors.verticalCenter: parent.verticalCenter
        text: Icons.folder
        font.family: Icons.fontFamily
        font.pixelSize: Theme.iconSize
        color: Theme.folderIcon
    }
    Text {
        anchors.left: icon.right
        anchors.leftMargin: 8
        anchors.right: closeButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        elide: Text.ElideRight
        font.pixelSize: Theme.fontSize
        color: root.current ? Theme.text : Theme.textMuted
    }

    // ---- close button ----
    Rectangle {
        id: closeButton
        anchors.right: parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.tabCloseSize
        height: Theme.tabCloseSize
        radius: Theme.controlRadius
        visible: root.current || root.hovered
        color: closeArea.containsMouse ? Theme.hoverBg : "transparent"

        Text {
            anchors.centerIn: parent
            text: Icons.close
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize - 1
            color: closeArea.containsMouse ? Theme.text : Theme.textMuted
        }
        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.closeRequested()
        }
    }
}
