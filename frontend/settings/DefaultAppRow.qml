import QtQuick

// One file type in the Default Apps panel: its name, and a button showing the app that opens it.
Item {
    id: root

    property var type: ({})
    signal clicked()

    implicitHeight: Theme.popupRowHeight + 4

    Text {
        anchors.left: parent.left
        anchors.right: box.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        text: root.type.label
        elide: Text.ElideRight
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
    Rectangle {
        id: box
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.defaultAppBoxWidth
        height: Theme.popupRowHeight
        radius: Theme.controlRadius
        color: area.containsMouse ? Theme.hoverBg : Theme.dialogFieldBg
        border.color: Theme.menuBorder

        AppIcon {
            id: appIcon
            visible: root.type.appId !== ""
            anchors.left: parent.left
            anchors.leftMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            name: root.type.appIcon
        }
        Text {
            anchors.left: appIcon.visible ? appIcon.right : parent.left
            anchors.leftMargin: 8
            anchors.right: arrow.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            text: root.type.appId !== "" ? root.type.appName : "Not set"
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.pixelSize: Theme.smallFontSize
            color: root.type.appId !== "" ? Theme.text : Theme.textDim
        }
        Text {
            id: arrow
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: Icons.chevronDown
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize - 3
            color: Theme.textMuted
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
        }
    }
}
