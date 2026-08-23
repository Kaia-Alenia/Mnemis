import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: window
    width: 1280
    height: 800
    visible: true
    title: qsTr("Mnemis")

    onVisibilityChanged: {
        console.log("[WINDOW_STATE] visibility changed to: " + window.visibility)
    }

    // Handle fullscreen from ViewerViewModel
    Connections {
        target: viewerModel
        function onFullscreenRequested(on) {
            console.log("[WINDOW_STATE] viewerModel.onFullscreenRequested(" + on + ") called. Current visibility: " + window.visibility);
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

                    // Single click — open in viewer
                    onMediaSelected: function(idx, id, type) {
                        console.log("[NAVIGATION] open viewer id=" + id + " index=" + idx)
                        viewerModel.open(id, idx)
                        stack.push(viewerPage)
                    }

                    // Double click — select exclusively
                    onMediaOpened: function(idx, id, type) {
                        console.log("[NAVIGATION] gallery selection id=" + id + " index=" + idx)
                        galleryModel.selectOne(id)
                    }
                }
            }
        }
    }

    Component {
        id: viewerPage
        MediaViewer {
            onBackRequested: {
                console.log("[WINDOW_STATE] onBackRequested fired. Current visibility: " + window.visibility);
                if (window.visibility === Window.FullScreen) {
                    console.log("[WINDOW_STATE] Window is FullScreen, restoring to previous state.");
                    window.showNormal();
                } else {
                    console.log("[WINDOW_STATE] Window is NOT FullScreen (visibility=" + window.visibility + "), doing nothing to avoid minimizing/un-maximizing.");
                }
                stack.pop();
            }
        }
    }
}
