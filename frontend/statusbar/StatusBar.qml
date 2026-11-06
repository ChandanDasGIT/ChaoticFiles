import QtQuick

// The bottom line: how many items, what is selected, a passing message, and free disk space.
Rectangle {
    id: root

    property var session
    property string message: ""
    signal updateClicked()

    function showMessage(text) {
        message = text
        messageTimer.restart()
    }

    implicitHeight: Theme.statusBarHeight
    color: Theme.statusBarBg

    Timer {
        id: messageTimer
        interval: Theme.messageMs
        onTriggered: root.message = ""
    }

    // "Loading" appears only when a folder takes long enough to notice
    Timer {
        id: loadingTimer
        interval: Theme.loadingDelayMs
        running: root.session?.loading ?? false
        onTriggered: loadingText.visible = true
        onRunningChanged: if (!running) loadingText.visible = false
    }

    Row {
        anchors.left: parent.left
        anchors.leftMargin: Theme.statusPadding
        anchors.right: updateDot.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.statusGap
        clip: true

        Text {
            text: root.session?.itemsText ?? ""
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        Text {
            visible: text !== ""
            text: root.session?.selectionText ?? ""
            font.pixelSize: Theme.smallFontSize
            color: Theme.text
        }
        Text {
            id: loadingText
            visible: false
            text: "Loading…"
            font.pixelSize: Theme.smallFontSize
            color: Theme.textMuted
        }
        Text {
            textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
            visible: text !== ""
            text: root.message
            font.pixelSize: Theme.smallFontSize
            color: Theme.accent
        }
    }

    // a new version is out
    Item {
        id: updateDot
        objectName: "updateDot"
        anchors.right: disk.left
        width: UpdateController.available ? Theme.updateDotArea : 0
        height: parent.height
        visible: UpdateController.available

        Rectangle {
            anchors.centerIn: parent
            width: Theme.updateDotSize
            height: Theme.updateDotSize
            radius: width / 2
            color: Theme.updateDot
        }
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: root.updateClicked()
        }
    }

    Text {
        id: disk
        anchors.right: parent.right
        anchors.rightMargin: Theme.statusPadding
        anchors.verticalCenter: parent.verticalCenter
        text: root.session?.diskText ?? ""
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
}
