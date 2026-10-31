import QtQuick
import QtQuick.Controls.Basic

// What takes up the space in a folder, biggest first. Only draws DiskUsageController's state: click a folder to look inside.
DialogFrame {
    id: root

    signal goToFolder(string folder)

    visible: DiskUsageController.isOpen
    onClosed: DiskUsageController.close()

    width: Math.min(Theme.usageWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.usageHeight, parent.height - 2 * Theme.dialogMargin)

    ToolbarButton {
        id: upButton
        objectName: "usageUp"
        anchors.left: parent.left
        anchors.top: parent.top
        visible: DiskUsageController.canGoUp
        icon: Icons.up
        onClicked: DiskUsageController.goUp()
    }
    Text {
        anchors.left: upButton.visible ? upButton.right : parent.left
        anchors.leftMargin: upButton.visible ? 8 : 0
        anchors.right: closeButton.left
        anchors.rightMargin: 8
        anchors.verticalCenter: upButton.verticalCenter
        text: DiskUsageController.folder
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        font.pixelSize: Theme.dialogTitleSize
        font.weight: Font.DemiBold
        color: Theme.text
    }
    ToolbarButton {
        id: closeButton
        anchors.right: parent.right
        anchors.top: parent.top
        icon: Icons.close
        onClicked: root.close()
    }

    ListView {
        id: list
        anchors.top: closeButton.bottom
        anchors.topMargin: 8
        anchors.bottom: footer.top
        anchors.bottomMargin: 8
        width: parent.width
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: DiskUsageController.items
        ScrollBar.vertical: ThinScrollBar {}

        delegate: Item {
            id: row
            required property var modelData
            required property int index

            objectName: "usageRow" + index
            width: ListView.view.width - Theme.scrollbarWidth
            height: Theme.usageRowHeight

            Rectangle {
                anchors.fill: parent
                radius: Theme.controlRadius
                color: area.containsMouse && row.modelData.isFolder ? Theme.hoverBg : "transparent"
            }
            Text {
                id: rowIcon
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.top: parent.top
                anchors.topMargin: 6
                text: row.modelData.isFolder ? Icons.folder : Icons.file
                font.family: Icons.fontFamily
                font.pixelSize: Theme.iconSize
                color: row.modelData.isFolder ? Theme.folderIcon : Theme.textDim
            }
            Text {
                anchors.left: rowIcon.right
                anchors.leftMargin: 8
                anchors.right: sizeText.left
                anchors.rightMargin: 8
                anchors.top: parent.top
                anchors.topMargin: 5
                text: row.modelData.name
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            Text {
                id: sizeText
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.top: parent.top
                anchors.topMargin: 6
                width: Theme.usageSizeWidth
                horizontalAlignment: Text.AlignRight
                text: row.modelData.sizeText
                font.pixelSize: Theme.smallFontSize
                color: Theme.textMuted
            }
            Rectangle {
                anchors.left: parent.left
                anchors.leftMargin: 6
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 4
                height: Theme.usageBarHeight
                radius: height / 2
                color: Theme.divider

                Rectangle {
                    width: Math.max(height, parent.width * row.modelData.share)
                    height: parent.height
                    radius: parent.radius
                    color: Theme.accent
                }
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                enabled: row.modelData.isFolder
                cursorShape: Qt.PointingHandCursor
                onClicked: DiskUsageController.openFolder(row.modelData.path)
            }
        }
    }

    Item {
        id: footer
        anchors.bottom: parent.bottom
        width: parent.width
        height: Theme.dialogButtonHeight

        Text {
            anchors.left: parent.left
            anchors.right: goButton.left
            anchors.rightMargin: 8
            anchors.verticalCenter: parent.verticalCenter
            text: DiskUsageController.summary
            textFormat: Text.PlainText
            elide: Text.ElideRight
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        DialogButton {
            id: goButton
            anchors.right: parent.right
            text: "Go to This Folder"
            onClicked: {
                const folder = DiskUsageController.folder
                root.close()
                root.goToFolder(folder)
            }
        }
    }
}
