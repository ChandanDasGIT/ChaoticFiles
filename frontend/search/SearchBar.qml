import QtQuick

// Ctrl+F. Typing narrows the current folder at once; "All Subfolders" looks through everything below it,
// and "Inside Files" reads the text of the files below it.
Rectangle {
    id: root

    property var session
    property bool open: false
    property bool recursive: false
    property bool insideFiles: false
    readonly property bool walking: recursive || insideFiles        // slow, so it waits for a pause in typing
    signal finished()

    function start() {
        open = true
        input.forceActiveFocus()
        input.selectAll()
    }

    function stop() {
        if (!open) return
        open = false
        delay.stop()
        input.text = ""
        session.endSearch()
        finished()
    }

    function apply() {
        if (input.text === "") session.endSearch()
        else if (walking) session.search(input.text, insideFiles)
        else session.setFilter(input.text)
    }

    function changeMode() {
        if (!open) return
        session.endSearch()
        apply()
    }
    onRecursiveChanged: changeMode()
    onInsideFilesChanged: changeMode()
    onSessionChanged: { open = false; input.text = "" }

    Connections {
        target: root.session
        function onLocationChanged() { if (root.open) root.stop() }
    }

    implicitHeight: open ? Theme.searchBarHeight : 0
    visible: open
    color: Theme.toolbarBg

    Timer {
        id: delay
        interval: Theme.searchDelayMs
        onTriggered: root.apply()
    }

    Rectangle {
        anchors.left: parent.left
        anchors.leftMargin: Theme.toolbarMargin
        anchors.right: subfolders.left
        anchors.rightMargin: Theme.toolbarMargin
        anchors.verticalCenter: parent.verticalCenter
        height: Theme.addressHeight
        radius: Theme.controlRadius
        color: Theme.fieldBg
        border.color: Theme.accent

        Text {
            id: icon
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            text: Icons.search
            font.family: Icons.fontFamily
            font.pixelSize: Theme.iconSize - 1
            color: Theme.textMuted
        }
        TextInput {
            id: input
            anchors.left: icon.right
            anchors.leftMargin: 10
            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            clip: true
            selectByMouse: true
            font.pixelSize: Theme.fontSize
            color: Theme.text
            selectionColor: Theme.selectionBg
            selectedTextColor: Theme.text
            onTextEdited: root.walking ? delay.restart() : root.apply()
            Keys.onEscapePressed: root.stop()
            Keys.onDownPressed: root.finished()
            Keys.onReturnPressed: root.finished()
            Keys.onEnterPressed: root.finished()
        }
    }

    DialogButton {
        id: subfolders
        anchors.right: inside.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: "All Subfolders"
        primary: root.recursive
        onClicked: {
            root.insideFiles = false
            root.recursive = !root.recursive
        }
    }
    DialogButton {
        id: inside
        anchors.right: closeButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        text: "Inside Files"
        primary: root.insideFiles
        onClicked: {
            root.recursive = false
            root.insideFiles = !root.insideFiles
        }
    }
    ToolbarButton {
        id: closeButton
        anchors.right: parent.right
        anchors.rightMargin: Theme.toolbarMargin
        anchors.verticalCenter: parent.verticalCenter
        icon: Icons.close
        onClicked: root.stop()
    }
}
