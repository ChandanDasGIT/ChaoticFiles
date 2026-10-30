import QtQuick

// One shortcut in the settings. Click it, then press the new keys. Escape cancels, Backspace removes the key.
Item {
    id: root

    property string actionId: ""
    property string title: ""
    property string keys: ""
    property bool capturing: false
    signal captureChanged(bool active)

    implicitHeight: Theme.popupRowHeight

    function stopCapture() {
        capturing = false
        root.captureChanged(false)
    }

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
    Rectangle {
        id: box
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        width: Theme.shortcutBoxWidth
        height: Theme.popupRowHeight - 4
        radius: Theme.controlRadius
        color: Theme.dialogFieldBg
        border.color: root.capturing ? Theme.accent : Theme.menuBorder
        focus: root.capturing

        Text {
            anchors.centerIn: parent
            width: parent.width - 12
            horizontalAlignment: Text.AlignHCenter
            elide: Text.ElideRight
            text: root.capturing ? "Press keys…" : root.keys !== "" ? root.keys : "None"
            font.pixelSize: Theme.smallFontSize
            color: root.keys !== "" || root.capturing ? Theme.text : Theme.textDim
        }
        MouseArea {
            anchors.fill: parent
            onClicked: {
                root.capturing = true
                root.captureChanged(true)
                box.forceActiveFocus()
            }
        }
        Keys.onPressed: (event) => {
            event.accepted = true
            if (!root.capturing) return
            if (event.key === Qt.Key_Escape) {
                root.stopCapture()
            } else if (event.key === Qt.Key_Backspace && event.modifiers === Qt.NoModifier) {
                ShortcutsController.set(root.actionId, "")
                root.stopCapture()
            } else {
                const sequence = ShortcutsController.sequenceFor(event.key, event.modifiers)
                if (sequence === "") return
                ShortcutsController.set(root.actionId, sequence)
                root.stopCapture()
            }
        }
    }
}
