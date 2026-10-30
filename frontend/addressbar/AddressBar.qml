import QtQuick
import QtQuick.Controls.Basic

// The path as a row of clickable folders. Clicking the empty part (or Ctrl+L) turns it into a text field.
// When the path is too long, the folders that do not fit move into a "…" button at the left.
Rectangle {
    id: root

    property var session
    property bool editing: false
    property int firstShown: 0             // parts before this one are behind the "…" button
    readonly property var parts: session ? PathBarController.parts(session.path) : []
    signal finished()                      // the keyboard can go back to the file list

    function startEditing() {
        editing = true
        input.text = session.path
        input.selectAll()
        input.forceActiveFocus()
    }

    function stopEditing() {
        if (!editing) return
        editing = false
        suggestions.close()
        root.finished()
    }

    function commit() {
        const path = suggestions.highlighted >= 0 ? suggestions.folders[suggestions.highlighted].path
                                                  : PathBarController.resolve(input.text, session.path)
        stopEditing()
        session.navigate(path)
    }

    function updateSuggestions() {
        suggestions.highlighted = -1
        suggestions.folders = PathBarController.suggestions(input.text, session.path)
        if (suggestions.folders.length === 0) {
            suggestions.close()
            return
        }
        const corner = root.mapToItem(null, 0, root.height)
        suggestions.x = corner.x
        suggestions.y = corner.y + 2
        suggestions.open()
    }

    function moveHighlight(delta) {
        const count = suggestions.folders.length
        if (count === 0) return
        suggestions.highlighted = (suggestions.highlighted + delta + count + 1) % (count + 1)
        if (suggestions.highlighted === count) suggestions.highlighted = -1
    }

    // Tab: finish the name of the highlighted or only suggestion, and look inside it
    function completeSuggestion() {
        const count = suggestions.folders.length
        if (count === 0) return
        input.text = suggestions.folders[Math.max(0, suggestions.highlighted)].path + "/"
        updateSuggestions()
    }

    function updateFirstShown() {
        const available = crumbs.width
        let used = ellipsis.implicitWidth
        let first = repeater.count - 1
        for (let i = repeater.count - 1; i >= 0; --i) {
            const item = repeater.itemAt(i)
            if (!item) return
            if (used + item.implicitWidth > available) break
            used += item.implicitWidth
            first = i
        }
        firstShown = Math.max(0, first)
    }

    function showFolders(folders, sceneX, sceneY) {
        dropdown.folders = folders
        dropdown.x = sceneX
        dropdown.y = sceneY
        dropdown.open()
    }

    onPartsChanged: Qt.callLater(updateFirstShown)

    implicitHeight: Theme.addressHeight
    radius: Theme.controlRadius
    color: Theme.fieldBg
    border.color: editing ? Theme.accent : "transparent"

    // ---- the row of folders ----
    Item {
        id: crumbs
        anchors.fill: parent
        anchors.leftMargin: 4
        anchors.rightMargin: 4
        visible: !root.editing
        clip: true
        onWidthChanged: Qt.callLater(root.updateFirstShown)

        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.IBeamCursor
            onClicked: root.startEditing()
        }

        Row {
            height: parent.height

            PathSegment {
                id: ellipsis
                visible: root.firstShown > 0
                icon: Icons.more
                showArrow: false
                onClicked: {
                    const corner = mapToItem(null, 0, height)
                    root.showFolders(root.parts.slice(0, root.firstShown).reverse(), corner.x, corner.y)
                }
            }

            Repeater {
                id: repeater
                model: root.parts
                delegate: PathSegment {
                    required property var modelData
                    required property int index

                    visible: index >= root.firstShown
                    label: modelData.label
                    lastPart: index === root.parts.length - 1
                    onClicked: root.session.navigate(modelData.path)
                    onArrowClicked: (sceneX, sceneY) => {
                        root.showFolders(PathBarController.subfolders(modelData.path), sceneX, sceneY)
                    }
                }
                onItemAdded: Qt.callLater(root.updateFirstShown)
            }
        }
    }

    // ---- the text field ----
    TextInput {
        id: input
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        visible: root.editing
        clip: true
        selectByMouse: true
        font.pixelSize: Theme.fontSize
        color: Theme.text
        selectionColor: Theme.selectionBg
        selectedTextColor: Theme.text

        onTextEdited: root.updateSuggestions()
        onActiveFocusChanged: if (!activeFocus) root.stopEditing()
        Keys.onReturnPressed: root.commit()
        Keys.onEnterPressed: root.commit()
        Keys.onEscapePressed: root.stopEditing()
        Keys.onUpPressed: root.moveHighlight(-1)
        Keys.onDownPressed: root.moveHighlight(1)
        Keys.onTabPressed: root.completeSuggestion()
    }

    // ---- the lists that open under the bar ----
    FolderPopup {
        id: dropdown
        onChosen: (path) => root.session.navigate(path)
    }
    FolderPopup {
        id: suggestions
        focus: false
        closePolicy: Popup.NoAutoClose
        width: root.width
        onChosen: (path) => {
            root.stopEditing()
            root.session.navigate(path)
        }
    }
}
