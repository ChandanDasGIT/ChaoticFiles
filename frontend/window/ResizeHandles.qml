import QtQuick

// Thin invisible strips along the window's edges. The app draws its own title bar, so it asks the
// window manager to start a resize when one is dragged.
Item {
    id: root

    required property var targetWindow

    component Handle: MouseArea {
        property int edges: 0
        hoverEnabled: true
        onPressed: root.targetWindow.startSystemResize(edges)
    }

    readonly property int border: Theme.resizeBorder

    Handle {
        edges: Qt.LeftEdge; cursorShape: Qt.SizeHorCursor
        x: 0; y: root.border; width: root.border; height: root.height - 2 * root.border
    }
    Handle {
        edges: Qt.RightEdge; cursorShape: Qt.SizeHorCursor
        x: root.width - root.border; y: root.border; width: root.border; height: root.height - 2 * root.border
    }
    Handle {
        edges: Qt.TopEdge; cursorShape: Qt.SizeVerCursor
        x: root.border; y: 0; width: root.width - 2 * root.border; height: root.border
    }
    Handle {
        edges: Qt.BottomEdge; cursorShape: Qt.SizeVerCursor
        x: root.border; y: root.height - root.border; width: root.width - 2 * root.border; height: root.border
    }
    Handle {
        edges: Qt.LeftEdge | Qt.TopEdge; cursorShape: Qt.SizeFDiagCursor
        x: 0; y: 0; width: root.border; height: root.border
    }
    Handle {
        edges: Qt.RightEdge | Qt.TopEdge; cursorShape: Qt.SizeBDiagCursor
        x: root.width - root.border; y: 0; width: root.border; height: root.border
    }
    Handle {
        edges: Qt.LeftEdge | Qt.BottomEdge; cursorShape: Qt.SizeBDiagCursor
        x: 0; y: root.height - root.border; width: root.border; height: root.border
    }
    Handle {
        edges: Qt.RightEdge | Qt.BottomEdge; cursorShape: Qt.SizeFDiagCursor
        x: root.width - root.border; y: root.height - root.border; width: root.border; height: root.border
    }
}
