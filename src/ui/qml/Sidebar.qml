import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#111111"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 4

        // App header
        Label {
            text: "Mnemis"
            font.pixelSize: 22
            font.bold: true
            color: "#4a9eff"
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: 8
        }

        // Search box
        TextField {
            id: searchField
            Layout.fillWidth: true
            placeholderText: "Buscar..."
            color: "white"
            background: Rectangle {
                color: "#252525"
                radius: 6
                border.color: searchField.activeFocus ? "#4a9eff" : "#333"
            }
            onTextChanged: galleryModel.setFilter(text, currentTypeFilter)
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: "#2a2a2a"; Layout.topMargin: 8; Layout.bottomMargin: 4 }

        Label { text: "FILTROS"; color: "#666"; font.pixelSize: 10; font.bold: true }

        property int currentTypeFilter: -1

        // Filter buttons
        Repeater {
            model: [
                { label: "Todos",     icon: "⬛", typeId: -1 },
                { label: "Imágenes",  icon: "🖼",  typeId: 1  },
                { label: "Videos",    icon: "▶",   typeId: 2  },
                { label: "GIFs",      icon: "GIF", typeId: 4  },
                { label: "Favoritos", icon: "★",   typeId: -2 },
            ]

            delegate: ItemDelegate {
                Layout.fillWidth: true
                highlighted: root.currentTypeFilter === modelData.typeId
                background: Rectangle {
                    color: parent.highlighted ? "#1e3a5f" : (parent.hovered ? "#1e1e1e" : "transparent")
                    radius: 6
                    border.color: parent.highlighted ? "#4a9eff" : "transparent"
                    border.width: 1
                }

                contentItem: RowLayout {
                    spacing: 10
                    Label {
                        text: modelData.icon
                        font.pixelSize: 14
                        color: parent.parent.highlighted ? "#4a9eff" : "#aaa"
                    }
                    Label {
                        text: modelData.label
                        color: parent.parent.highlighted ? "white" : "#ccc"
                        font.pixelSize: 13
                    }
                }

                onClicked: {
                    root.currentTypeFilter = modelData.typeId
                    if (modelData.typeId === -2) {
                        galleryModel.setFilter(searchField.text, -1)  // favorites: filtered separately
                    } else {
                        galleryModel.setFilter(searchField.text, modelData.typeId)
                    }
                }
            }
        }

        // Separator
        Rectangle { Layout.fillWidth: true; height: 1; color: "#2a2a2a"; Layout.topMargin: 8; Layout.bottomMargin: 4 }

        Label { text: "ORDENAR"; color: "#666"; font.pixelSize: 10; font.bold: true }

        Repeater {
            model: [
                { label: "Nombre ↑",   sortBy: "fileName",     asc: true  },
                { label: "Nombre ↓",   sortBy: "fileName",     asc: false },
                { label: "Fecha ↑",    sortBy: "modifiedTime", asc: true  },
                { label: "Fecha ↓",    sortBy: "modifiedTime", asc: false },
                { label: "Tamaño ↑",   sortBy: "fileSize",     asc: true  },
                { label: "Tamaño ↓",   sortBy: "fileSize",     asc: false },
            ]

            delegate: ItemDelegate {
                Layout.fillWidth: true
                contentItem: Label {
                    text: modelData.label
                    color: "#bbb"
                    font.pixelSize: 12
                }
                background: Rectangle {
                    color: parent.hovered ? "#1e1e1e" : "transparent"
                    radius: 6
                }
                onClicked: galleryModel.setSortOptions(modelData.sortBy, modelData.asc)
            }
        }

        Item { Layout.fillHeight: true }

        // Count label
        Label {
            text: galleryModel.count + " elementos"
            color: "#555"
            font.pixelSize: 11
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
