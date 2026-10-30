import QtQuick

// A title with a choice of buttons beside it. `choices` is a list of words; `current` is the chosen index.
Item {
    id: root

    property string title: ""
    property var choices: []
    property int current: 0
    signal chosen(int index)

    implicitHeight: Theme.settingsRowHeight

    Text {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        text: root.title
        font.pixelSize: Theme.fontSize
        color: Theme.text
    }
    Row {
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        spacing: 6
        Repeater {
            model: root.choices
            delegate: DialogButton {
                required property string modelData
                required property int index
                text: modelData
                primary: index === root.current
                onClicked: root.chosen(index)
            }
        }
    }
}
