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
    readonly property color updateDot:    isDark ? "#7fd18b" : "#23893a"   // a new version is out
    readonly property color accentFill:   isDark ? "#0067c0" : "#005fb8"   // a main button
    readonly property color dangerFill:   "#c42b1c"
    readonly property color textOnAccent:     "#ffffff"                         // text on those buttons
    readonly property color scrim:        isDark ? "#99000000" : "#55000000"
    readonly property color dialogBg:     isDark ? "#2b2b2b" : "#f9f9fa"
    readonly property color dialogFieldBg: isDark ? "#333333" : "#ffffff"
    readonly property color closeHoverBg: "#c42b1c"
    readonly property color closeHoverText: "#ffffff"
    readonly property color bandFill:     isDark ? "#3360cdff" : "#33005fb8"   // rubber band selection
    readonly property color scrollbarHandle:      isDark ? "#5c5c5c" : "#b5b8bd"
    readonly property color scrollbarHandleHover: isDark ? "#7a7a7a" : "#8f9298"

    // ---- icon colours, by kind of file ----
    readonly property color folderIcon:   isDark ? "#f3c551" : "#e0a526"
    readonly property var folderColorKeys: ["blue", "green", "red", "orange", "purple", "pink", "teal", "grey"]
    function folderColor(key) {
        switch (key) {
        case "blue":   return isDark ? "#6ea8fe" : "#2b5fbf"
        case "green":  return isDark ? "#7fd18b" : "#23893a"
        case "red":    return isDark ? "#f07c6c" : "#c7392a"
        case "orange": return isDark ? "#f4a261" : "#c4581c"
        case "purple": return isDark ? "#d79bf0" : "#8a3fb0"
        case "pink":   return isDark ? "#f08fa7" : "#c0335a"
        case "teal":   return isDark ? "#8fd6d0" : "#14807a"
        case "grey":   return isDark ? "#c8ccd2" : "#62666d"
        default:       return folderIcon
        }
    }
    function itemColor(category, colorKey) {
        return colorKey ? folderColor(colorKey) : categoryColor(category)
    }
    // a file with a tag: one colour per language, from the same set as the folders
    function tagColor(tag, category) {
        switch (tag) {
        case "CPP": case "CC": case "CXX": case "TS": case "TSX": return folderColor("blue")
        case "C": case "H": case "HPP": case "HH": case "HXX": case "GO": return folderColor("teal")
        case "QML": case "SH": case "BASH": case "ZSH": case "FISH": return folderColor("green")
        case "JS": case "MJS": case "JSX": case "PY": return folderColor("")
        case "RS": case "HTML": case "HTM": case "JAVA": return folderColor("orange")
        case "CSS": case "SCSS": case "SASS": case "LESS": case "PHP": return folderColor("purple")
        case "CMAK": case "MAKE": case "DOCK": return folderColor("red")
        case "RB": case "SWIF": case "VUE": return folderColor("pink")
        default: return categoryColor(category)
        }
    }
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
    readonly property string monoFontFamily: "monospace"
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
    readonly property int pinInsertEdge:    9       // the top and bottom of a pinned row: a drop here goes between rows
    readonly property int pinInsertLine:    2
    readonly property real pinDragOpacity:  0.8

    // ---- file list ----
    readonly property int headerHeight:     30
    readonly property int rowHeight:        30
    readonly property int columnFound:      460   // the matching line, when searching inside files
    function foundWidth(rowWidth) { return Math.min(columnFound, Math.round(rowWidth * 0.55)) }
    readonly property int rowPadding:       12
    readonly property int rowIconWidth:     26
    readonly property int rowArtSize:       20      // album art in a list row
    readonly property int fileTagWidth:     23
    readonly property int fileTagHeight:    13
    readonly property int fileTagFontSize:  7
    readonly property int fileTagSmallestFont: 5
    readonly property int fileTagRadius:    3
    readonly property int rowRadius:        4
    readonly property int rowMargin:        4
    readonly property int columnModified:   150
    readonly property int columnType:       170
    readonly property int columnSize:       100
    readonly property int wheelRows:        3
    readonly property int gridPadding:      8
    readonly property int gridTextHeight:   56
    readonly property int scrollbarWidth:   10
    readonly property real hiddenOpacity:   0.6

    // ---- preview panel ----
    readonly property int previewWidth:     300
    readonly property int previewPadding:   14

    // ---- status bar ----
    readonly property int statusBarHeight:  26
    readonly property int statusPadding:    12
    readonly property int statusGap:        16
    readonly property int updateDotSize:    10
    readonly property int updateDotArea:    26
    readonly property int messageMs:        4000
    readonly property int loadingDelayMs:   250

    // ---- menus ----
    readonly property int menuPadding:      4
    readonly property int menuRowHeight:    30
    readonly property int menuMinWidth:     210
    readonly property int menuRowPadding:   10
    readonly property int menuRadius:       8
    readonly property int menuShortcutSpace: 70     // room for a shortcut or check mark on the right

    // ---- search ----
    readonly property int searchBarHeight:  44
    readonly property int searchDelayMs:    300

    // ---- dialogs ----
    readonly property int dialogPadding:    18
    readonly property int dialogMargin:     24
    readonly property int dialogRadius:     10
    readonly property int dialogTitleSize:  16
    readonly property int dialogWidth:      420
    readonly property int dialogButtonHeight: 32
    readonly property int dialogButtonMinWidth: 88
    readonly property int dialogButtonPadding: 14
    readonly property int progressHeight:   6

    readonly property int conflictWidth:    500
    readonly property int openWithHeight:   460
    readonly property int appIconSize:      20
    readonly property int appSearchHeight:  32
    readonly property int defaultAppBoxWidth: 250
    readonly property int checkBoxSize:     16
    readonly property int operationsWidth:  340
    readonly property int operationHeight:  68
    readonly property real cutOpacity:      0.5
    readonly property int dragPreviewHeight: 34
    readonly property int dragPreviewPadding: 12
    readonly property int dragPreviewMaxWidth: 260
    readonly property int dragStackOffset:  5
    readonly property int dragHotSpot:      14
    readonly property int offScreen:        -100000  // far past any window edge

    // ---- settings ----
    readonly property int folderIconCell:   40
    readonly property int folderColorSize:  26
    readonly property int folderPreviewSize: 44
    readonly property int usageWidth:       640
    readonly property int usageHeight:      560
    readonly property int usageRowHeight:   34
    readonly property int usageBarHeight:   4
    readonly property int usageSizeWidth:   90
    readonly property int settingsWidth:    560
    readonly property int settingsHeight:   620
    readonly property int settingsRowHeight: 42
    readonly property int shortcutBoxWidth: 190

    // ---- properties ----
    readonly property int propertiesWidth:  700
    readonly property int propertiesHeight: 600
    readonly property int propertyLabelWidth: 170
    readonly property int propertyRowPadding: 10
    readonly property int propertyCopySize: 24
    readonly property int propertyTabHeight: 28
    readonly property int propertyTabPadding: 12

    // ---- window ----
    readonly property int windowWidth:      1120
    readonly property int windowHeight:     700
    readonly property int windowMinWidth:   560
    readonly property int windowMinHeight:  360
    readonly property int resizeBorder:     5
}
