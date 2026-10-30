import QtQuick

// The places on the left: Home and the user folders, then mounted drives.
Rectangle {
    id: root

    property var session
    signal placeMenuRequested(var place, real sceneX, real sceneY)

    implicitWidth: Theme.sidebarWidth
    color: Theme.sidebarBg

    Flickable {
        anchors.fill: parent
        anchors.margins: Theme.sidebarPadding
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Column {
            id: column
            width: parent.width

            Repeater {
                model: PlacesController.places
                delegate: SidebarItem {
                    required property var modelData
                    width: column.width
                    name: modelData.name
                    icon: Icons.forPlace(modelData.kind)
                    active: root.session?.path === modelData.path
                    onClicked: root.session.navigate(modelData.path)
                    onMiddleClicked: TabsController.openInBackground(modelData.path)
                    onUrlsDropped: (urls) => OperationsController.dropUrls(urls, modelData.path)
                    onMenuRequested: (sceneX, sceneY) => root.placeMenuRequested(modelData, sceneX, sceneY)
                }
            }

            Text {
                visible: PlacesController.bookmarks.length > 0
                height: Theme.sidebarHeaderHeight
                leftPadding: Theme.sidebarPadding
                verticalAlignment: Text.AlignBottom
                bottomPadding: 6
                text: "Bookmarks"
                font.pixelSize: Theme.smallFontSize
                color: Theme.textDim
            }

            Repeater {
                model: PlacesController.bookmarks
                delegate: SidebarItem {
                    required property var modelData
                    width: column.width
                    name: modelData.name
                    icon: Icons.forPlace(modelData.kind)
                    active: root.session?.path === modelData.path
                    onClicked: root.session.navigate(modelData.path)
                    onMiddleClicked: TabsController.openInBackground(modelData.path)
                    onUrlsDropped: (urls) => OperationsController.dropUrls(urls, modelData.path)
                    onMenuRequested: (sceneX, sceneY) => root.placeMenuRequested(modelData, sceneX, sceneY)
                }
            }

            Text {
                visible: PlacesController.drives.length > 0
                height: Theme.sidebarHeaderHeight
                leftPadding: Theme.sidebarPadding
                verticalAlignment: Text.AlignBottom
                bottomPadding: 6
                text: "Drives"
                font.pixelSize: Theme.smallFontSize
                color: Theme.textDim
            }

            Repeater {
                model: PlacesController.drives
                delegate: SidebarItem {
                    required property var modelData
                    width: column.width
                    name: modelData.name
                    icon: Icons.forPlace(modelData.kind)
                    active: root.session?.path === modelData.path
                    onClicked: root.session.navigate(modelData.path)
                    onMiddleClicked: TabsController.openInBackground(modelData.path)
                    onUrlsDropped: (urls) => OperationsController.dropUrls(urls, modelData.path)
                }
            }
        }
    }
}
