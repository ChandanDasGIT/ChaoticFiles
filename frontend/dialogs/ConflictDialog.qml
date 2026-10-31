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
        CheckRow { id: applyAll; text: "Do this for all conflicts" }
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
