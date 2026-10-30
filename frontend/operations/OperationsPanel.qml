import QtQuick

// The running copies, moves and so on, in the bottom right corner, each with pause and cancel.
Column {
    id: root

    spacing: 6
    width: Theme.operationsWidth

    Repeater {
        model: OperationsController
        delegate: Rectangle {
            id: card
            required property int operationId
            required property string title
            required property string detail
            required property double progress
            required property bool paused
            required property bool canPause

            width: root.width
            height: Theme.operationHeight
            radius: Theme.dialogRadius
            color: Theme.dialogBg
            border.color: Theme.menuBorder

            Column {
                anchors.left: parent.left
                anchors.leftMargin: 12
                anchors.right: buttons.left
                anchors.rightMargin: 8
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                Text {
                    width: parent.width
                    text: card.paused ? card.title + " (paused)" : card.title
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font.pixelSize: Theme.smallFontSize
                    color: Theme.text
                }
                Text {
                    width: parent.width
                    text: card.detail
                    textFormat: Text.PlainText
                    elide: Text.ElideMiddle
                    font.pixelSize: Theme.smallFontSize - 1
                    color: Theme.textMuted
                }
                Rectangle {
                    width: parent.width
                    height: Theme.progressHeight
                    radius: height / 2
                    color: Theme.dialogFieldBg
                    Rectangle {
                        width: parent.width * Math.min(1, card.progress)
                        height: parent.height
                        radius: parent.radius
                        color: Theme.accent
                    }
                }
            }
            Row {
                id: buttons
                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter

                ToolbarButton {
                    visible: card.canPause
                    icon: card.paused ? Icons.play : Icons.pause
                    onClicked: OperationsController.pause(card.operationId)
                }
                ToolbarButton {
                    icon: Icons.close
                    onClicked: OperationsController.cancel(card.operationId)
                }
            }
        }
    }
}
