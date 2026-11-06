import QtQuick
import QtQuick.Controls.Basic

// The list of folders that opens under the address bar. `folders` is a list of {label, path}.
// `highlighted` marks one row (the address bar moves it with the arrow keys while typing).
Popup {
    id: root

    property var folders: []
    property int highlighted: -1
    signal chosen(string path)

    parent: Overlay.overlay
    padding: Theme.menuPadding
    margins: Theme.menuPadding
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    implicitWidth: Theme.popupWidth
    implicitHeight: Math.min(folders.length, Theme.popupMaxRows) * Theme.popupRowHeight + 2 * Theme.menuPadding

    background: Rectangle {
        color: Theme.menuBg
        border.color: Theme.menuBorder
        radius: Theme.menuRadius
    }

    contentItem: ListView {
        id: list
        clip: true
        model: root.folders
        boundsBehavior: Flickable.StopAtBounds
        currentIndex: root.highlighted

        delegate: Rectangle {
            id: row
            required property var modelData
            required property int index

            width: ListView.view.width
            height: Theme.popupRowHeight
            radius: Theme.controlRadius
            color: index === root.highlighted || area.containsMouse ? Theme.hoverBg : "transparent"

            Text {
                id: icon
                anchors.left: parent.left
                anchors.leftMargin: Theme.menuRowPadding
                anchors.verticalCenter: parent.verticalCenter
                text: Icons.folder
                font.family: Icons.fontFamily
                font.pixelSize: Theme.iconSize
                color: Theme.folderIcon
            }
            Text {
                textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
                anchors.left: icon.right
                anchors.leftMargin: Theme.menuRowPadding
                anchors.right: parent.right
                anchors.rightMargin: Theme.menuRowPadding
                anchors.verticalCenter: parent.verticalCenter
                text: row.modelData.label
                elide: Text.ElideMiddle
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    root.close()
                    root.chosen(row.modelData.path)
                }
            }
        }
    }
}
