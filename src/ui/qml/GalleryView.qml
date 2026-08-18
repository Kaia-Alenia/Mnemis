import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: "#0d0d0d"

    // Emitted on single click (selection) and double-click (open)
    signal mediaSelected(int idx, string id, string type)
    signal mediaOpened(int idx, string id, string type)

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: 12
        cellWidth: 210
        cellHeight: 210
        cacheBuffer: 800

        model: galleryModel

        ScrollBar.vertical: ScrollBar {}

        delegate: Item {
            id: cell
            width: grid.cellWidth - 12
            height: grid.cellHeight - 12

            property bool hovered: false
            property bool selected: model.isSelected === true

            Rectangle {
                id: card
                anchors.fill: parent
                radius: 10
                clip: true
                color: cell.selected ? "#1e3a5f" : (cell.hovered ? "#252525" : "#181818")

                border.color: cell.selected ? "#4a9eff" : (cell.hovered ? "#3a3a3a" : "transparent")
                border.width: cell.selected ? 2 : 1

                // Thumbnail — uses model.thumbnailUrl EXCLUSIVELY
                Image {
                    id: thumb
                    anchors.fill: parent
                    source: model.thumbnailUrl !== undefined ? model.thumbnailUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    smooth: true
                    cache: true

                    // Loading placeholder
                    Rectangle {
                        anchors.fill: parent
                        color: "#1e1e1e"
                        visible: thumb.status === Image.Loading || thumb.status === Image.Null

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: thumb.status === Image.Loading
                            width: 28
                            height: 28
                        }
                    }

                    // Error state
                    Rectangle {
                        anchors.fill: parent
                        color: "#1a1a1a"
                        visible: thumb.status === Image.Error

                        Column {
                            anchors.centerIn: parent
                            spacing: 6
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "✗"
                                color: "#ff4444"
                                font.pixelSize: 24
                            }
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: "Sin vista previa"
                                color: "#666"
                                font.pixelSize: 11
                            }
                        }
                    }
                }

                // Bottom info bar
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 36
                    color: "#cc000000"

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        text: model.fileName !== undefined ? model.fileName : ""
                        color: "white"
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 12
                    }
                }

                // Selected checkmark
                Rectangle {
                    visible: cell.selected
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.margins: 6
                    width: 22
                    height: 22
                    radius: 11
                    color: "#4a9eff"

                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "white"
                        font.pixelSize: 13
                        font.bold: true
                    }
                }

                Behavior on color { ColorAnimation { duration: 80 } }
                Behavior on border.color { ColorAnimation { duration: 80 } }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                acceptedButtons: Qt.LeftButton | Qt.RightButton

                onEntered: cell.hovered = true
                onExited:  cell.hovered = false

                onClicked: function(mouse) {
                    if (mouse.modifiers & Qt.ControlModifier) {
                        galleryModel.toggleSelection(model.mediaId)
                    } else {
                        root.mediaSelected(index, model.mediaId, model.mediaType)
                    }
                }

                onDoubleClicked: {
                    root.mediaOpened(index, model.mediaId, model.mediaType)
                }
            }
        }
    }
}
