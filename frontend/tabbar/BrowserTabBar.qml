import QtQuick

// The title bar of the window: the tabs, the "+" button, free space that moves the window, and the
// window buttons. Tabs shrink as more open; when even the smallest size does not fit, the strip scrolls
// with the mouse wheel and keeps the current tab in view. A tab can be dragged to a new place.
Rectangle {
    id: root

    required property var targetWindow

    implicitHeight: Theme.tabBarHeight
    color: Theme.tabBarBg

    // ---- dragging a tab ----
    property int dragIndex: -1             // the tab that is being dragged (-1 = none)
    property real dragGrab: 0              // where inside the tab the mouse took hold of it
    property real dragLeft: 0              // where the left edge of the dragged tab is, in the row

    function beginDrag(index, sceneX, tab) {
        dragGrab = sceneX - tab.mapToItem(null, 0, 0).x
        dragLeft = tab.x
        dragIndex = index
    }

    // the mouse moved to this x (in window coordinates): follow it, and swap places with a tab it passed
    function dragTo(sceneX) {
        if (dragIndex < 0) return
        dragLeft = sceneX - row.mapToItem(null, 0, 0).x - dragGrab
        const center = dragLeft + tabWidth / 2
        const target = Math.max(0, Math.min(repeater.count - 1, Math.floor(center / (tabWidth + Theme.tabGap))))
        if (target !== dragIndex) {
            const from = dragIndex
            dragIndex = target
            TabsController.moveTab(from, target)
        }
    }

    // ---- sizes ----
    readonly property real spaceForTabs: width - Theme.tabBarMargin - Theme.addTabSize - Theme.windowDragMinWidth
                                         - (windowButtons.visible ? windowButtons.width : 0)
    readonly property real tabWidth: Math.max(Theme.tabMinWidth,
                                              Math.min(Theme.tabMaxWidth,
                                                       (spaceForTabs - Theme.tabRadius * 2) / Math.max(1, repeater.count) - Theme.tabGap))

    function revealCurrent() {
        const item = repeater.itemAt(TabsController.currentIndex)
        if (!item) return
        const left = item.x + row.x - Theme.tabRadius
        const right = item.x + row.x + item.width + Theme.tabRadius
        if (left < strip.contentX) strip.contentX = left
        else if (right > strip.contentX + strip.width) strip.contentX = right - strip.width
    }
    Connections {
        target: TabsController
        function onCurrentIndexChanged() { Qt.callLater(root.revealCurrent) }
    }

    // ---- the free space after the tabs moves the window; a double click maximises it ----
    Item {
        anchors.left: addButton.right
        anchors.right: windowButtons.visible ? windowButtons.left : parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        DragHandler {
            target: null
            onActiveChanged: if (active) root.targetWindow.startSystemMove()
        }
        TapHandler {
            onDoubleTapped: root.targetWindow.visibility === Window.Maximized ? root.targetWindow.showNormal()
                                                                              : root.targetWindow.showMaximized()
        }
        FolderDropArea { id: spaceDrop }
    }

    // folders dropped on the free space or on "+" open in new tabs
    component FolderDropArea: DropArea {
        anchors.fill: parent
        onEntered: (drag) => { drag.accepted = drag.hasUrls }
        onDropped: (drop) => {
            drop.accept(Qt.CopyAction)
            TabsController.openFolders(drop.urls)
        }
    }

    // ---- the tabs ----
    Flickable {
        id: strip
        anchors.left: parent.left
        anchors.leftMargin: Theme.tabBarMargin - Theme.tabRadius
        anchors.bottom: parent.bottom
        width: Math.min(contentWidth, root.spaceForTabs)
        height: Theme.tabHeight
        contentWidth: row.width + 2 * Theme.tabRadius
        contentHeight: height
        flickableDirection: Flickable.HorizontalFlick
        boundsBehavior: Flickable.StopAtBounds
        interactive: false                 // the mouse drags tabs here; the wheel scrolls the strip
        clip: true

        Row {
            id: row
            x: Theme.tabRadius
            height: strip.height
            spacing: Theme.tabGap

            move: Transition { NumberAnimation { properties: "x"; duration: 160; easing.type: Easing.OutCubic } }

            Repeater {
                id: repeater
                model: TabsController
                onItemAdded: Qt.callLater(root.revealCurrent)

                delegate: BrowserTab {
                    id: tab
                    required property var session
                    required property bool isCurrent
                    required property int index

                    width: root.tabWidth
                    height: row.height
                    title: session.title
                    current: isCurrent
                    showDivider: !isCurrent && index !== TabsController.currentIndex - 1
                                 && index !== repeater.count - 1

                    onClicked: TabsController.selectTab(index)
                    onCloseRequested: TabsController.closeTab(index)
                    onUrlsDropped: (urls) => OperationsController.dropUrls(urls, session.path)

                    // while dragged the tab follows the mouse; when let go it slides into its place
                    readonly property bool beingDragged: root.dragIndex === index
                    property real settle: 0
                    z: beingDragged || settleAnimation.running ? 10 : current ? 5 : 0
                    transform: Translate { x: tab.beingDragged ? root.dragLeft - tab.x : tab.settle }
                    NumberAnimation { id: settleAnimation; target: tab; property: "settle"; to: 0; duration: 140; easing.type: Easing.OutCubic }

                    onDragStarted: (sceneX) => root.beginDrag(index, sceneX, tab)
                    onDragMoved: (sceneX) => root.dragTo(sceneX)
                    onDragEnded: {
                        tab.settle = root.dragLeft - tab.x
                        root.dragIndex = -1
                        settleAnimation.restart()
                    }
                }
            }
        }

        WheelHandler {
            acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
            onWheel: (event) => {
                const d = event.angleDelta.y !== 0 ? event.angleDelta.y : event.angleDelta.x
                const most = Math.max(0, strip.contentWidth - strip.width)
                strip.contentX = Math.max(0, Math.min(most, strip.contentX - d))
            }
        }
    }

    // ---- the "+" button sits right after the last tab ----
    Rectangle {
        id: addButton
        anchors.left: strip.right
        anchors.verticalCenter: strip.verticalCenter
        width: Theme.addTabSize
        height: Theme.addTabSize
        radius: Theme.controlRadius
        color: addArea.containsMouse || addDrop.containsDrag || spaceDrop.containsDrag ? Theme.hoverBg : "transparent"

        Text {
            anchors.centerIn: parent
            text: Icons.add
            font.family: Icons.fontFamily
            font.pixelSize: Theme.iconSize - 1
            color: Theme.text
        }
        MouseArea {
            id: addArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: TabsController.newTab()
        }
        FolderDropArea { id: addDrop }
    }

    WindowButtons {
        id: windowButtons
        targetWindow: root.targetWindow
        visible: SettingsController.windowButtonsVisible
        anchors.right: parent.right
        anchors.top: parent.top
        height: Theme.toolbarButtonSize
    }
}
