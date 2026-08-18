import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("Mnemis")

    // Handle fullscreen from ViewerViewModel
    Connections {
        target: viewerModel
        function onFullscreenRequested(on) {
            if (on) window.showFullScreen()
            else window.showNormal()
        }
    }

    // F11 global shortcut
    Shortcut {
        sequence: "F11"
        onActivated: {
            if (window.visibility === Window.FullScreen) window.showNormal()
            else window.showFullScreen()
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: {
            if (window.visibility === Window.FullScreen) window.showNormal()
        }
    }

    StackView {
        id: stack
        anchors.fill: parent
        initialItem: galleryPage
    }

    Component {
        id: galleryPage
        Item {
            RowLayout {
                anchors.fill: parent
                spacing: 0

                Sidebar {
                    Layout.fillHeight: true
                    Layout.preferredWidth: 220
                }

                GalleryView {
                    Layout.fillHeight: true
                    Layout.fillWidth: true

                    // Single click — select
                    onMediaSelected: function(idx, id, type) {
                        galleryModel.toggleSelection(id)
                    }

                    // Double click — open in viewer
                    onMediaOpened: function(idx, id, type) {
                        viewerModel.open(id)
                        stack.push(viewerPage)
                    }
                }
            }
        }
    }

    Component {
        id: viewerPage
        MediaViewer {
            onBackRequested: {
                window.showNormal()
                stack.pop()
            }
        }
    }
}
