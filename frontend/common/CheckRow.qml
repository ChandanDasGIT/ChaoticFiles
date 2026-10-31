import QtQuick

// A check box with its text. A click anywhere on it flips it.
Row {
    id: root

    property string text: ""
    property bool checked: false

    spacing: 8

    Rectangle {
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.checkBoxSize
        height: Theme.checkBoxSize
        radius: 3
        color: root.checked ? Theme.accentFill : "transparent"
        border.color: Theme.menuBorder

        Text {
            anchors.centerIn: parent
            visible: root.checked
            text: Icons.check
            font.family: Icons.fontFamily
            font.pixelSize: 10
            color: Theme.textOnAccent
        }
    }
    Text {
        text: root.text
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
    TapHandler { onTapped: root.checked = !root.checked }
}
