import QtQuick

// A square button with one icon.
Rectangle {
    id: root

    property string icon: ""
    property bool active: true
    signal clicked()

    implicitWidth: Theme.toolbarButtonSize
    implicitHeight: Theme.toolbarButtonSize
    radius: Theme.controlRadius
    color: !active || !area.containsMouse ? "transparent" : area.pressed ? Theme.pressedBg : Theme.hoverBg

    Text {
        anchors.centerIn: parent
        text: root.icon
        font.family: Icons.fontFamily
        font.pixelSize: Theme.iconSize
        color: root.active ? Theme.text : Theme.textDisabled
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.active
        onClicked: root.clicked()
    }
}
