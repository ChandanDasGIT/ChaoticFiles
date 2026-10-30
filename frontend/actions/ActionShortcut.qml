import QtQuick

// A shortcut whose keys come from the settings.
Shortcut {
    required property string name
    sequences: ShortcutsController.keys[name] ?? []
}
