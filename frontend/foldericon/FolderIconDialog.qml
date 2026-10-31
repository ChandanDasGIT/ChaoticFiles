import QtQuick

// Choose an icon and a colour for the selected folders. Nothing changes until Apply.
DialogFrame {
    id: root

    property var paths: []
    property string iconKey: ""
    property string colorKey: ""
    signal applied()
    signal failed(string text)

    function ask(folders) {
        if (folders.length === 0) return
        paths = folders
        const current = OperationsController.folderIcon(folders[0])
        iconKey = current.icon
        colorKey = current.color
        open()
    }

    function save(icon, color) {
        const failure = OperationsController.setFolderIcon(paths, icon, color)
        close()
        if (failure) failed(failure)
        else applied()
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)
    contentHeight: column.implicitHeight

    Column {
        id: column
        width: parent.width
        spacing: 12

        Text {
            text: root.paths.length > 1 ? "Change Icon of " + root.paths.length + " Folders" : "Change Icon"
            font.pixelSize: Theme.dialogTitleSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: Icons.forFolder(root.iconKey)
            font.family: Icons.fontFamily
            font.pixelSize: Theme.folderPreviewSize
            color: Theme.folderColor(root.colorKey)
        }
        Grid {
            anchors.horizontalCenter: parent.horizontalCenter
            columns: Math.floor(parent.width / Theme.folderIconCell)

            Repeater {
                model: [{key: "", glyph: Icons.folder}].concat(Icons.folderChoices)
                delegate: Rectangle {
                    required property var modelData
                    width: Theme.folderIconCell
                    height: Theme.folderIconCell
                    radius: Theme.controlRadius
                    color: root.iconKey === modelData.key ? Theme.selectionBg
                         : iconArea.containsMouse ? Theme.hoverBg : "transparent"

                    Text {
                        anchors.centerIn: parent
                        text: parent.modelData.glyph
                        font.family: Icons.fontFamily
                        font.pixelSize: Theme.iconSize + 4
                        color: Theme.folderColor(root.colorKey)
                    }
                    MouseArea {
                        id: iconArea
                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: root.iconKey = parent.modelData.key
                    }
                }
            }
        }
        Row {
            anchors.horizontalCenter: parent.horizontalCenter
            spacing: 8

            Repeater {
                model: [""].concat(Theme.folderColorKeys)
                delegate: Rectangle {
                    required property string modelData
                    width: Theme.folderColorSize
                    height: Theme.folderColorSize
                    radius: width / 2
                    color: Theme.folderColor(modelData)
                    border.width: root.colorKey === modelData ? 2 : 0
                    border.color: Theme.text

                    MouseArea {
                        anchors.fill: parent
                        onClicked: root.colorKey = parent.modelData
                    }
                }
            }
        }
        Item {
            width: parent.width
            height: Theme.dialogButtonHeight

            DialogButton { text: "Reset"; onClicked: root.save("", "") }
            Row {
                anchors.right: parent.right
                spacing: 8
                DialogButton { text: "Cancel"; onClicked: root.close() }
                DialogButton { text: "Apply"; primary: true; onClicked: root.save(root.iconKey, root.colorKey) }
            }
        }
    }
}
