import QtQuick

// The end of the settings: which version this is, and updates. Only draws UpdateController's state.
Column {
    id: root

    spacing: 4

    Item {
        width: parent.width
        implicitHeight: Theme.settingsRowHeight
        height: implicitHeight

        Text {
            anchors.left: parent.left
            anchors.verticalCenter: parent.verticalCenter
            text: "Version"
            font.pixelSize: Theme.fontSize
            color: Theme.text
        }
        Text {
            objectName: "aboutVersion"
            anchors.right: parent.right
            anchors.verticalCenter: parent.verticalCenter
            text: UpdateController.version
            font.pixelSize: Theme.fontSize
            color: Theme.textMuted
        }
    }

    SettingsSection {
        objectName: "autoCheckRow"
        width: parent.width
        title: "Check for updates automatically"
        choices: ["Off", "On"]
        current: UpdateController.autoCheck ? 1 : 0
        onChosen: (index) => UpdateController.autoCheck = index === 1
    }

    Text {
        objectName: "updateStatus"
        width: parent.width
        visible: text !== ""
        topPadding: 6
        bottomPadding: 6
        text: UpdateController.status !== "" ? UpdateController.status
              : UpdateController.available ? "Version " + UpdateController.latestVersion + " is available." : ""
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        font.pixelSize: Theme.fontSize
        color: Theme.textMuted
    }

    Flow {
        width: parent.width
        spacing: 6
        bottomPadding: 8

        DialogButton {
            objectName: "checkNowButton"
            visible: !UpdateController.available
            active: !UpdateController.busy
            text: "Check now"
            onClicked: UpdateController.checkNow()
        }
        DialogButton {
            objectName: "updateNowButton"
            visible: UpdateController.available && UpdateController.canInstall && !UpdateController.waitingForOperations
            active: !UpdateController.busy
            primary: true
            text: UpdateController.readyToRestart ? "Restart to update" : "Update now"
            onClicked: UpdateController.updateNow()
        }
        DialogButton {
            objectName: "cancelRestartButton"
            visible: UpdateController.waitingForOperations
            text: "Don't restart"
            onClicked: UpdateController.cancelRestart()
        }
        DialogButton {
            objectName: "releasePageButton"
            visible: UpdateController.available && !UpdateController.canInstall
            primary: true
            text: "Open release page"
            onClicked: UpdateController.openReleasePage()
        }
    }
}
