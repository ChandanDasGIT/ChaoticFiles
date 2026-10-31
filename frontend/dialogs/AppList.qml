import QtQuick
import QtQuick.Controls.Basic

// A list of apps to choose from, each with its icon. The current default is marked.
ListView {
    id: root

    property string defaultApp: ""
    signal chosen(string appId)

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ThinScrollBar {}

    delegate: Rectangle {
        required property var modelData
        width: ListView.view.width
        height: Theme.popupRowHeight
        radius: Theme.controlRadius
        color: area.containsMouse ? Theme.hoverBg : "transparent"

        AppIcon {
            id: appIcon
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            name: parent.modelData.icon
        }
        Text {
            anchors.left: appIcon.right
            anchors.leftMargin: 8
            anchors.right: defaultMark.visible ? defaultMark.left : parent.right
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: parent.modelData.name
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSize
            color: Theme.text
        }
        Text {
            id: defaultMark
            anchors.right: parent.right
            anchors.rightMargin: 12
            anchors.verticalCenter: parent.verticalCenter
            visible: parent.modelData.id === root.defaultApp
            text: "Default"
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.chosen(parent.modelData.id)
        }
    }
}
