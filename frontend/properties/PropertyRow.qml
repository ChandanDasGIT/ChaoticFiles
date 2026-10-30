import QtQuick

// One property: its name, and a value that can be selected. Hover to get a button that copies the value.
Item {
    id: root

    property string label: ""
    property string value: ""
    signal copyRequested()

    implicitHeight: Math.max(valueText.contentHeight, labelText.contentHeight) + Theme.propertyRowPadding

    HoverHandler { id: hover }

    Text {
        id: labelText
        width: Theme.propertyLabelWidth
        anchors.top: parent.top
        anchors.topMargin: Theme.propertyRowPadding / 2
        text: root.label
        textFormat: Text.PlainText
        wrapMode: Text.Wrap
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
    TextEdit {
        id: valueText
        anchors.left: labelText.right
        anchors.leftMargin: 12
        anchors.right: copyButton.left
        anchors.rightMargin: 4
        anchors.top: parent.top
        anchors.topMargin: Theme.propertyRowPadding / 2
        text: root.value
        textFormat: TextEdit.PlainText
        readOnly: true
        selectByMouse: true
        wrapMode: TextEdit.WrapAtWordBoundaryOrAnywhere
        font.pixelSize: Theme.smallFontSize
        color: Theme.text
        selectionColor: Theme.selectionBg
        selectedTextColor: Theme.text
    }
    ToolbarButton {
        id: copyButton
        anchors.right: parent.right
        anchors.top: parent.top
        implicitWidth: Theme.propertyCopySize
        implicitHeight: Theme.propertyCopySize
        icon: Icons.copy
        opacity: hover.hovered ? 1 : 0
        onClicked: root.copyRequested()
    }
}
