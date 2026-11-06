import QtQuick

// Asks for a name: new folder, new file, rename, compress. The part before the extension starts selected.
// `extensions`, if given, shows a button for each, which swaps the end of the name.
DialogFrame {
    id: root

    property string title: ""
    property string confirmText: "OK"
    property var extensions: []
    property string currentExtension: ""
    property string problem: ""
    property var onConfirm: null             // gets the name; returns a sentence if it failed

    function ask(settings) {
        title = settings.title
        confirmText = settings.confirmText ?? "OK"
        extensions = settings.extensions ?? []
        currentExtension = extensions.length > 0 ? extensions[0] : ""
        onConfirm = settings.onConfirm
        problem = ""
        input.text = settings.text ?? ""
        open()
        Qt.callLater(selectStem)
    }

    function selectStem() {
        input.forceActiveFocus()
        const dot = input.text.indexOf(".", 1)
        input.select(0, dot > 0 && !input.text.startsWith(".") ? dot : input.text.length)
    }

    function swapExtension(extension) {
        if (input.text.endsWith(currentExtension))
            input.text = input.text.slice(0, input.text.length - currentExtension.length) + extension
        currentExtension = extension
    }

    function accept() {
        problem = OperationsController.nameProblem(input.text)
        if (problem !== "") return
        const failure = root.onConfirm(input.text)
        if (failure) problem = failure
        else close()
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)

    Column {
        width: parent.width
        spacing: 12

        Text {
            textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
            text: root.title
            font.pixelSize: Theme.dialogTitleSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Rectangle {
            width: parent.width
            height: Theme.addressHeight
            radius: Theme.controlRadius
            color: Theme.dialogFieldBg
            border.color: root.problem !== "" ? Theme.danger : Theme.accent

            TextInput {
                id: input
                anchors.fill: parent
                anchors.leftMargin: 10
                anchors.rightMargin: 10
                verticalAlignment: TextInput.AlignVCenter
                clip: true
                selectByMouse: true
                font.pixelSize: Theme.fontSize
                color: Theme.text
                selectionColor: Theme.selectionBg
                selectedTextColor: Theme.text
                onTextEdited: root.problem = ""
                Keys.onReturnPressed: root.accept()
                Keys.onEnterPressed: root.accept()
            }
        }
        Row {
            visible: root.extensions.length > 0
            spacing: 6
            Repeater {
                model: root.extensions
                delegate: DialogButton {
                    required property string modelData
                    text: modelData
                    primary: modelData === root.currentExtension
                    onClicked: root.swapExtension(modelData)
                }
            }
        }
        Text {
            width: parent.width
            visible: root.problem !== ""
            text: root.problem
            textFormat: Text.PlainText
            wrapMode: Text.Wrap
            font.pixelSize: Theme.smallFontSize
            color: Theme.danger
        }
        Row {
            anchors.right: parent.right
            spacing: 8
            DialogButton {
                text: "Cancel"
                onClicked: root.close()
            }
            DialogButton {
                text: root.confirmText
                primary: true
                onClicked: root.accept()
            }
        }
    }
}
