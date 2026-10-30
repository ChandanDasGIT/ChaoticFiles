import QtQuick

// Minimise, maximise and close, for desktops that do not manage those themselves.
Row {
    id: root

    required property var targetWindow
    readonly property bool maximized: targetWindow.visibility === Window.Maximized

    component WindowButton: Rectangle {
        id: button
        property string icon: ""
        property bool closing: false
        signal clicked()

        width: Theme.windowButtonWidth
        height: parent.height
        color: !area.containsMouse ? "transparent" : closing ? Theme.closeHoverBg : Theme.hoverBg

        Text {
            anchors.centerIn: parent
            text: button.icon
            font.family: Icons.fontFamily
            font.pixelSize: Theme.smallFontSize
            color: area.containsMouse && button.closing ? Theme.closeHoverText : Theme.text
        }
        MouseArea {
            id: area
            anchors.fill: parent
            hoverEnabled: true
            onClicked: button.clicked()
        }
    }

    WindowButton {
        icon: Icons.minimize
        onClicked: root.targetWindow.showMinimized()
    }
    WindowButton {
        icon: root.maximized ? Icons.restore : Icons.maximize
        onClicked: root.maximized ? root.targetWindow.showNormal() : root.targetWindow.showMaximized()
    }
    WindowButton {
        icon: Icons.close
        closing: true
        onClicked: root.targetWindow.close()
    }
}
