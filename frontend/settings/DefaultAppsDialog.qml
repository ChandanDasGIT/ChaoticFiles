import QtQuick
import QtQuick.Controls.Basic

// Which app opens each kind of file, by category. Every Linux app that opens files shares these choices.
DialogFrame {
    id: root

    width: Math.min(Theme.settingsWidth, parent.width - 2 * Theme.dialogMargin)
    height: Math.min(Theme.settingsHeight, parent.height - 2 * Theme.dialogMargin)

    onAboutToShow: AppsController.loadFileTypes()

    Text {
        text: "Default Apps"
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
    Text {
        anchors.centerIn: parent
        visible: AppsController.fileTypes.length === 0
        text: "Loading…"
        font.pixelSize: Theme.fontSize
        color: Theme.textMuted
    }

    Flickable {
        anchors.top: closeButton.bottom
        anchors.topMargin: 8
        anchors.bottom: parent.bottom
        width: parent.width
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ThinScrollBar {}

        Column {
            id: column
            width: parent.width - Theme.scrollbarWidth
            spacing: 2

            Repeater {
                model: AppsController.fileTypes
                delegate: Column {
                    id: group
                    required property var modelData
                    width: column.width
                    spacing: 2

                    Item {
                        width: parent.width
                        height: Theme.settingsRowHeight

                        Text {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            text: group.modelData.name
                            font.pixelSize: Theme.fontSize
                            font.weight: Font.DemiBold
                            color: Theme.text
                        }
                        DialogButton {
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            text: "Set All…"
                            onClicked: picker.ask("Default app for all " + group.modelData.name, group.modelData.mimeTypes, true, "")
                        }
                    }
                    Repeater {
                        model: group.modelData.types
                        delegate: DefaultAppRow {
                            required property var modelData
                            width: group.width
                            type: modelData
                            onClicked: picker.ask("Default app for " + modelData.label, modelData.mimeTypes, false, modelData.appId)
                        }
                    }
                }
            }
        }
    }

    DefaultAppPicker { id: picker }
}
