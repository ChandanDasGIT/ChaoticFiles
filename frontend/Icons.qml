pragma Singleton
import QtQuick

// Every icon of the app is a glyph of one icon font (Font Awesome 6 Free, Solid).
// Each icon is written as a unicode escape: "\uf002" is the magnifying glass.
// Use it anywhere like this:
//     Text { text: Icons.folder; font.family: Icons.fontFamily }
// To add an icon: look up its code on fontawesome.com and add a line below, for example
//     readonly property string cloud: "\uf0c2"   // cloud
// Only SOLID icons of the free set exist in frontend/fonts/fa-solid-900.ttf.
QtObject {
    // The font file is a resource of the app (see RESOURCES in CMakeLists.txt).
    readonly property FontLoader loader: FontLoader { source: "fonts/fa-solid-900.ttf" }
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
    readonly property string settings:      "\uf013"   // gear

    // ---- menus ----
    readonly property string check:         "\uf00c"   // check
    readonly property string copy:          "\uf0c5"   // copy
    readonly property string open:          "\uf07c"   // folder-open
    readonly property string newTab:        "\uf0fe"   // square-plus
    readonly property string eye:           "\uf06e"   // eye
    readonly property string warning:       "\uf071"   // triangle-exclamation
    readonly property string lock:          "\uf023"   // lock
    readonly property string link:          "\uf0c1"   // link

    // ---- places ----
    readonly property string home:          "\uf015"   // house
    readonly property string desktop:       "\uf390"   // desktop
    readonly property string documents:     "\uf15c"   // file-lines
    readonly property string downloads:     "\uf019"   // download
    readonly property string music:         "\uf001"   // music
    readonly property string pictures:      "\uf03e"   // image
    readonly property string videos:        "\uf03d"   // video
    readonly property string drive:         "\uf0a0"   // hard-drive
    readonly property string root:          "\uf233"   // server

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
        default:          return drive
        }
    }
}
