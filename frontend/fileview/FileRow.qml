import QtQuick

// One file or folder in the list. It only draws; every click goes to the session.
Rectangle {
    id: root

    required property int index
    required property string name
    required property string typeText
    required property string sizeText
    required property string modifiedText
    required property string category
    required property string path
    required property bool isDirectory
    required property bool isLink
    required property bool isHidden
    required property bool isBroken
    required property bool isSelected
    required property bool isCurrent
    required property bool isCut

    property string dragText: ""           // file URLs, set when the mouse goes down

    signal pressedWith(int button, int modifiers, real sceneX, real sceneY)
    signal releasedWithoutDrag(int modifiers)
    signal doubleClicked()
    signal urlsDropped(var urls)

    x: Theme.rowMargin
    width: ListView.view.width - 2 * Theme.rowMargin
    height: Theme.rowHeight
    radius: Theme.rowRadius
    color: dropArea.containsDrag ? Theme.selectionHoverBg
         : isSelected ? (area.containsMouse ? Theme.selectionHoverBg : Theme.selectionBg)
         : area.containsMouse ? Theme.hoverBg : "transparent"
    opacity: isCut ? Theme.cutOpacity : 1

    // ---- name ----
    Text {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: Theme.rowPadding
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.rowIconWidth
        text: root.isBroken ? Icons.warning : Icons.forCategory(root.category)
        font.family: Icons.fontFamily
        font.pixelSize: Theme.iconSize + 1
        color: root.isBroken ? Theme.danger : Theme.categoryColor(root.category)
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
    }
    Text {
        id: linkBadge
        visible: root.isLink
        x: icon.x + 9
        y: icon.y + icon.height - 7
        text: Icons.link
        font.family: Icons.fontFamily
        font.pixelSize: Theme.smallFontSize - 3
        color: Theme.text
    }
    Text {
        anchors.left: icon.right
        anchors.right: modified.left
        anchors.rightMargin: Theme.rowPadding
        anchors.verticalCenter: parent.verticalCenter
        text: root.name
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        font.pixelSize: Theme.fontSize
        color: root.isBroken ? Theme.danger : Theme.text
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
    }

    // ---- other columns ----
    Text {
        id: modified
        anchors.right: type.left
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.columnModified
        leftPadding: Theme.rowPadding
        text: root.modifiedText
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
    Text {
        id: type
        anchors.right: size.left
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.columnType
        leftPadding: Theme.rowPadding
        text: root.typeText
        textFormat: Text.PlainText
        elide: Text.ElideRight
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
    Text {
        id: size
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.columnSize
        rightPadding: Theme.rowPadding
        horizontalAlignment: Text.AlignRight
        text: root.sizeText
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }

    // what a drag carries: it stays still, and the system draws the drag
    Item {
        id: dragItem
        Drag.dragType: Drag.Automatic
        Drag.active: area.drag.active
        Drag.supportedActions: Qt.CopyAction | Qt.MoveAction | Qt.LinkAction
        Drag.mimeData: ({"text/uri-list": root.dragText})
    }

    // files dragged onto a folder go into it
    DropArea {
        id: dropArea
        anchors.fill: parent
        enabled: root.isDirectory
        onEntered: (drag) => { drag.accepted = drag.hasUrls }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            root.urlsDropped(drop.urls)
        }
    }

    MouseArea {
        id: area
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.RightButton | Qt.MiddleButton
        drag.target: pressedButtons & Qt.LeftButton ? dragItem : null
        drag.threshold: 8
        onPressed: (mouse) => {
            const scene = mapToItem(null, mouse.x, mouse.y)
            root.pressedWith(mouse.button, mouse.modifiers, scene.x, scene.y)
        }
        onReleased: (mouse) => {
            if (mouse.button === Qt.LeftButton && !drag.active) root.releasedWithoutDrag(mouse.modifiers)
            dragItem.x = 0
            dragItem.y = 0
        }
        onDoubleClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) root.doubleClicked()
        }
    }
}
