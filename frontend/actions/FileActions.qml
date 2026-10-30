import QtQuick

// Every action on files, in one place: what the shortcuts and the menus call. It holds the dialogs those
// actions need. The work itself happens in OperationsController.
Item {
    id: root

    property var session
    signal dialogClosed()

    function open() { session.openCurrent() }

    function properties() { PropertiesController.show(session.selectedPaths()) }

    function copy() {
        const items = session.selectedItems()
        if (items.length > 0) ClipboardController.copy(items)
    }

    function cut() {
        const items = session.selectedItems()
        if (items.length > 0 && !session.isTrash) ClipboardController.cut(items)
    }

    function paste() {
        if (session.canWrite) ClipboardController.paste(session.path)
    }

    function newFolder() {
        if (!session.canWrite) return
        nameDialog.ask({
            title: "New Folder", confirmText: "Create", text: OperationsController.freeName(session.path, "New Folder"),
            onConfirm: (name) => {
                const failure = OperationsController.newFolder(session.path, name)
                if (!failure) session.refreshAndSelect(name)
                return failure
            }
        })
    }

    function newFile() {
        if (!session.canWrite) return
        nameDialog.ask({
            title: "New File", confirmText: "Create", text: OperationsController.freeName(session.path, "New File.txt"),
            onConfirm: (name) => {
                const failure = OperationsController.newFile(session.path, name)
                if (!failure) session.refreshAndSelect(name)
                return failure
            }
        })
    }

    function rename() {
        const items = session.selectedItems()
        if (items.length !== 1 || session.isTrash) return
        const path = items[0]
        const name = path.slice(path.lastIndexOf("/") + 1)
        nameDialog.ask({
            title: "Rename", confirmText: "Rename", text: name,
            onConfirm: (newName) => {
                const failure = OperationsController.rename(path, newName)
                if (!failure) session.refreshAndSelect(newName)
                return failure
            }
        })
    }

    function moveToTrash() {
        const items = session.selectedItems()
        if (items.length === 0) return
        if (session.isTrash) deletePermanently()
        else OperationsController.trash(items)
    }

    function deletePermanently() {
        const items = session.selectedItems()
        if (items.length === 0) return
        confirmDelete(items)
    }

    function confirmDelete(items) {
        const name = items[0].slice(items[0].lastIndexOf("/") + 1)
        confirm.ask({
            title: items.length === 1 ? "Delete \"" + name + "\" permanently?" : "Delete " + items.length + " items permanently?",
            text: "This can't be undone.", confirmText: "Delete", danger: true,
            onConfirm: () => OperationsController.deleteForever(items)
        })
    }

    function restore() {
        OperationsController.restore(session.selectedItems())
    }

    function emptyTrash() {
        confirm.ask({
            title: "Empty the Trash?", text: "Everything in the Trash will be deleted permanently. This can't be undone.",
            confirmText: "Empty Trash", danger: true, onConfirm: () => OperationsController.emptyTrash()
        })
    }

    function compress() {
        const items = session.selectedItems()
        if (items.length === 0) return
        const first = items[0].slice(items[0].lastIndexOf("/") + 1)
        const stem = items.length === 1 ? first : "Archive"
        nameDialog.ask({
            title: "Compress", confirmText: "Create", extensions: [".tar.gz", ".tar.xz", ".tar.zst", ".zip", ".7z"],
            text: stem + ".tar.gz",
            onConfirm: (name) => {
                OperationsController.compress(items, session.path + "/" + OperationsController.freeName(session.path, name))
                return ""
            }
        })
    }

    function extract() { OperationsController.extract(session.selectedItems()) }

    function createLinks() { OperationsController.createLinks(session.selectedItems()) }

    function openWith() {
        const items = session.selectedItems()
        if (items.length > 0) openWithDialog.ask(items)
    }

    function openTerminal() {
        if (session.canWrite && !AppsController.openTerminal(session.path))
            notice.ask({title: "No terminal was found", text: "Set the TERMINAL environment variable to your terminal app.", cancelText: ""})
    }

    function connectToServer() { connectDialog.ask() }

    function showError(title, text) { notice.ask({title: title, text: text, cancelText: ""}) }

    Connections {
        target: OperationsController
        function onErrors(title, text) { root.showError(title, text) }
        function onTrashFailed(paths, reason) {
            confirm.ask({
                title: "Delete permanently instead?", text: reason + "\n\nDeleting can't be undone.",
                confirmText: "Delete", danger: true, onConfirm: () => OperationsController.deleteForever(paths)
            })
        }
    }

    NameDialog { id: nameDialog; onClosed: root.dialogClosed() }
    ConfirmDialog { id: confirm; onClosed: root.dialogClosed() }
    ConfirmDialog { id: notice; onClosed: root.dialogClosed() }
    OpenWithDialog { id: openWithDialog; onClosed: root.dialogClosed() }
    ConnectDialog { id: connectDialog; onClosed: root.dialogClosed() }
    ConflictDialog { onClosed: root.dialogClosed() }
}
