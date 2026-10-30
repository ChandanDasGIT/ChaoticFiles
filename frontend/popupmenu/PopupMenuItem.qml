import QtQuick

// One row of a PopupMenu: icon, text, and on the right a shortcut or a check mark.
Item {
    id: root

    property var entry: ({})
    signal triggered()

    readonly property bool isSeparator: entry.separator === true
    readonly property bool isHeader: entry.header !== undefined
    readonly property bool isEnabled: entry.enabled !== false

    implicitWidth: isSeparator ? 0 : row.implicitWidth + 2 * Theme.menuRowPadding
    implicitHeight: isSeparator ? Theme.menuPadding * 2 + 1 : Theme.menuRowHeight

    Rectangle {
        visible: root.isSeparator
        anchors.verticalCenter: parent.verticalCenter
        width: parent.width
        height: 1
        color: Theme.divider
    }

    Rectangle {
        visible: !root.isSeparator
        anchors.fill: parent
        radius: Theme.controlRadius
        color: area.containsMouse && root.isEnabled && !root.isHeader ? Theme.hoverBg : "transparent"
    }

    Row {
        id: row
        visible: !root.isSeparator
        anchors.left: parent.left
        anchors.leftMargin: Theme.menuRowPadding
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.menuRowPadding

        Text {
            visible: !root.isHeader
            width: Theme.iconSize + 4
            anchors.verticalCenter: parent.verticalCenter
            horizontalAlignment: Text.AlignHCenter
            text: root.entry.icon ?? ""
            font.family: Icons.fontFamily
            font.pixelSize: Theme.iconSize - 1
            color: root.isEnabled ? Theme.textMuted : Theme.textDisabled
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.isHeader ? root.entry.header : (root.entry.text ?? "")
            font.pixelSize: root.isHeader ? Theme.smallFontSize : Theme.fontSize
            color: root.isHeader ? Theme.textDim : root.isEnabled ? Theme.text : Theme.textDisabled
        }
    }

    Text {
        visible: !root.isSeparator && text !== ""
        anchors.right: parent.right
        anchors.rightMargin: Theme.menuRowPadding
        anchors.verticalCenter: parent.verticalCenter
        text: root.entry.checked ? Icons.check : (root.entry.shortcut ?? "")
        font.family: root.entry.checked ? Icons.fontFamily : Theme.fontFamily
        font.pixelSize: Theme.smallFontSize
        color: root.entry.checked ? Theme.accent : Theme.textDim
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        enabled: root.isEnabled && !root.isHeader && !root.isSeparator
        onClicked: root.triggered()
    }
}
