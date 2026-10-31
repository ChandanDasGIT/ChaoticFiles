import QtQuick

// The icon of an installed app. `name` is its Icon= value; without one it gets the system's generic app icon.
Image {
    property string name: ""

    width: Theme.appIconSize
    height: Theme.appIconSize
    sourceSize.width: 2 * Theme.appIconSize
    sourceSize.height: 2 * Theme.appIconSize
    fillMode: Image.PreserveAspectFit
    smooth: true
    source: "image://appicon/" + encodeURIComponent(name !== "" ? name : "application-x-executable")
}
