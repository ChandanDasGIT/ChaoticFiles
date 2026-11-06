pragma Singleton
import QtQuick

// Every icon of the app is a glyph of one icon font (Font Awesome 6 Free, Solid).
// Each icon is written as a unicode escape: "\uf002" is the magnifying glass.
// Use it anywhere like this:
//     Text { text: Icons.folder; font.family: Icons.fontFamily }
// To add an icon: look up its code on fontawesome.com and add a line below, for example
//     readonly property string cloud: "\uf0c2"   // cloud
// Only SOLID icons of the free set exist in resources/fonts/fa-solid-900.ttf.
QtObject {
    // The font file is a resource of the app (see RESOURCES in CMakeLists.txt).
    readonly property FontLoader loader: FontLoader { source: "../resources/fonts/fa-solid-900.ttf" }
    readonly property string fontFamily: loader.name

    // ---- navigation ----
    readonly property string back:          "\uf060"   // arrow-left
    readonly property string forward:       "\uf061"   // arrow-right
    readonly property string up:            "\uf062"   // arrow-up
    readonly property string refresh:       "\uf2f9"   // rotate-right
    readonly property string chevronRight:  "\uf054"   // chevron-right
    readonly property string chevronDown:   "\uf078"   // chevron-down
    readonly property string caretUp:       "\uf0d8"   // caret-up
    readonly property string caretDown:     "\uf0d7"   // caret-down
    readonly property string more:          "\uf141"   // ellipsis

    // ---- tabs and window ----
    readonly property string add:           "\uf067"   // plus
    readonly property string close:         "\uf00d"   // xmark
    readonly property string minimize:      "\uf068"   // minus
    readonly property string maximize:      "\uf0c8"   // square
    readonly property string restore:       "\uf2d2"   // window-restore
    readonly property string grid:          "\uf00a"   // table-cells
    readonly property string list:          "\uf03a"   // list
    readonly property string preview:       "\uf2d0"   // window-maximize
    readonly property string settings:      "\uf013"   // gear

    // ---- menus ----
    readonly property string check:         "\uf00c"   // check
    readonly property string checkText:     "\u2713"   // a check mark in the ordinary font, for the right side of a menu row
    readonly property string copy:          "\uf0c5"   // copy
    readonly property string open:          "\uf07c"   // folder-open
    readonly property string newTab:        "\uf0fe"   // square-plus
    readonly property string eye:           "\uf06e"   // eye
    readonly property string warning:       "\uf071"   // triangle-exclamation
    readonly property string info:          "\uf05a"   // circle-info
    readonly property string cut:           "\uf0c4"   // scissors
    readonly property string paste:         "\uf0ea"   // paste
    readonly property string rename:        "\uf304"   // pen
    readonly property string undo:          "\uf0e2"   // arrow-rotate-left
    readonly property string restoreItem:      "\uf1da"   // clock-rotate-left
    readonly property string archive:       "\uf187"   // box-archive
    readonly property string extract:       "\uf466"   // box-open
    readonly property string terminal:      "\uf120"   // terminal
    readonly property string newFolder:     "\uf65e"   // folder-plus
    readonly property string newFile:       "\uf15b"   // file
    readonly property string play:          "\uf04b"   // play
    readonly property string pause:         "\uf04c"   // pause
    readonly property string lock:          "\uf023"   // lock
    readonly property string link:          "\uf0c1"   // link
    readonly property string palette:       "\uf53f"   // palette

    // ---- places ----
    readonly property string home:          "\uf015"   // house
    readonly property string desktop:       "\uf390"   // desktop
    readonly property string documents:     "\uf15c"   // file-lines
    readonly property string downloads:     "\uf019"   // download
    readonly property string music:         "\uf001"   // music
    readonly property string pictures:      "\uf03e"   // image
    readonly property string videos:        "\uf03d"   // video
    readonly property string drive:         "\uf0a0"   // hard-drive
    readonly property string phone:         "\uf3cd"   // mobile-screen-button
    readonly property string recent:        "\uf017"   // clock
    readonly property string pin:           "\uf08d"   // thumbtack
    readonly property string search:        "\uf002"   // magnifying-glass
    readonly property string eject:         "\uf052"   // eject
    readonly property string network:       "\uf6ff"   // network-wired
    readonly property string root:          "\uf233"   // server
    readonly property string trash:         "\uf1f8"   // trash

    // ---- kinds of file ----
    readonly property string folder:        "\uf07b"   // folder
    readonly property string file:          "\uf15b"   // file
    readonly property string fileText:      "\uf15c"   // file-lines
    readonly property string fileCode:      "\uf1c9"   // file-code
    readonly property string fileImage:     "\uf1c5"   // file-image
    readonly property string fileAudio:     "\uf1c7"   // file-audio
    readonly property string fileVideo:     "\uf1c8"   // file-video
    readonly property string fileArchive:   "\uf1c6"   // file-zipper
    readonly property string filePdf:       "\uf1c1"   // file-pdf
    readonly property string fileDocument:  "\uf1c2"   // file-word
    readonly property string fileSheet:     "\uf1c3"   // file-excel
    readonly property string fileSlides:    "\uf1c4"   // file-powerpoint
    readonly property string fileProgram:   "\uf120"   // terminal
    readonly property string fileDisk:      "\uf51f"   // compact-disc
    readonly property string fileFont:      "\uf031"   // font
    readonly property string fileSystem:    "\uf085"   // gears
    readonly property string diskUsage:     "\uf200"   // chart-pie

    // ---- icons a folder can be given; the key is what is saved on the folder ----
    readonly property var folderChoices: [
        {key: "star", glyph: "\uf005"}, {key: "heart", glyph: "\uf004"}, {key: "music", glyph: "\uf001"},
        {key: "image", glyph: "\uf03e"}, {key: "video", glyph: "\uf03d"}, {key: "camera", glyph: "\uf030"},
        {key: "gamepad", glyph: "\uf11b"}, {key: "code", glyph: "\uf121"}, {key: "terminal", glyph: "\uf120"},
        {key: "book", glyph: "\uf02d"}, {key: "school", glyph: "\uf19d"}, {key: "work", glyph: "\uf0b1"},
        {key: "money", glyph: "\uf155"}, {key: "home", glyph: "\uf015"}, {key: "people", glyph: "\uf0c0"},
        {key: "download", glyph: "\uf019"}, {key: "cloud", glyph: "\uf0c2"}, {key: "globe", glyph: "\uf0ac"},
        {key: "lock", glyph: "\uf023"}, {key: "archive", glyph: "\uf187"}, {key: "tools", glyph: "\uf0ad"},
        {key: "bolt", glyph: "\uf0e7"}, {key: "rocket", glyph: "\uf135"}, {key: "palette", glyph: "\uf53f"},
        {key: "flask", glyph: "\uf0c3"}, {key: "leaf", glyph: "\uf06c"},
    ]

    function forFolder(key) {
        const choice = folderChoices.find(item => item.key === key)
        return choice ? choice.glyph : folder
    }

    function forItem(category, iconKey) {
        return iconKey ? forFolder(iconKey) : forCategory(category)
    }

    function forCategory(category) {
        switch (category) {
        case "folder":       return folder
        case "text":         return fileText
        case "code":         return fileCode
        case "image":        return fileImage
        case "audio":        return fileAudio
        case "video":        return fileVideo
        case "archive":      return fileArchive
        case "pdf":          return filePdf
        case "document":     return fileDocument
        case "spreadsheet":  return fileSheet
        case "presentation": return fileSlides
        case "program":      return fileProgram
        case "diskimage":    return fileDisk
        case "font":         return fileFont
        case "system":       return fileSystem
        default:             return file
        }
    }

    function forPlace(kind) {
        switch (kind) {
        case "home":      return home
        case "desktop":   return desktop
        case "documents": return documents
        case "downloads": return downloads
        case "music":     return music
        case "pictures":  return pictures
        case "videos":    return videos
        case "root":      return root
        case "trash":     return trash
        case "recent":    return recent
        case "bookmark":  return folder
        case "network":   return network
        case "phone":     return phone
        default:          return drive
        }
    }
}
