import QtQuick

// The folder the app opens to, when that is the choice. It takes the folder of the current tab.
Item {
    id: root

    implicitHeight: Theme.settingsRowHeight
    height: visible ? implicitHeight : 0

    Text {
        objectName: "startFolderPath"
        anchors.left: parent.left
        anchors.right: button.left
        anchors.rightMargin: 12
        anchors.verticalCenter: parent.verticalCenter
        text: SettingsController.startFolder
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        font.pixelSize: Theme.fontSize
        color: Theme.textMuted
    }
    DialogButton {
        id: button
        objectName: "useCurrentFolderButton"
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        text: "Use Current Folder"
        onClicked: SettingsController.startFolder = TabsController.currentSession.path
    }
}
