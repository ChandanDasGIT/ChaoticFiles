import QtQuick
import QtQuick.Controls.Basic

// The panel on the right (F3): a picture or the start of the text of the current file, and its details.
Rectangle {
    id: root

    property var session
    readonly property var item: session ? session.currentItem : ({})
    readonly property string path: session ? session.currentPath : ""
    readonly property string category: item.category ?? ""
    readonly property bool hasPicture: category === "image" || category === "video" || category === "pdf"

    function update() {
        if (visible && path !== "" && !hasPicture && category !== "folder") PreviewController.showText(path)
        else PreviewController.clear()
    }
    onPathChanged: update()
    onVisibleChanged: update()

    implicitWidth: Theme.previewWidth
    color: Theme.sidebarBg

    Rectangle {
        width: 1
        height: parent.height
        color: Theme.divider
    }

    Text {
        anchors.centerIn: parent
        visible: root.path === ""
        text: "Select a file to preview it."
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }

    Column {
        anchors.fill: parent
        anchors.margins: Theme.previewPadding
        spacing: 10
        visible: root.path !== ""

        Item {
            id: picture
            width: parent.width
            height: PreviewController.text !== "" ? 0 : width
            visible: height > 0

            Text {
                anchors.centerIn: parent
                visible: thumbnail.status !== Image.Ready || thumbnail.paintedWidth === 0
                text: Icons.forCategory(root.category)
                font.family: Icons.fontFamily
                font.pixelSize: picture.width * 0.4
                color: Theme.categoryColor(root.category)
            }
            Image {
                id: thumbnail
                anchors.fill: parent
                asynchronous: true
                cache: false
                fillMode: Image.PreserveAspectFit
                sourceSize.width: width
                sourceSize.height: height
                source: root.hasPicture ? "image://thumb/" + root.category + "/" + encodeURIComponent(root.path) + "#" + (root.item.date ?? "") : ""
            }
        }

        Text {
            width: parent.width
            text: root.item.name ?? ""
            textFormat: Text.PlainText
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            font.pixelSize: Theme.fontSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            width: parent.width
            text: [root.item.type, root.item.size, root.item.date].filter((part) => part).join("\n")
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }

        Flickable {
            width: parent.width
            height: parent.height - y
            visible: PreviewController.text !== ""
            contentHeight: previewText.height
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            ScrollBar.vertical: ThinScrollBar {}

            Text {
                id: previewText
                width: parent.width - Theme.scrollbarWidth
                text: PreviewController.text
                textFormat: Text.PlainText
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
                font.family: "monospace"
                font.pixelSize: Theme.smallFontSize - 1
                color: Theme.text
            }
        }
    }
}
