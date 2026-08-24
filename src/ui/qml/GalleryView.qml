import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Rectangle {
    id: root
    color: Theme.background // Match sidebar deep dark

    // Emitted on single click (open) and double-click (selection).
    signal mediaSelected(int idx, string id, string type)
    signal mediaOpened(int idx, string id, string type)

    GridView {
        id: grid
        anchors.fill: parent
        anchors.margins: Theme.spacingMd
        focus: true

        property int minCellWidth: 160 // More compact
        property int columns: Math.max(1, Math.floor(width / minCellWidth))
        cellWidth: width / columns
        cellHeight: cellWidth

        // Pre-load roughly 2 screens of content to ensure smooth scrolling
        cacheBuffer: grid.height * 2

        model: galleryModel

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            contentItem: Rectangle {
                implicitWidth: 6
                implicitHeight: 100
                radius: 3
                color: parent.pressed ? Theme.borderFocus : (parent.hovered ? Theme.border : Theme.surfaceElevated)
            }
        }

        Keys.onPressed: function(event) {
            if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
                var sel = galleryModel.getSelectedMediaIds()
                if (sel && sel.length > 0) {
                    root.mediaOpened(0, sel[0], "")
                }
                event.accepted = true
            } else if (event.key === Qt.Key_Escape) {
                galleryModel.clearSelection()
                event.accepted = true
            }
        }

        delegate: Item {
            id: cell
            width: grid.cellWidth - Theme.spacingMd
            height: grid.cellHeight - Theme.spacingMd

            property bool hovered: false
            property bool selected: model.isSelected === true

            // A double click begins with a first click. Delay the single-click
            // action just long enough to let the double-click handler replace it.
            Timer {
                id: singleClickTimer
                interval: 250
                repeat: false
                onTriggered: root.mediaSelected(index, model.mediaId, model.mediaType)
            }

            // 0: Pending, 1: Ready, 2: Unavailable, 3: Error, 4: Cancelled
            property int tState: model.thumbnailState !== undefined ? model.thumbnailState : 0

            Rectangle {
                id: card
                anchors.fill: parent
                radius: Theme.radiusSm // Crisp, professional radius
                clip: true
                color: cell.selected ? Theme.accentHover : (cell.hovered ? Theme.surfaceElevated : Theme.surface)

                border.color: cell.selected ? Theme.accent : (cell.hovered ? Theme.borderFocus : Theme.border)
                border.width: cell.selected ? 2 : 1

                // Thumbnail
                Image {
                    id: thumb
                    anchors.fill: parent
                    source: (tState === 1 && model.thumbnailUrl !== undefined) ? model.thumbnailUrl : ""
                    fillMode: Image.PreserveAspectCrop
                    asynchronous: true
                    smooth: false // Phase 6: optimized for pixel art
                    mipmap: false // Phase 6: optimized for pixel art
                    cache: true
                    opacity: (tState === 2 || tState === 3) ? 0.2 : 1.0

                    // Loading placeholder
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        visible: tState === 0

                        BusyIndicator {
                            anchors.centerIn: parent
                            running: tState === 0
                            width: Theme.iconSizeMd
                            height: Theme.iconSizeMd
                        }
                    }

                    // Error state
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        visible: tState === 3

                        Text {
                            anchors.centerIn: parent
                            text: qsTr("ERROR")
                            color: Theme.danger
                            font.pixelSize: Theme.fontSizeSm
                            font.bold: true
                            font.letterSpacing: 1
                        }
                    }

                    // Unavailable state
                    Rectangle {
                        anchors.fill: parent
                        color: "transparent"
                        visible: tState === 2

                        Column {
                            anchors.centerIn: parent
                            spacing: Theme.spacingXs
                            Text {
                                anchors.horizontalCenter: parent.horizontalCenter
                                text: qsTr("OFFLINE")
                                color: Theme.secondaryText
                                font.pixelSize: Theme.fontSizeSm
                                font.bold: true
                                font.letterSpacing: 1
                            }
                        }
                    }
                }

                // Bottom info bar overlay
                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 28
                    gradient: Gradient {
                        GradientStop { position: 0.0; color: "transparent" }
                        GradientStop { position: 1.0; color: Theme.overlayMedium }
                    }

                    Text {
                        anchors.fill: parent
                        anchors.leftMargin: Theme.spacingSm
                        anchors.rightMargin: Theme.spacingSm
                        anchors.bottomMargin: 6
                        text: model.fileName !== undefined ? model.fileName : ""
                        color: Theme.primaryText
                        elide: Text.ElideRight
                        verticalAlignment: Text.AlignBottom
                        font.pixelSize: 11
                    }
                }

                // Selected indicator
                Rectangle {
                    visible: cell.selected
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.margins: Theme.spacingSm
                    width: Theme.iconSizeSm
                    height: Theme.iconSizeSm
                    radius: Theme.radiusSm
                    color: Theme.accent

                    Text {
                        anchors.centerIn: parent
                        text: "✓"
                        color: "white"
                        font.pixelSize: 11
                        font.bold: true
                    }
                }

                // Keyboard focus indicator
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: Theme.primaryText
                    border.width: 2
                    visible: cell.GridView.isCurrentItem && grid.activeFocus
                    radius: Theme.radiusSm
                }

                Behavior on color { ColorAnimation { duration: 100 } }
                Behavior on border.color { ColorAnimation { duration: 100 } }
            }

            Accessible.role: Accessible.ListItem
            Accessible.name: model.fileName !== undefined ? model.fileName : qsTr("Media item")
            Accessible.description: cell.selected ? qsTr("Selected") : ""

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
                        singleClickTimer.restart()
                    }
                }

                onDoubleClicked: {
                    singleClickTimer.stop()
                    root.mediaOpened(index, model.mediaId, model.mediaType)
                }
            }
        }
    }
}
