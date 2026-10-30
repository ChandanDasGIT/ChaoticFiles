import QtQuick
import QtQuick.Controls.Basic

// "This name already exists": asked by a running copy or move, which waits for the answer.
DialogFrame {
    id: root

    closePolicy: Popup.NoAutoClose          // an answer is needed
    visible: OperationsController.hasConflict
    width: Math.min(Theme.conflictWidth, parent.width - 2 * Theme.dialogMargin)

    Column {
        width: parent.width
        spacing: 12

        Text {
            width: parent.width
            text: "\"" + OperationsController.conflictName + "\" already exists"
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.dialogTitleSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            width: parent.width
            text: "Existing: " + OperationsController.conflictTarget + "\nNew: " + OperationsController.conflictSource
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.fontSize
            color: Theme.textMuted
        }
        Row {
            spacing: 8
            Rectangle {
                width: 16
                height: 16
                radius: 4
                anchors.verticalCenter: parent.verticalCenter
                color: applyAll.checked ? Theme.accentFill : "transparent"
                border.color: Theme.menuBorder
                Text {
                    anchors.centerIn: parent
                    visible: applyAll.checked
                    text: Icons.check
                    font.family: Icons.fontFamily
                    font.pixelSize: 10
                    color: Theme.textOnAccent
                }
            }
            Text {
                text: "Do this for all conflicts"
                font.pixelSize: Theme.fontSize
                color: Theme.text
            }
            TapHandler { id: applyAll; property bool checked: false; onTapped: checked = !checked }
        }
        Row {
            anchors.right: parent.right
            spacing: 8
            DialogButton { text: "Cancel"; onClicked: OperationsController.answerConflict(3, false) }
            DialogButton { text: "Skip"; onClicked: OperationsController.answerConflict(1, applyAll.checked) }
            DialogButton { text: "Keep Both"; primary: true; onClicked: OperationsController.answerConflict(2, applyAll.checked) }
            DialogButton { text: "Replace"; danger: true; onClicked: OperationsController.answerConflict(0, applyAll.checked) }
        }
    }
}
