import QtQuick

// One place in the sidebar.
Rectangle {
    id: root

    property string name: ""
    property string icon: ""
    property bool active: false
    signal clicked()
    signal middleClicked()

    implicitHeight: Theme.sidebarRowHeight
    radius: Theme.controlRadius
    color: active ? Theme.selectionBg : area.containsMouse ? Theme.hoverBg : "transparent"

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
        color: root.active ? Theme.accent : Theme.textMuted
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
        color: Theme.text
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton
        onClicked: (mouse) => {
            if (mouse.button === Qt.MiddleButton) root.middleClicked()
            else root.clicked()
        }
    }
}
