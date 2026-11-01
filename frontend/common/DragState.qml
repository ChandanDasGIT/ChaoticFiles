pragma Singleton
import QtQuick

// What is being dragged out of a file list right now, for the parts of the window that react to it.
QtObject {
    property bool active: false
    property bool hasFolders: false

    // a pinned folder being moved inside the sidebar
    readonly property string pinKey: "chaoticfiles-pin"
    property string pinPath: ""
}
