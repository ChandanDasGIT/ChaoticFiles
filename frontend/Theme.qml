pragma Singleton
import QtQuick

// Every colour and size of the app lives here. Use it anywhere as Theme.windowBg, Theme.accent, ...
// themeMode comes from the settings: 0 follows the system, 1 is dark, 2 is light.
QtObject {
    readonly property int mode: SettingsController.themeMode
    readonly property bool isDark: mode === 1 || (mode === 0 && SettingsController.systemDark)

    // ---- surfaces ----
    readonly property color windowBg:     isDark ? "#191919" : "#ffffff"   // the file list
    readonly property color tabBarBg:     isDark ? "#0f0f0f" : "#e4e7ec"   // the strip behind the tabs
    readonly property color toolbarBg:    isDark ? "#262626" : "#f6f7f9"   // the toolbar and the current tab, which flow into each other
    readonly property color sidebarBg:    isDark ? "#202020" : "#f3f4f6"
    readonly property color statusBarBg:  isDark ? "#202020" : "#f3f4f6"
    readonly property color fieldBg:      isDark ? "#1c1c1c" : "#ffffff"   // the address bar
    readonly property color menuBg:       isDark ? "#2c2c2c" : "#ffffff"
    readonly property color menuBorder:   isDark ? "#3f3f3f" : "#d4d7dc"
    readonly property color divider:      isDark ? "#333333" : "#e1e3e7"

    // ---- text ----
    readonly property color text:         isDark ? "#ededed" : "#1c1c1e"
    readonly property color textMuted:    isDark ? "#a8a8a8" : "#5f6368"
    readonly property color textDim:      isDark ? "#767676" : "#94979c"
    readonly property color textDisabled: isDark ? "#555555" : "#bcbec3"

    // ---- states ----
    readonly property color accent:       isDark ? "#60cdff" : "#005fb8"
    readonly property color hoverBg:      isDark ? "#14ffffff" : "#0d000000"
    readonly property color pressedBg:    isDark ? "#0dffffff" : "#16000000"
    readonly property color selectionBg:  isDark ? "#3a4a57" : "#cfe4f7"
    readonly property color selectionHoverBg: isDark ? "#445665" : "#bfdaf3"
    readonly property color danger:       isDark ? "#ff6b6b" : "#c42b1c"
    readonly property color closeHoverBg: "#c42b1c"
    readonly property color closeHoverText: "#ffffff"
    readonly property color scrollbarHandle:      isDark ? "#5c5c5c" : "#b5b8bd"
    readonly property color scrollbarHandleHover: isDark ? "#7a7a7a" : "#8f9298"

    // ---- icon colours, by kind of file ----
    readonly property color folderIcon:   isDark ? "#f3c551" : "#e0a526"
    function categoryColor(category) {
        switch (category) {
        case "folder":       return folderIcon
        case "text":         return isDark ? "#c8ccd2" : "#62666d"
        case "code":         return isDark ? "#7cc4ff" : "#1a73c8"
        case "image":        return isDark ? "#7fd18b" : "#23893a"
        case "audio":        return isDark ? "#d79bf0" : "#8a3fb0"
        case "video":        return isDark ? "#f08fa7" : "#c0335a"
        case "archive":      return isDark ? "#d9a66c" : "#9c6420"
        case "pdf":          return isDark ? "#f07c6c" : "#c7392a"
        case "document":     return isDark ? "#6ea8fe" : "#2b5fbf"
        case "spreadsheet":  return isDark ? "#6fd39b" : "#187a47"
        case "presentation": return isDark ? "#f4a261" : "#c4581c"
        case "program":      return isDark ? "#8fd6d0" : "#14807a"
        case "diskimage":    return isDark ? "#b4b9e8" : "#4f569f"
        case "font":         return isDark ? "#e8c07d" : "#94691c"
        default:             return textMuted
        }
    }

    // ---- fonts ----
    readonly property string fontFamily: Qt.application.font.family
    readonly property int fontSize:      13
    readonly property int smallFontSize: 12
    readonly property int iconSize:      14

    // ---- tab bar ----
    readonly property int tabBarHeight:  42
    readonly property int tabHeight:     36
    readonly property int tabRadius:     8
    readonly property int tabMinWidth:   110
    readonly property int tabMaxWidth:   240
    readonly property int tabPadding:    12
    readonly property int tabCloseSize:  22
    readonly property int tabGap:        2
    readonly property int tabBarMargin:  8
    readonly property int addTabSize:    30
    readonly property int windowDragMinWidth: 60     // this much of the strip stays free to move the window
    readonly property int windowButtonWidth:  44

    // ---- toolbar and address bar ----
    readonly property int toolbarHeight:   44
    readonly property int toolbarMargin:   8
    readonly property int toolbarButtonSize: 32
    readonly property int controlRadius:   6
    readonly property int addressHeight:   32
    readonly property int segmentPadding:  7
    readonly property int segmentArrowWidth: 18
    readonly property int popupRowHeight:  30
    readonly property int popupMaxRows:    12
    readonly property int popupWidth:      280

    // ---- sidebar ----
    readonly property int sidebarWidth:     210
    readonly property int sidebarRowHeight: 32
    readonly property int sidebarPadding:   8
    readonly property int sidebarHeaderHeight: 30

    // ---- file list ----
    readonly property int headerHeight:     30
    readonly property int rowHeight:        30
    readonly property int rowPadding:       12
    readonly property int rowIconWidth:     26
    readonly property int rowRadius:        4
    readonly property int rowMargin:        4
    readonly property int columnModified:   150
    readonly property int columnType:       170
    readonly property int columnSize:       100
    readonly property int wheelRows:        3
    readonly property int scrollbarWidth:   10
    readonly property real hiddenOpacity:   0.6

    // ---- status bar ----
    readonly property int statusBarHeight:  26
    readonly property int statusPadding:    12
    readonly property int statusGap:        16
    readonly property int messageMs:        4000
    readonly property int loadingDelayMs:   250

    // ---- menus ----
    readonly property int menuPadding:      4
    readonly property int menuRowHeight:    30
    readonly property int menuMinWidth:     210
    readonly property int menuRowPadding:   10
    readonly property int menuRadius:       8
    readonly property int menuShortcutSpace: 70     // room for a shortcut or check mark on the right

    // ---- window ----
    readonly property int windowWidth:      1120
    readonly property int windowHeight:     700
    readonly property int windowMinWidth:   560
    readonly property int windowMinHeight:  360
    readonly property int resizeBorder:     5
}
