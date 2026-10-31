import QtQuick

// One place in the sidebar.
Rectangle {
    id: root

    property string name: ""
    property string icon: ""
    property color iconColor: Theme.textMuted
    property bool active: false
    property bool dimmed: false            // a drive that is not mounted yet
    property bool droppable: true
    signal clicked()
    signal middleClicked()
    signal menuRequested(real sceneX, real sceneY)
    signal urlsDropped(var urls)

    implicitHeight: Theme.sidebarRowHeight
    radius: Theme.controlRadius
    color: active || dropArea.containsDrag ? Theme.selectionBg : area.containsMouse ? Theme.hoverBg : "transparent"

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
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton
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
