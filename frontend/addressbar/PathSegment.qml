import QtQuick

// One folder of the address bar: its name goes to that folder, the arrow after it lists its subfolders.
Item {
    id: root

    property string label: ""
    property string icon: ""               // an icon instead of a name, for the "…" button
    property bool lastPart: false
    property bool showArrow: true
    signal clicked()
    signal arrowClicked(real sceneX, real sceneY)

    implicitWidth: labelBox.width + (showArrow ? Theme.segmentArrowWidth : 0)
    implicitHeight: Theme.addressHeight

    Rectangle {
        id: labelBox
        height: parent.height - 6
        anchors.verticalCenter: parent.verticalCenter
        width: labelText.implicitWidth + 2 * Theme.segmentPadding
        radius: Theme.controlRadius
        color: labelArea.containsMouse ? Theme.hoverBg : "transparent"

        Text {
            textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
            id: labelText
            anchors.centerIn: parent
            text: root.icon !== "" ? root.icon : root.label
            font.family: root.icon !== "" ? Icons.fontFamily : Theme.fontFamily
            font.pixelSize: Theme.fontSize
            color: root.lastPart ? Theme.text : Theme.textMuted
        }
        MouseArea {
            id: labelArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.clicked()
        }
    }

    Rectangle {
        id: arrowBox
        visible: root.showArrow
        anchors.left: labelBox.right
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.segmentArrowWidth
        height: labelBox.height
        radius: Theme.controlRadius
        color: arrowArea.containsMouse ? Theme.hoverBg : "transparent"

        Text {
            anchors.centerIn: parent
            text: Icons.chevronRight
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize - 3
            color: Theme.textDim
        }
        MouseArea {
            id: arrowArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                const corner = arrowBox.mapToItem(null, 0, arrowBox.height)
                root.arrowClicked(corner.x, corner.y)
            }
        }
    }
}
