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
    required property string fileTag
    property bool showingMatches: false    // the type column shows the line found in the file, and fills the row
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

    x: Theme.rowMargin
    width: ListView.view.width - 2 * Theme.rowMargin
    height: Theme.rowHeight
    radius: Theme.rowRadius
    color: itemArea.containsDrag ? Theme.selectionHoverBg
         : isSelected ? (itemArea.containsMouse ? Theme.selectionHoverBg : Theme.selectionBg)
         : itemArea.containsMouse ? Theme.hoverBg : "transparent"
    opacity: isCut ? Theme.cutOpacity : 1

    // ---- name ----
    Text {
        id: icon
        anchors.left: parent.left
        anchors.leftMargin: Theme.rowPadding
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.rowIconWidth
        text: root.isBroken ? Icons.warning : Icons.forItem(root.category, root.iconKey)
        font.family: Icons.fontFamily
        font.pixelSize: Theme.iconSize + 1
        color: root.isBroken ? Theme.danger : Theme.itemColor(root.category, root.colorKey)
        opacity: root.fileTag !== "" || art.shown ? 0 : root.isHidden ? Theme.hiddenOpacity : 1
    }
    Image {
        id: art
        readonly property bool shown: status === Image.Ready && paintedWidth > 0
        x: icon.x
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.rowArtSize
        height: Theme.rowArtSize
        visible: shown
        asynchronous: true
        fillMode: Image.PreserveAspectFit
        sourceSize.width: 2 * width
        sourceSize.height: 2 * height
        opacity: root.isHidden ? Theme.hiddenOpacity : 1
        source: SettingsController.showAlbumArt && root.category === "audio"
                ? "image://thumb/audio/" + encodeURIComponent(root.path) + "#" + root.modifiedText : ""
    }
    Rectangle {
        visible: root.fileTag !== ""
        x: icon.x
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.fileTagWidth
        height: Theme.fileTagHeight
        radius: Theme.fileTagRadius
        color: Theme.tagColor(root.fileTag, root.category)
        opacity: root.isHidden ? Theme.hiddenOpacity : 1

        Text {
            anchors.fill: parent
            anchors.leftMargin: 1
            anchors.rightMargin: 1
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: root.fileTag
            fontSizeMode: Text.HorizontalFit
            minimumPixelSize: Theme.fileTagSmallestFont
            font.pixelSize: Theme.fileTagFontSize
            font.bold: true
            color: Theme.windowBg
        }
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
        anchors.right: root.showingMatches ? type.left : modified.left
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
        visible: !root.showingMatches
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
        anchors.right: root.showingMatches ? parent.right : size.left
        anchors.verticalCenter: parent.verticalCenter
        width: root.showingMatches ? Theme.foundWidth(root.width) : Theme.columnType
        leftPadding: Theme.rowPadding
        text: root.typeText
        textFormat: Text.PlainText
        elide: root.showingMatches ? Text.ElideRight : Text.ElideMiddle
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
    Text {
        id: size
        visible: !root.showingMatches
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.columnSize
        rightPadding: Theme.rowPadding
        horizontalAlignment: Text.AlignRight
        text: root.sizeText
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
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
