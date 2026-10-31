import QtQuick

// Choose an app for a file. Starts with the apps that say they can open it.
DialogFrame {
    id: root

    property var paths: []
    property var apps: []
    property var type: ({})                // what opening the file goes by: {label, mimeType, defaultApp}
    property bool showingAll: false

    function ask(files) {
        paths = files
        showingAll = false
        type = AppsController.typeOf(files[0])
        always.checked = false
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
    AppList {
        anchors.top: title.bottom
        anchors.topMargin: 10
        anchors.bottom: always.top
        anchors.bottomMargin: 10
        width: parent.width
        apps: root.apps
        searchable: root.showingAll
        defaultApp: root.type.defaultApp ?? ""
        onChosen: (appId) => {
            if (always.checked) AppsController.setDefault(appId, [root.type.mimeType], false)
            AppsController.open(appId, root.paths)
            root.close()
        }
    }
    CheckRow {
        id: always
        anchors.bottom: footer.top
        anchors.bottomMargin: 10
        text: "Always use this app for " + (root.type.label ?? "") + " files"
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
