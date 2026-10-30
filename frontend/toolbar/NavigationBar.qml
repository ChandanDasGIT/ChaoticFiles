import QtQuick

// The row under the tabs: back, forward, up, refresh, the address bar and the settings button.
Rectangle {
    id: root

    property var session
    readonly property alias addressBar: address
    signal settingsRequested(real sceneX, real sceneY)
    signal addressFinished()

    implicitHeight: Theme.toolbarHeight
    color: Theme.toolbarBg

    Row {
        id: buttons
        anchors.left: parent.left
        anchors.leftMargin: Theme.toolbarMargin
        anchors.verticalCenter: parent.verticalCenter

        ToolbarButton { icon: Icons.back; active: root.session?.canGoBack ?? false; onClicked: root.session.goBack() }
        ToolbarButton { icon: Icons.forward; active: root.session?.canGoForward ?? false; onClicked: root.session.goForward() }
        ToolbarButton { icon: Icons.up; active: root.session?.canGoUp ?? false; onClicked: root.session.goUp() }
        ToolbarButton { icon: Icons.refresh; onClicked: root.session.refresh() }
    }

    AddressBar {
        id: address
        session: root.session
        anchors.left: buttons.right
        anchors.leftMargin: Theme.toolbarMargin
        anchors.right: viewButtons.left
        anchors.rightMargin: Theme.toolbarMargin
        anchors.verticalCenter: parent.verticalCenter
        onFinished: root.addressFinished()
    }

    Row {
        id: viewButtons
        anchors.right: parent.right
        anchors.rightMargin: Theme.toolbarMargin
        anchors.verticalCenter: parent.verticalCenter

        ToolbarButton {
            icon: SettingsController.viewMode === 0 ? Icons.grid : Icons.list
            onClicked: SettingsController.viewMode = SettingsController.viewMode === 0 ? 1 : 0
        }
        ToolbarButton {
            icon: Icons.preview
            onClicked: SettingsController.showPreview = !SettingsController.showPreview
        }
        ToolbarButton {
            icon: Icons.settings
            onClicked: {
                const corner = mapToItem(null, width, height)
                root.settingsRequested(corner.x, corner.y)
            }
        }
    }
}
