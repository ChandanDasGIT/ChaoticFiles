import QtQuick

// Choose the default app for one file type, or for a whole category.
DialogFrame {
    id: root

    property string title: ""
    property var mimeTypes: []
    property bool wholeCategory: false
    property string currentApp: ""
    property var apps: []
    property bool showingAll: false

    function showAll() {
        showingAll = true
        apps = AppsController.allApps()
    }

    function ask(title, mimeTypes, wholeCategory, currentApp) {
        root.title = title
        root.mimeTypes = mimeTypes
        root.wholeCategory = wholeCategory
        root.currentApp = currentApp
        showingAll = false
        apps = AppsController.appsForTypes(mimeTypes)
        if (apps.length === 0) showAll()
        open()
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.openWithHeight, parent.height - 2 * Theme.dialogMargin)

    Text {
        id: heading
        width: parent.width
        text: root.title
        elide: Text.ElideRight
        font.pixelSize: Theme.dialogTitleSize
        font.weight: Font.DemiBold
        color: Theme.text
    }
    AppList {
        anchors.top: heading.bottom
        anchors.topMargin: 10
        anchors.bottom: footer.top
        anchors.bottomMargin: 10
        width: parent.width
        apps: root.apps
        searchable: root.showingAll
        defaultApp: root.currentApp
        onChosen: (appId) => {
            AppsController.setDefault(appId, root.mimeTypes, root.wholeCategory)
            root.close()
        }
    }
    Row {
        id: footer
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        spacing: 8
        DialogButton {
            text: "Show All Apps"
            visible: !root.wholeCategory && !root.showingAll
            onClicked: root.showAll()
        }
        DialogButton { text: "Cancel"; onClicked: root.close() }
    }
}
