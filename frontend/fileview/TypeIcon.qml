import QtQuick

// A file type's icon from the icon theme. `name` is empty when the theme has none: the row's own icon shows then.
Image {
    property string name: ""
    property real size: Theme.appIconSize

    visible: name !== ""
    width: size
    height: size
    sourceSize.width: 2 * size
    sourceSize.height: 2 * size
    fillMode: Image.PreserveAspectFit
    smooth: true
    asynchronous: true
    source: name !== "" ? "image://appicon/" + encodeURIComponent(name) : ""
}
