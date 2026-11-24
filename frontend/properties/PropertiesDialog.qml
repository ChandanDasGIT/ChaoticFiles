import QtQuick
import QtQuick.Controls.Basic

// The properties window. The tabs come from PropertiesController; Hashes is added for files, before Advanced.
DialogFrame {
    id: root

    property int currentTab: 0
    readonly property var sections: PropertiesController.sections
    readonly property var tabNames: {
        const names = sections.map(section => section.title)
        if (PropertiesController.canHash) names.splice(names[names.length - 1] === "Advanced" ? names.length - 1 : names.length, 0, "Hashes")
        return names
    }
    readonly property string currentName: tabNames[currentTab] ?? ""
    readonly property bool hashesTab: currentName === "Hashes" && PropertiesController.canHash
    readonly property var currentRows: hashesTab ? PropertiesController.hashes
                                                 : (sections.find(section => section.title === currentName)?.rows ?? [])

    visible: PropertiesController.isOpen
    onClosed: PropertiesController.close()
    onOpened: currentTab = 0

    width: Math.min(Theme.propertiesWidth, maxWidth)
    height: Math.min(Theme.propertiesHeight, maxHeight)

    // ---- title ----
    Text {
        id: titleText
        width: parent.width - closeButton.width
        text: PropertiesController.title
        textFormat: Text.PlainText
        elide: Text.ElideMiddle
        font.pixelSize: Theme.dialogTitleSize
        font.weight: Font.DemiBold
        color: Theme.text
    }
    ToolbarButton {
        id: closeButton
        anchors.right: parent.right
        anchors.top: parent.top
        icon: Icons.close
        onClicked: PropertiesController.close()
    }

    // ---- tabs ----
    Flow {
        id: tabs
        anchors.top: closeButton.bottom
        anchors.topMargin: 6
        width: parent.width
        spacing: 2

        Repeater {
            model: root.tabNames
            delegate: Rectangle {
                required property int index
                required property string modelData
                readonly property bool selected: index === root.currentTab

                width: tabText.implicitWidth + 2 * Theme.propertyTabPadding
                height: Theme.propertyTabHeight
                radius: Theme.controlRadius
                color: selected ? Theme.selectionBg : tabArea.containsMouse ? Theme.hoverBg : "transparent"

                Text {
                    id: tabText
                    anchors.centerIn: parent
                    text: parent.modelData
                    font.pixelSize: Theme.smallFontSize
                    color: Theme.text
                }
                MouseArea {
                    id: tabArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.currentTab = parent.index
                }
            }
        }
    }

    // ---- rows ----
    Flickable {
        id: scroller
        anchors.top: tabs.bottom
        anchors.topMargin: 10
        anchors.bottom: footer.top
        anchors.bottomMargin: 10
        width: parent.width
        contentHeight: column.height
        boundsBehavior: Flickable.StopAtBounds
        clip: true
        ScrollBar.vertical: ThinScrollBar {}

        Column {
            id: column
            width: scroller.width - Theme.scrollbarWidth

            CheckRow {
                id: runAllowed
                visible: PropertiesController.canSetExecutable && root.currentName === "Permissions"
                height: visible ? implicitHeight + Theme.propertyRowPadding : 0
                text: "Allow running as program"
                checked: PropertiesController.executable
                onCheckedChanged: {
                    if (checked === PropertiesController.executable) return
                    PropertiesController.setExecutable(checked)
                    checked = Qt.binding(() => PropertiesController.executable)
                }
            }

            Repeater {
                model: root.currentRows
                delegate: PropertyRow {
                    required property var modelData
                    width: column.width
                    label: modelData.label
                    value: modelData.value
                    onCopyRequested: PropertiesController.copyText(modelData.value)
                }
            }
        }

        // ---- the Hashes tab before and while calculating ----
        Column {
            visible: root.hashesTab && PropertiesController.hashes.length === 0
            width: scroller.width
            spacing: 10

            Text {
                textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
                width: parent.width
                wrapMode: Text.Wrap
                text: PropertiesController.hashError !== "" ? PropertiesController.hashError
                                                            : "MD5, SHA-1, SHA-256, SHA-512 and BLAKE3. The whole file has to be read."
                font.pixelSize: Theme.smallFontSize
                color: PropertiesController.hashError !== "" ? Theme.danger : Theme.textMuted
            }
            Rectangle {
                visible: PropertiesController.hashing
                width: parent.width
                height: Theme.progressHeight
                radius: height / 2
                color: Theme.dialogFieldBg
                Rectangle {
                    width: parent.width * PropertiesController.hashProgress
                    height: parent.height
                    radius: parent.radius
                    color: Theme.accent
                }
            }
            DialogButton {
                text: PropertiesController.hashing ? "Cancel" : "Calculate"
                primary: !PropertiesController.hashing
                onClicked: PropertiesController.hashing ? PropertiesController.cancelHashes()
                                                        : PropertiesController.startHashes()
            }
        }
    }

    // ---- bottom line ----
    Text {
        textFormat: Text.PlainText        // a name from outside is shown as it is, never as markup
        anchors.left: parent.left
        anchors.verticalCenter: footer.verticalCenter
        text: PropertiesController.status
        font.pixelSize: Theme.smallFontSize
        color: Theme.textMuted
    }
    DialogButton {
        id: footer
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: "Copy All"
        onClicked: PropertiesController.copyAll()
    }
}
