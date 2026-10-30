import QtQuick

// The titles above the file list. Clicking one sorts by it; clicking it again flips the order.
Rectangle {
    id: root

    property bool inTrash: false
    property int sortKey: 0
    property bool sortAscending: true
    signal sortRequested(int key)

    implicitHeight: Theme.headerHeight
    color: Theme.windowBg

    component Column: Rectangle {
        id: column
        property string title: ""
        property int key: 0
        property bool alignRight: false

        height: root.height
        color: area.containsMouse ? Theme.hoverBg : "transparent"

        Text {
            anchors.left: column.alignRight ? undefined : parent.left
            anchors.right: column.alignRight ? parent.right : undefined
            anchors.leftMargin: Theme.rowPadding
            anchors.rightMargin: Theme.rowPadding
            anchors.verticalCenter: parent.verticalCenter
            text: column.title
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        Text {
            visible: root.sortKey === column.key
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 1
            text: root.sortAscending ? Icons.caretUp : Icons.caretDown
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize - 3
            color: Theme.textDim
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.sortRequested(column.key)
        }
    }

    Row {
        anchors.fill: parent
        anchors.leftMargin: Theme.rowMargin
        anchors.rightMargin: Theme.rowMargin

        Column {
            title: "Name"
            key: 0
            width: parent.width - Theme.columnModified - Theme.columnType - Theme.columnSize
        }
        Column { title: root.inTrash ? "Date deleted" : "Date modified"; key: 1; width: Theme.columnModified }
        Column { title: root.inTrash ? "Original location" : "Type"; key: 2; width: Theme.columnType }
        Column { title: "Size"; key: 3; width: Theme.columnSize; alignRight: true }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.divider
    }
}
