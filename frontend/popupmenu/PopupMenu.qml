import QtQuick

// A menu in the colours of the theme. Each entry of `entries` is one of:
//     {text, icon, shortcut, checked, enabled, action}     a row; action() runs when it is clicked
//     {header: "Theme"}                                    a small title
//     {separator: true}                                    a line
// It is a real desktop menu (ContextMenu, a QMenu), like the menus of other apps: it opens at the pointer, can
// reach past the edge of the window, and the desktop keeps it on screen. This file only hands over the rows
// and the theme, and runs the action of the row that was chosen.
Item {
    id: root

    property var entries: []
    signal closed()

    function openAt(sceneX, sceneY) {
        menu.popup(root, sceneX, sceneY, entries, {
            background: Theme.menuBg,
            border: Theme.menuBorder,
            textColor: Theme.text,
            iconColor: Theme.textMuted,
            headerColor: Theme.textDim,
            disabledColor: Theme.textDisabled,
            hoverColor: Theme.hoverBg,
            dividerColor: Theme.divider,
            radius: Theme.menuRadius,
            rowRadius: Theme.controlRadius,
            padding: Theme.menuPadding,
            rowHeight: Theme.menuRowHeight,
            rowPadding: Theme.menuRowPadding,
            minWidth: Theme.menuMinWidth,
            fontSize: Theme.fontSize,
            smallFontSize: Theme.smallFontSize,
            iconSize: Theme.iconSize,
            iconFont: Icons.fontFamily,
            checkGlyph: Icons.checkText,
        })
    }

    ContextMenu {
        id: menu
        onTriggered: (index) => {
            const entry = root.entries[index]
            if (entry && entry.action) entry.action()
        }
        onClosed: root.closed()
    }
}
