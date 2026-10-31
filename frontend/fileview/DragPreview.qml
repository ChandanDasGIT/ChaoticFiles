import QtQuick

// The picture that follows the mouse during a drag: what is being dragged, as a stack when it is several items.
// It sits out of sight, and is photographed when a drag starts.
Item {
    id: root

    property string icon: ""
    property color iconColor: Theme.text
    property string label: ""
    property int count: 1
    property var photo: null               // keeps the picture alive for as long as the drag needs it

    readonly property int layers: Math.min(count - 1, 2)
    readonly property int offset: layers * Theme.dragStackOffset

    function capture(icon, iconColor, name, count, done) {
        root.icon = icon
        root.iconColor = iconColor
        root.count = count
        root.label = count > 1 ? count + " items" : name
        grabToImage((result) => {
            root.photo = result
            done(result.url)
        })
    }

    x: Theme.offScreen
    width: Math.min(Theme.dragPreviewMaxWidth, glyph.implicitWidth + row.spacing + name.implicitWidth + 2 * Theme.dragPreviewPadding) + offset
    height: Theme.dragPreviewHeight + offset

    Repeater {
        model: root.layers
        delegate: Rectangle {
            required property int index
            x: root.offset - (index + 1) * Theme.dragStackOffset + Theme.dragStackOffset
            y: x
            width: card.width
            height: card.height
            radius: Theme.controlRadius
            color: Theme.menuBg
            border.color: Theme.menuBorder
        }
    }
    Rectangle {
        id: card
        width: root.width - root.offset
        height: Theme.dragPreviewHeight
        radius: Theme.controlRadius
        color: Theme.menuBg
        border.color: Theme.menuBorder

        Row {
            id: row
            anchors.verticalCenter: parent.verticalCenter
            x: Theme.dragPreviewPadding
            width: parent.width - 2 * Theme.dragPreviewPadding
            spacing: 8

            Text {
                id: glyph
                anchors.verticalCenter: parent.verticalCenter
                text: root.icon
                font.family: Icons.fontFamily
                font.pixelSize: Theme.iconSize + 1
                color: root.iconColor
            }
            Text {
                id: name
                anchors.verticalCenter: parent.verticalCenter
                width: row.width - glyph.width - row.spacing
                text: root.label
                textFormat: Text.PlainText
                elide: Text.ElideMiddle
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
        }
    }
}
