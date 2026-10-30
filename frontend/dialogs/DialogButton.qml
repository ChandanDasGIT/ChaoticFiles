import QtQuick

// A button with a word on it, for dialogs. `primary` fills it with the accent colour, `danger` with red.
Rectangle {
    id: root

    property string text: ""
    property bool primary: false
    property bool danger: false
    property bool active: true
    signal clicked()

    implicitWidth: Math.max(Theme.dialogButtonMinWidth, label.implicitWidth + 2 * Theme.dialogButtonPadding)
    implicitHeight: Theme.dialogButtonHeight
    radius: Theme.controlRadius
    opacity: active ? 1 : 0.5
    color: danger ? Theme.dangerFill : primary ? Theme.accentFill : area.containsMouse ? Theme.hoverBg : Theme.dialogFieldBg
    border.color: primary || danger ? "transparent" : Theme.menuBorder

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        font.pixelSize: Theme.fontSize
        color: root.primary || root.danger ? Theme.textOnAccent : Theme.text
    }
    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.active
        onClicked: root.clicked()
    }
}
