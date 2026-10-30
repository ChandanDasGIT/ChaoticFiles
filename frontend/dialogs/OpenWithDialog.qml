import QtQuick
import QtQuick.Controls.Basic

// Choose an app for a file. Starts with the apps that say they can open it.
DialogFrame {
    id: root

    property var paths: []
    property var apps: []
    property bool showingAll: false

    function ask(files) {
        paths = files
        showingAll = false
        apps = AppsController.appsFor(files[0])
        if (apps.length === 0) showAll()
        open()
    }

    function showAll() {
        showingAll = true
        apps = AppsController.allApps()
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.openWithHeight, parent.height - 2 * Theme.dialogMargin)

    Text {
        id: title
        text: "Open With"
        font.pixelSize: Theme.dialogTitleSize
        font.weight: Font.DemiBold
        color: Theme.text
    }
    ListView {
        anchors.top: title.bottom
        anchors.topMargin: 10
        anchors.bottom: footer.top
        anchors.bottomMargin: 10
        width: parent.width
        clip: true
        model: root.apps
        boundsBehavior: Flickable.StopAtBounds
        ScrollBar.vertical: ThinScrollBar {}

        delegate: Rectangle {
            required property var modelData
            width: ListView.view.width
            height: Theme.popupRowHeight
            radius: Theme.controlRadius
            color: area.containsMouse ? Theme.hoverBg : "transparent"

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 10
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                text: parent.modelData.name
                textFormat: Text.PlainText
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    AppsController.open(parent.modelData.id, root.paths)
                    root.close()
                }
            }
        }
    }
    Row {
        id: footer
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        spacing: 8
        DialogButton { text: "Show All Apps"; visible: !root.showingAll; onClicked: root.showAll() }
        DialogButton { text: "Cancel"; onClicked: root.close() }
    }
}
