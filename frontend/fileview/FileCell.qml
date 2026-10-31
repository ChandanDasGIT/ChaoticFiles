import QtQuick

// One file or folder in the grid: a thumbnail or big icon over its name.
Item {
    id: root

    required property int index
    required property string name
    required property string typeText
    required property string sizeText
    required property string modifiedText
    required property string category
    required property string mimeIcon
    required property string path
    required property bool isDirectory
    required property bool isLink
    required property bool isHidden
    required property bool isBroken
    required property bool isSelected
    required property bool isCurrent
    required property bool isCut
    required property string iconKey
    required property string colorKey

    property alias dragText: itemArea.dragText
    property alias dragImage: itemArea.dragImage
    property alias dragReady: itemArea.dragReady
    readonly property bool dragHeld: itemArea.dragHeld
    signal pressedWith(int button, int modifiers, real sceneX, real sceneY)
    signal releasedWithoutDrag(int modifiers)
    signal doubleClicked()
    signal urlsDropped(var urls)
    signal dragRequested()

    readonly property int imageSize: SettingsController.gridSize
    readonly property string thumbnailKind: category === "image" || category === "video" || category === "pdf" ? category : ""

    width: GridView.view.cellWidth
    height: GridView.view.cellHeight
    opacity: isCut ? Theme.cutOpacity : 1

    Rectangle {
        anchors.fill: parent
        anchors.margins: 2
        radius: Theme.controlRadius
        color: itemArea.containsDrag ? Theme.selectionHoverBg
             : root.isSelected ? (itemArea.containsMouse ? Theme.selectionHoverBg : Theme.selectionBg)
             : itemArea.containsMouse ? Theme.hoverBg : "transparent"
    }

    Text {
        anchors.horizontalCenter: picture.horizontalCenter
        anchors.verticalCenter: picture.verticalCenter
        visible: root.mimeIcon === "" && (thumbnail.status !== Image.Ready || thumbnail.paintedWidth === 0)
        text: root.isBroken ? Icons.warning : Icons.forItem(root.category, root.iconKey)
        font.family: Icons.fontFamily
        font.pixelSize: root.imageSize * 0.6
        color: root.isBroken ? Theme.danger : Theme.itemColor(root.category, root.colorKey)
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
    }
    TypeIcon {
        anchors.horizontalCenter: picture.horizontalCenter
        anchors.verticalCenter: picture.verticalCenter
        visible: name !== "" && (thumbnail.status !== Image.Ready || thumbnail.paintedWidth === 0)
        name: root.mimeIcon
        size: root.imageSize * 0.75
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
    }

    Item {
        id: picture
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: Theme.gridPadding
        width: root.imageSize
        height: root.imageSize

        Image {
            id: thumbnail
            anchors.fill: parent
            asynchronous: true
            cache: false
            fillMode: Image.PreserveAspectFit
            sourceSize.width: root.imageSize
            sourceSize.height: root.imageSize
            // the date is in the address so a changed file gets a new thumbnail
            source: SettingsController.showThumbnails && root.thumbnailKind !== ""
                    ? "image://thumb/" + root.thumbnailKind + "/" + encodeURIComponent(root.path) + "#" + root.modifiedText : ""
        }
    }

    Text {
        anchors.top: picture.bottom
        anchors.topMargin: 4
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        horizontalAlignment: Text.AlignHCenter
        text: root.name
        textFormat: Text.PlainText
        wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        maximumLineCount: 2
        elide: Text.ElideMiddle
        font.pixelSize: Theme.smallFontSize
        color: root.isBroken ? Theme.danger : Theme.text
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
    }

    FileItemArea {
        id: itemArea
        anchors.fill: parent
        isDirectory: root.isDirectory
        onPressedWith: (button, modifiers, sceneX, sceneY) => root.pressedWith(button, modifiers, sceneX, sceneY)
        onReleasedWithoutDrag: (modifiers) => root.releasedWithoutDrag(modifiers)
        onDoubleClicked: root.doubleClicked()
        onUrlsDropped: (urls) => root.urlsDropped(urls)
        onDragRequested: root.dragRequested()
    }
}
