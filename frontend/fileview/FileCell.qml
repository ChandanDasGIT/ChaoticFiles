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
    required property string fileTag
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
    readonly property string thumbnailKind: category === "audio" ? (SettingsController.showAlbumArt ? category : "")
        : SettingsController.showThumbnails && (category === "image" || category === "video" || category === "pdf") ? category : ""

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
        visible: thumbnail.status !== Image.Ready || thumbnail.paintedWidth === 0
        text: root.isBroken ? Icons.warning : root.fileTag !== "" ? Icons.file : Icons.forItem(root.category, root.iconKey)
        font.family: Icons.fontFamily
        font.pixelSize: root.imageSize * 0.6
        color: root.isBroken ? Theme.danger
             : root.fileTag !== "" ? Theme.tagColor(root.fileTag, root.category)
             : Theme.itemColor(root.category, root.colorKey)
        opacity: root.isHidden ? Theme.hiddenOpacity : 1

        Text {
            visible: root.fileTag !== ""
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: parent.font.pixelSize * 0.12
            width: parent.implicitWidth * 0.8
            horizontalAlignment: Text.AlignHCenter
            text: root.fileTag
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Theme.fileTagSmallestFont
            font.pixelSize: root.imageSize * 0.16
            font.bold: true
            color: Theme.windowBg
        }
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
            cache: root.category === "audio"    // album art stays in memory, so scrolling back doesn't load it again
            fillMode: Image.PreserveAspectFit
            sourceSize.width: root.imageSize
            sourceSize.height: root.imageSize
            // the date is in the address so a changed file gets a new thumbnail
            source: root.thumbnailKind !== ""
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
