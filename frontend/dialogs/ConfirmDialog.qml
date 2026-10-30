import QtQuick

// A question with Cancel and one other button. Also used as a plain notice: leave `cancelText` empty.
DialogFrame {
    id: root

    property string title: ""
    property string text: ""
    property string confirmText: "OK"
    property string cancelText: "Cancel"
    property bool danger: false
    property var onConfirm: null

    function ask(settings) {
        title = settings.title
        text = settings.text ?? ""
        confirmText = settings.confirmText ?? "OK"
        cancelText = settings.cancelText ?? "Cancel"
        danger = settings.danger ?? false
        onConfirm = settings.onConfirm ?? null
        open()
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)

    Column {
        width: parent.width
        spacing: 14

        Text {
            width: parent.width
            text: root.title
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.dialogTitleSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Text {
            width: parent.width
            visible: text !== ""
            text: root.text
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            maximumLineCount: 14
            elide: Text.ElideRight
            font.pixelSize: Theme.fontSize
            color: Theme.textMuted
        }
        Row {
            anchors.right: parent.right
            spacing: 8

            DialogButton {
                visible: root.cancelText !== ""
                text: root.cancelText
                onClicked: root.close()
            }
            DialogButton {
                text: root.confirmText
                primary: !root.danger
                danger: root.danger
                onClicked: {
                    const confirm = root.onConfirm
                    root.close()
                    if (confirm) confirm()
                }
            }
        }
    }
}
