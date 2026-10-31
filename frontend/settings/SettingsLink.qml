import QtQuick

// A line in the settings that opens its own panel.
Item {
    id: root

    property string title: ""
    property string buttonText: "Open"
    signal clicked()

    implicitHeight: Theme.settingsRowHeight

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
    DialogButton {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: root.buttonText
        onClicked: root.clicked()
    }
}
