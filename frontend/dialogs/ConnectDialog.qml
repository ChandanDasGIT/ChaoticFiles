import QtQuick

// Connect to a NAS or server. The password is passed on and never stored.
DialogFrame {
    id: root

    property string problem: ""
    property bool working: false

    function ask() {
        problem = ""
        working = false
        address.text = ""
        user.text = ""
        password.text = ""
        open()
        address.forceActiveFocus()
    }

    function accept() {
        if (address.text === "" || working) return
        problem = ""
        working = true
        PlacesController.connectShare(address.text, user.text, password.text)
    }

    Connections {
        target: PlacesController
        function onConnectFinished(failure) {
            root.working = false
            root.problem = failure
            if (failure === "") root.close()
        }
    }

    component Field: Rectangle {
        id: field
        property alias text: input.text
        property alias echoMode: input.echoMode
        property string hint: ""
        function forceActiveFocus() { input.forceActiveFocus() }

        width: parent.width
        height: Theme.addressHeight
        radius: Theme.controlRadius
        color: Theme.dialogFieldBg
        border.color: input.activeFocus ? Theme.accent : Theme.menuBorder

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
            Keys.onReturnPressed: root.accept()
            Keys.onEnterPressed: root.accept()
        }
        Text {
            anchors.left: parent.left
            anchors.leftMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            visible: input.text === ""
            text: field.hint
            font.pixelSize: Theme.fontSize
            color: Theme.textDim
        }
    }

    width: Math.min(Theme.dialogWidth, parent.width - 2 * Theme.dialogMargin)

    Column {
        width: parent.width
        spacing: 10

        Text {
            text: "Connect to Server"
            font.pixelSize: Theme.dialogTitleSize
            font.weight: Font.DemiBold
            color: Theme.text
        }
        Field { id: address; hint: "smb://nas/share or sftp://user@server" }
        Field { id: user; hint: "User name (optional)" }
        Field { id: password; hint: "Password (optional)"; echoMode: TextInput.Password }
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
            DialogButton { text: "Cancel"; onClicked: root.close() }
            DialogButton { text: root.working ? "Connecting…" : "Connect"; primary: true; active: !root.working; onClicked: root.accept() }
        }
    }
}
