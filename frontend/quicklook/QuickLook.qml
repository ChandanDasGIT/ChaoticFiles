import QtQuick

// Space on an image: the picture full screen, on the monitor the app is on. It follows the selection while it is
// open. Space, Esc or the close button in the corner closes it; the arrow keys go to the image before or after it.
// Small pictures are shown at their own size, never blown up; big ones are loaded at the size they are shown.
Window {
    id: root

    property var session
    readonly property var item: session ? session.currentItem : ({})
    readonly property string path: session ? session.currentPath : ""
    readonly property bool isImage: (item.category ?? "") === "image"
    readonly property var details: visible && isImage ? PreviewController.imageDetails(path) : ({})
    readonly property bool animated: details.animated ?? false
    readonly property var shown: animated ? animatedPicture : picture

    function toggle() {
        if (visible) close()
        else if (isImage) openFullScreen()
    }

    // Some desktops (Hyprland, for one) ignore full screen asked for while a window is still opening, so it
    // opens screen-sized and asks once its first frame is drawn. That works on every desktop, X11 or Wayland.
    property bool fullScreenPending: false
    function openFullScreen() {
        if (transientParent) screen = transientParent.screen
        width = screen.width
        height = screen.height
        fullScreenPending = true
        showNormal()                            // not straight to full screen: see above
        requestActivate()
    }
    onFrameSwapped: if (fullScreenPending) { fullScreenPending = false; showFullScreen() }

    onIsImageChanged: if (!isImage) close()     // the selection moved off images, or the image was deleted

    title: (item.name ?? "") + " — Quick Look"
    color: Theme.quickLookBg

    Item {
        anchors.fill: parent
        anchors.margins: Theme.quickLookMargin
        focus: true
        Keys.onPressed: (event) => {
            switch (event.key) {
            case Qt.Key_Space:
            case Qt.Key_Escape: if (!event.isAutoRepeat) root.close(); break
            case Qt.Key_Left:
            case Qt.Key_Up:    root.session.stepToImage(-1); break
            case Qt.Key_Right:
            case Qt.Key_Down:  root.session.stepToImage(1); break
            default: return
            }
            event.accepted = true
        }

        Item {
            id: area
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: caption.top
            anchors.bottomMargin: Theme.quickLookCaptionGap

            Image {
                id: picture
                anchors.fill: parent
                visible: !root.animated
                asynchronous: true
                cache: false
                autoTransform: true
                fillMode: implicitWidth <= width && implicitHeight <= height ? Image.Pad : Image.PreserveAspectFit
                sourceSize.width: area.width * Screen.devicePixelRatio
                sourceSize.height: area.height * Screen.devicePixelRatio
                source: root.visible && root.isImage && !root.animated ? root.details.url : ""
            }
            AnimatedImage {
                id: animatedPicture
                anchors.fill: parent
                visible: root.animated
                cache: false
                fillMode: implicitWidth <= width && implicitHeight <= height ? Image.Pad : Image.PreserveAspectFit
                source: root.visible && root.isImage && root.animated ? root.details.url : ""
            }

            Text {
                anchors.centerIn: parent
                visible: root.shown.status !== Image.Ready
                text: root.shown.status === Image.Error ? "This image cannot be shown." : Icons.fileImage
                font.family: root.shown.status === Image.Error ? Theme.fontFamily : Icons.fontFamily
                font.pixelSize: root.shown.status === Image.Error ? Theme.fontSize : Theme.quickLookIconSize
                color: Theme.quickLookMuted
            }
        }

        Text {
            id: caption
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            horizontalAlignment: Text.AlignHCenter
            text: [(root.details.width ?? 0) > 0 ? root.details.width + " × " + root.details.height : "",
                   root.item.size ?? ""].filter((part) => part).join("   ·   ")
            textFormat: Text.PlainText
            elide: Text.ElideMiddle
            font.pixelSize: Theme.smallFontSize
            color: Theme.quickLookMuted
        }
    }

    // the way out for the mouse, as a full screen window has no title bar
    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: Theme.quickLookCloseMargin
        width: Theme.toolbarButtonSize
        height: Theme.toolbarButtonSize
        radius: Theme.controlRadius
        color: closeArea.containsMouse ? Theme.quickLookHoverBg : "transparent"

        Text {
            anchors.centerIn: parent
            text: Icons.close
            font.family: Icons.fontFamily
            font.pixelSize: Theme.iconSize
            color: closeArea.containsMouse ? Theme.quickLookText : Theme.quickLookMuted
        }
        MouseArea {
            id: closeArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: root.close()
        }
    }
}
