import QtQuick
import QtQuick.Controls.Basic

// A list of apps to choose from, each with its icon, and a search box when `searchable`. The current default is marked.
Item {
    id: root

    property var apps: []
    property bool searchable: false
    property string defaultApp: ""
    signal chosen(string appId)

    onSearchableChanged: {
        search.text = ""
        if (searchable) search.forceActiveFocus()
    }

    Rectangle {
        id: searchBox
        visible: root.searchable
        width: parent.width
        height: visible ? Theme.appSearchHeight : 0
        radius: Theme.controlRadius
        color: Theme.dialogFieldBg
        border.color: search.activeFocus ? Theme.accent : Theme.menuBorder

        Text {
            id: searchIcon
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: Icons.search
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        TextInput {
            id: search
            anchors.left: searchIcon.right
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.leftMargin: 8
            anchors.rightMargin: 10
            verticalAlignment: TextInput.AlignVCenter
            clip: true
            selectByMouse: true
            font.pixelSize: Theme.fontSize
            color: Theme.text
            selectionColor: Theme.selectionBg
            selectedTextColor: Theme.text
        }
        Text {
            anchors.fill: search
            verticalAlignment: Text.AlignVCenter
            visible: search.text === ""
            text: "Search apps"
            font.pixelSize: Theme.fontSize
            color: Theme.textDim
        }
    }

    ListView {
        id: list
        anchors.top: searchBox.bottom
        anchors.topMargin: root.searchable ? 8 : 0
        anchors.bottom: parent.bottom
        width: parent.width
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: AppsController.filterApps(root.apps, search.text)
        ScrollBar.vertical: ThinScrollBar {}

        delegate: Rectangle {
            required property var modelData
            width: ListView.view.width
            height: Theme.popupRowHeight
            radius: Theme.controlRadius
            color: area.containsMouse ? Theme.hoverBg : "transparent"

            AppIcon {
                id: appIcon
                anchors.left: parent.left
                anchors.leftMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                name: parent.modelData.icon
            }
            Text {
                anchors.left: appIcon.right
                anchors.leftMargin: 8
                anchors.right: defaultMark.visible ? defaultMark.left : parent.right
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                text: parent.modelData.name
                textFormat: Text.PlainText
                elide: Text.ElideRight
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            Text {
                id: defaultMark
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                visible: parent.modelData.id === root.defaultApp
                text: "Default"
                font.pixelSize: Theme.smallFontSize
                color: Theme.textMuted
            }
            MouseArea {
                id: area
                anchors.fill: parent
                hoverEnabled: true
                onClicked: root.chosen(parent.modelData.id)
            }
        }
    }
    Text {
        anchors.centerIn: list
        visible: list.count === 0 && search.text !== ""
        text: "No apps found."
        font.pixelSize: Theme.fontSize
        color: Theme.textMuted
    }
}
