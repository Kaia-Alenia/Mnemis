import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0a0a0c" // Deeper dark

    property int currentTypeFilter: -1
    property bool currentFavoriteFilter: false
    property bool currentRecentFilter: false

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 6

        // App header
        Label {
            text: qsTr("MNEMIS")
            font.pixelSize: 14
            font.bold: true
            font.letterSpacing: 2
            color: "#e2e2e2"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 12
            Layout.topMargin: 4
        }

        // Search box
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: qsTr("Search...")
            Accessible.name: qsTr("Search field")
            Accessible.role: Accessible.EditableText
            color: "#e2e2e2"
            font.pixelSize: 12
            leftPadding: 12
            rightPadding: 12
            topPadding: 8
            bottomPadding: 8
            background: Rectangle {
                color: searchField.activeFocus ? "#1a1a1f" : "#141418"
                radius: 4
                border.color: searchField.activeFocus ? "#3a7bd5" : "transparent"
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
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1c1c22"; Layout.topMargin: 12; Layout.bottomMargin: 8 }

        Label { text: qsTr("VIEWS"); color: "#5a5a66"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.bottomMargin: 4; Accessible.ignored: true }

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
                Layout.preferredHeight: 32
                Accessible.name: modelData.label
                Accessible.role: Accessible.Button
                highlighted: !modelData.isAction && (modelData.isRecent ? root.currentRecentFilter :
                             (modelData.isFavorite ? root.currentFavoriteFilter :
                             (!root.currentFavoriteFilter && !root.currentRecentFilter && root.currentTypeFilter === modelData.typeId)))
                background: Rectangle {
                    color: parent.highlighted ? "#162436" : (parent.hovered ? "#141418" : "transparent")
                    radius: 4
                    border.color: parent.activeFocus ? "#3a7bd5" : "transparent"
                    border.width: parent.activeFocus ? 1 : 0
                }

                contentItem: Label {
                    text: modelData.label
                    color: parent.highlighted ? "#5ba4fc" : (parent.hovered ? "#e2e2e2" : "#9999a3")
                    font.pixelSize: 12
                    font.weight: parent.highlighted ? Font.DemiBold : Font.Normal
                    verticalAlignment: Text.AlignVCenter
                }

                onClicked: {
                    if (modelData.isAction) {
                        console.log("Action clicked:", modelData.label)
                        // TODO: Implement navigation for Folders, Playlists, Plugins, Settings
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
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1c1c22"; Layout.topMargin: 12; Layout.bottomMargin: 8 }

        Label { text: qsTr("SORT BY"); color: "#5a5a66"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.bottomMargin: 4; Accessible.ignored: true }

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
                    color: parent.hovered ? "#e2e2e2" : "#888894"
                    font.pixelSize: 11
                    verticalAlignment: Text.AlignVCenter
                }
                background: Rectangle {
                    color: parent.hovered ? "#141418" : "transparent"
                    radius: 4
                    border.color: parent.activeFocus ? "#3a7bd5" : "transparent"
                    border.width: parent.activeFocus ? 1 : 0
                }
                onClicked: {
                    galleryModel.setSortOptions(modelData.sortBy, modelData.asc)
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1c1c22"; Layout.topMargin: 12; Layout.bottomMargin: 8 }

        RowLayout {
            Layout.fillWidth: true
            Label { text: qsTr("LIBRARY ROOTS"); color: "#5a5a66"; font.pixelSize: 10; font.bold: true; font.letterSpacing: 1; Layout.fillWidth: true; Accessible.ignored: true }
            Button {
                id: addRootBtn
                text: "+"
                Accessible.name: qsTr("Add Library Root")
                Accessible.role: Accessible.Button
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                padding: 0
                background: Rectangle { color: addRootBtn.hovered ? "#1c1c22" : "transparent"; radius: 4; border.color: addRootBtn.activeFocus ? "#3a7bd5" : "transparent"; border.width: addRootBtn.activeFocus ? 1 : 0 }
                contentItem: Label { text: addRootBtn.text; color: addRootBtn.hovered ? "#fff" : "#888894"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 16 }
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
            spacing: 4

            Label {
                text: qsTr("No directories added.")
                color: "#777782"
                font.pixelSize: 11
                font.italic: true
                Layout.bottomMargin: 4
            }

            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Pictures Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? "#1c1c22" : "#141418"; radius: 4; border.color: parent.activeFocus ? "#3a7bd5" : "#22222a" }
                contentItem: Label { text: qsTr("Add ~/Pictures"); color: "#aaddff"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.picturesLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Videos Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? "#1c1c22" : "#141418"; radius: 4; border.color: parent.activeFocus ? "#3a7bd5" : "#22222a" }
                contentItem: Label { text: qsTr("Add ~/Videos"); color: "#aaddff"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.moviesLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Music Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? "#1c1c22" : "#141418"; radius: 4; border.color: parent.activeFocus ? "#3a7bd5" : "#22222a" }
                contentItem: Label { text: qsTr("Add ~/Music"); color: "#aaddff"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.musicLocation())
                }
            }
            Button {
                Layout.fillWidth: true
                Layout.preferredHeight: 28
                Accessible.name: qsTr("Add Downloads Directory")
                Accessible.role: Accessible.Button
                background: Rectangle { color: parent.hovered ? "#1c1c22" : "#141418"; radius: 4; border.color: parent.activeFocus ? "#3a7bd5" : "#22222a" }
                contentItem: Label { text: qsTr("Add ~/Downloads"); color: "#aaddff"; font.pixelSize: 11; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter }
                onClicked: {
                    if (settingsModel && typeof systemPaths !== "undefined") settingsModel.addRoot("file://" + systemPaths.downloadLocation())
                }
            }
        }

        Repeater {
            model: settingsModel ? settingsModel.libraryRoots : []
            delegate: RowLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 24
                Label {
                    text: modelData.toString().substring(modelData.toString().lastIndexOf("/") + 1)
                    color: "#a0a0ab"
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
                    background: Rectangle { color: removeRootBtn.hovered ? "#4a1c1c" : "transparent"; radius: 4; border.color: removeRootBtn.activeFocus ? "#3a7bd5" : "transparent"; border.width: removeRootBtn.activeFocus ? 1 : 0 }
                    contentItem: Label { text: removeRootBtn.text; color: removeRootBtn.hovered ? "#ff6b6b" : "#666"; horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter; font.pixelSize: 14 }
                    onClicked: {
                        settingsModel.removeRoot(modelData)
                    }
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: "#1c1c22"; Layout.topMargin: 12; Layout.bottomMargin: 8 }

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Show Hidden Files")
                color: "#888894"
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
                    color: parent.checked ? "#3a7bd5" : "#22222a"
                    border.color: parent.checked ? "#3a7bd5" : "#33333a"

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
            color: "#44444d"
            font.pixelSize: 10
            font.letterSpacing: 1
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
