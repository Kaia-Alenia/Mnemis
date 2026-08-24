import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: Theme.background

    property int currentTypeFilter: -1
    property bool currentFavoriteFilter: false
    property bool currentRecentFilter: false

    signal navigationRequested(string page)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        spacing: Theme.spacingSm

        // App header
        Label {
            text: qsTr("MNEMIS")
            font.pixelSize: Theme.fontSizeMd
            font.bold: true
            font.letterSpacing: 2
            color: Theme.primaryText
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Theme.spacingMd
            Layout.topMargin: Theme.spacingXs
        }

        // Search box
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search...")
            Accessible.name: qsTr("Search field")
            Accessible.role: Accessible.EditableText
            color: Theme.primaryText
            font.pixelSize: Theme.fontSizeSm
            leftPadding: Theme.spacingMd
            rightPadding: Theme.spacingMd
            topPadding: Theme.spacingSm
            bottomPadding: Theme.spacingSm
            background: Rectangle {
                color: searchField.activeFocus ? Theme.surfaceElevated : Theme.surface
                radius: Theme.radiusSm
                border.color: searchField.activeFocus ? Theme.accent : "transparent"
                border.width: 1
            }
            onTextChanged: {
                if (root.currentRecentFilter) {
                    galleryModel.setRecentFilter(true, text)
                } else if (root.currentFavoriteFilter) {
                    galleryModel.setFavoriteFilter(true, text)
                } else {
                    galleryModel.setFilter(text, currentTypeFilter)
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: Theme.spacingMd; Layout.bottomMargin: Theme.spacingSm }

        Label { text: qsTr("VIEWS"); color: Theme.secondaryText; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.bottomMargin: Theme.spacingXs; Accessible.ignored: true }

        // Filter buttons
        Repeater {
            model: [
                { label: qsTr("Home"),      typeId: -1,  isFavorite: false, isRecent: false, isAction: false },
                { label: qsTr("Photos"),    typeId: 1,   isFavorite: false, isRecent: false, isAction: false },
                { label: qsTr("Videos"),    typeId: 2,   isFavorite: false, isRecent: false, isAction: false },
                { label: qsTr("Music"),     typeId: 3,   isFavorite: false, isRecent: false, isAction: false },
                { label: qsTr("Folders"),   typeId: -2,  isFavorite: false, isRecent: false, isAction: true },
                { label: qsTr("Favorites"), typeId: -1,  isFavorite: true,  isRecent: false, isAction: false },
                { label: qsTr("Recent"),    typeId: -1,  isFavorite: false, isRecent: true,  isAction: false },
                { label: qsTr("Playlists"), typeId: -3,  isFavorite: false, isRecent: false, isAction: true },
                { label: qsTr("Plugins"),   typeId: -4,  isFavorite: false, isRecent: false, isAction: true },
                { label: qsTr("Settings"),  typeId: -5,  isFavorite: false, isRecent: false, isAction: true },
            ]

            delegate: ItemDelegate {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.controlHeight
                Accessible.name: modelData.label
                Accessible.role: Accessible.Button
                highlighted: !modelData.isAction && (modelData.isRecent ? root.currentRecentFilter :
                             (modelData.isFavorite ? root.currentFavoriteFilter :
                             (!root.currentFavoriteFilter && !root.currentRecentFilter && root.currentTypeFilter === modelData.typeId)))
                background: Rectangle {
                    color: parent.highlighted ? Theme.surfaceElevated : (parent.hovered ? Theme.surface : "transparent")
                    radius: Theme.radiusSm
                    border.color: parent.activeFocus ? Theme.accent : "transparent"
                    border.width: parent.activeFocus ? 1 : 0
                }

                contentItem: Label {
                    text: modelData.label
                    color: parent.highlighted ? Theme.accent : (parent.hovered ? Theme.primaryText : Theme.secondaryText)
                    font.pixelSize: Theme.fontSizeSm
                    font.weight: parent.highlighted ? Font.DemiBold : Font.Normal
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (modelData.isAction) {
                        console.log("Action clicked:", modelData.label)
                        root.navigationRequested(modelData.label)
                        return;
                    }
                    if (modelData.isRecent) {
                        root.currentRecentFilter = true
                        root.currentFavoriteFilter = false
                        root.currentTypeFilter = -1
                        galleryModel.setRecentFilter(true, searchField.text)
                    } else if (modelData.isFavorite) {
                        root.currentFavoriteFilter = true
                        root.currentRecentFilter = false
                        root.currentTypeFilter = -1
                        galleryModel.setFavoriteFilter(true, searchField.text)
                    } else {
                        root.currentFavoriteFilter = false
                        root.currentRecentFilter = false
                        root.currentTypeFilter = modelData.typeId
                        galleryModel.setFilter(searchField.text, modelData.typeId)
                    }
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: Theme.spacingMd; Layout.bottomMargin: Theme.spacingSm }

        Label { text: qsTr("SORT BY"); color: Theme.secondaryText; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.bottomMargin: Theme.spacingXs; Accessible.ignored: true }

        Repeater {
            model: [
                { label: qsTr("Name (A-Z)"),   sortBy: "fileName",     asc: true  },
                { label: qsTr("Name (Z-A)"),   sortBy: "fileName",     asc: false },
                { label: qsTr("Newest First"), sortBy: "modifiedTime", asc: false },
                { label: qsTr("Oldest First"), sortBy: "modifiedTime", asc: true  },
                { label: qsTr("Size (Large)"), sortBy: "fileSize",     asc: false },
                { label: qsTr("Size (Small)"), sortBy: "fileSize",     asc: true  },
            ]

            delegate: ItemDelegate {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Sort by ") + modelData.label
                Accessible.role: Accessible.Button
                contentItem: Label {
                    text: modelData.label
                    color: parent.hovered ? Theme.primaryText : Theme.secondaryText
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? Theme.surface : "transparent"
                    radius: Theme.radiusSm
                    border.color: parent.activeFocus ? Theme.accent : "transparent"
                    border.width: parent.activeFocus ? 1 : 0
                }
                onClicked: {
                    galleryModel.setSortOptions(modelData.sortBy, modelData.asc)
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: Theme.spacingMd; Layout.bottomMargin: Theme.spacingSm }

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("LIBRARY ROOTS"); color: Theme.secondaryText; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.fillWidth: true; Accessible.ignored: true }
            Button {
                id: addRootBtn
                text: "+"
                Accessible.name: qsTr("Add Library Root")
                Accessible.role: Accessible.Button
                Layout.preferredWidth: Theme.iconSizeMd
                Layout.preferredHeight: Theme.iconSizeMd
                padding: 0
                background: Rectangle { color: addRootBtn.hovered ? Theme.surfaceElevated : "transparent"; radius: Theme.radiusSm; border.color: addRootBtn.activeFocus ? Theme.accent : "transparent"; border.width: addRootBtn.activeFocus ? 1 : 0 }
                contentItem: Label { text: addRootBtn.text; color: addRootBtn.hovered ? Theme.primaryText : Theme.secondaryText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 16 }
                onClicked: {
                    if (typeof systemPaths !== "undefined") {
                        var folder = systemPaths.openFolderDialog()
                        if (folder !== "" && typeof settingsModel !== "undefined") {
                            settingsModel.addRoot(folder)
                        }
                    } else {
                        console.error("systemPaths is not available.")
                    }
                }
            }
        }

        // Quick Add buttons for empty state
        ColumnLayout {
            Layout.fillWidth: true
            visible: settingsModel && settingsModel.libraryRoots.length === 0
            spacing: Theme.spacingXs

            Label {
                text: qsTr("No directories added.")
                color: Theme.secondaryText
                font.pixelSize: 11
                font.italic: true
                Layout.bottomMargin: Theme.spacingXs
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Pictures Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? Theme.surfaceElevated : Theme.surface; radius: Theme.radiusSm; border.color: parent.activeFocus ? Theme.accent : Theme.border }
                contentItem: Label { text: qsTr("Add ~/Pictures"); color: Theme.accent; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.picturesLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Videos Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? Theme.surfaceElevated : Theme.surface; radius: Theme.radiusSm; border.color: parent.activeFocus ? Theme.accent : Theme.border }
                contentItem: Label { text: qsTr("Add ~/Videos"); color: Theme.accent; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.moviesLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Music Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? Theme.surfaceElevated : Theme.surface; radius: Theme.radiusSm; border.color: parent.activeFocus ? Theme.accent : Theme.border }
                contentItem: Label { text: qsTr("Add ~/Music"); color: Theme.accent; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.musicLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Downloads Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? Theme.surfaceElevated : Theme.surface; radius: Theme.radiusSm; border.color: parent.activeFocus ? Theme.accent : Theme.border }
                contentItem: Label { text: qsTr("Add ~/Downloads"); color: Theme.accent; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.downloadLocation())
                }
            }
        }

        Repeater {
            model: settingsModel ? settingsModel.libraryRoots : []
            delegate: RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.iconSizeMd
                Label {
                    text: modelData.toString().substring(modelData.toString().lastIndexOf("/") + 1)
                    color: Theme.secondaryText
                    font.pixelSize: 11
                    Layout.fillWidth: true
                    elide: Text.ElideRight
                    ToolTip.text: modelData
                    ToolTip.visible: hoverArea.containsMouse
                    ToolTip.delay: 500

                    MouseArea {
                        id: hoverArea
                        anchors.fill: parent
                        hoverEnabled: true
                    }
                }
                Button {
                    id: removeRootBtn
                    text: "×"
                    Accessible.name: qsTr("Remove ") + modelData
                    Accessible.role: Accessible.Button
                    Layout.preferredWidth: 20
                    Layout.preferredHeight: 20
                    padding: 0
                    background: Rectangle { color: removeRootBtn.hovered ? Theme.danger : "transparent"; radius: Theme.radiusSm; border.color: removeRootBtn.activeFocus ? Theme.accent : "transparent"; border.width: removeRootBtn.activeFocus ? 1 : 0 }
                    contentItem: Label { text: removeRootBtn.text; color: removeRootBtn.hovered ? Theme.primaryText : Theme.secondaryText; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 14 }
                    onClicked: {
                        settingsModel.removeRoot(modelData)
                    }
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: Theme.border; Layout.topMargin: Theme.spacingMd; Layout.bottomMargin: Theme.spacingSm }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Show Hidden Files")
                color: Theme.secondaryText
                font.pixelSize: 11
                Layout.fillWidth: true
            }
            Switch {
                checked: settingsModel ? settingsModel.includeHidden : false
                // Custom, smaller switch design
                indicator: Rectangle {
                    implicitWidth: 32
                    implicitHeight: 18
                    x: parent.leftPadding
                    y: parent.height / 2 - height / 2
                    radius: 9
                    color: parent.checked ? Theme.accent : Theme.surface
                    border.color: parent.checked ? Theme.accent : Theme.border

                    Rectangle {
                        x: parent.checked ? parent.width - width - 2 : 2
                        y: 2
                        width: 14
                        height: 14
                        radius: 7
                        color: "white"
                        Behavior on x { NumberAnimation { duration: 150 } }
                    }
                }
                onToggled: {
                    if (settingsModel) {
                        settingsModel.setIncludeHidden(checked)
                    }
                }
                Accessible.name: qsTr("Show Hidden Files")
                Accessible.role: Accessible.CheckBox
            }
        }

        Item { Layout.fillHeight: true }

        // Count label
        Label {
            text: galleryModel.count + qsTr(" items")
            Accessible.name: galleryModel.count + qsTr(" items total")
            color: Theme.secondaryText
            font.pixelSize: 10
            font.letterSpacing: 1
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
