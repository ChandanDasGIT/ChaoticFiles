import QtQuick

// What updating asks and says in a dialog. The questions come from UpdateController, and the answers go back.
Item {
    id: root

    signal closed()

    ConfirmDialog {
        id: prompt
        objectName: "updatePrompt"
        onClosed: root.closed()
    }
    Connections {
        target: UpdateController
        function onRestartWanted(operationsRunning) {
            const running = operationsRunning > 0
            prompt.ask({
                title: "Restart to update?",
                text: running ? "Files are still being copied or moved. The app can restart when they finish, and your tabs will open again."
                              : "ChaoticFiles will close and open again with the new version. Your tabs will open again.",
                confirmText: running ? "Restart when finished" : "Restart now",
                onConfirm: running ? UpdateController.restartWhenFinished : UpdateController.restartNow,
            })
        }
        function onRolledBack(text) {
            prompt.ask({title: "The update did not work", text: text, cancelText: ""})
        }
    }
}
