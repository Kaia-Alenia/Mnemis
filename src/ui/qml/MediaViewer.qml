import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import mnemis.ui 1.0

Rectangle {
    id: root
    color: "#050505"

    signal backRequested()
    signal fullscreenToggleRequested(bool on)

    // === Image zoom/pan state ===
    property real currentScale: 1.0
    property real minScale: 0.1
    property real maxScale: 16.0

    property bool isVideo:    viewerModel.currentType === "video"
    property bool isAnimated: viewerModel.currentType === "animated"
    property bool isImage:    viewerModel.currentType === "image" || viewerModel.currentType === ""

    // Connect fullscreen signal
    Connections {
        target: viewerModel
        function onFullscreenRequested(on) { root.fullscreenToggleRequested(on) }
    }

    // ─── Image view (Flickable + pinch) ───────────────────────────────────────
    Flickable {
        id: flickable
        anchors.fill: parent
        visible: isImage
        contentWidth:  Math.max(width,  photoImage.width  * currentScale)
        contentHeight: Math.max(height, photoImage.height * currentScale)
        clip: true
        interactive: currentScale > 1.0

        Image {
            id: photoImage
            width:  flickable.width
            height: flickable.height
            source: isImage && viewerModel.canonicalPath !== ""
                        ? ("file://" + viewerModel.canonicalPath)
                        : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: true
            cache: false
            rotation: viewerModel.rotation

            // Loading indicator
            BusyIndicator {
                anchors.centerIn: parent
                running: photoImage.status === Image.Loading
                visible: running
            }

            // Error message
            Text {
                anchors.centerIn: parent
                visible: photoImage.status === Image.Error
                text: "No se pudo cargar la imagen\n" + (viewerModel.canonicalPath ?? "")
                color: "#ff6666"
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
            }
        }

        // Scroll wheel zoom
        WheelHandler {
            onWheel: function(event) {
                var factor = event.angleDelta.y > 0 ? 1.15 : (1.0 / 1.15)
                var newScale = Math.max(root.minScale, Math.min(root.maxScale, root.currentScale * factor))
                root.currentScale = newScale
            }
        }
    }

    // ─── Video Player ──────────────────────────────────────────────────────────
    MpvVideoItem {
        id: videoView
        anchors.fill: parent
        visible: isVideo
        controller: playbackController
    }

    // ─── Animated (GIF/WebP/APNG) ─────────────────────────────────────────────
    AnimatedFrameItem {
        id: animatedView
        anchors.fill: parent
        visible: isAnimated
        controller: viewerModel.animatedController
    }

    // ─── Top Bar ───────────────────────────────────────────────────────────────
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: "#e0000000"
        z: 10

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 8
            anchors.rightMargin: 8
            spacing: 6

            // Back
            ToolButton {
                text: "← Volver"
                onClicked: {
                    if (isVideo) playbackController.stop()
                    if (isAnimated) viewerModel.animatedController.stop()
                    root.currentScale = 1.0
                    root.backRequested()
                }
            }

            // Title
            Label {
                Layout.fillWidth: true
                text: viewerModel.title !== undefined ? viewerModel.title : ""
                color: "white"
                font.bold: true
                font.pixelSize: 13
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignHCenter
            }

            // Favorite
            ToolButton {
                text: viewerModel.isFavorite ? "★" : "☆"
                font.pixelSize: 18
                onClicked: viewerModel.toggleFavorite()
                ToolTip.text: viewerModel.isFavorite ? "Quitar de favoritos" : "Añadir a favoritos"
                ToolTip.visible: hovered
            }

            // Fullscreen
            ToolButton {
                text: "⛶"
                font.pixelSize: 16
                onClicked: viewerModel.toggleFullscreen()
                ToolTip.text: "Pantalla completa"
                ToolTip.visible: hovered
            }

            // Prev / Next
            ToolButton {
                text: "‹"
                font.pixelSize: 20
                onClicked: viewerModel.previous()
                ToolTip.text: "Anterior"
                ToolTip.visible: hovered
            }
            ToolButton {
                text: "›"
                font.pixelSize: 20
                onClicked: viewerModel.next()
                ToolTip.text: "Siguiente"
                ToolTip.visible: hovered
            }
        }
    }

    // ─── Image Controls Bar (only for images) ─────────────────────────────────
    Rectangle {
        id: imgControls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 48
        color: "#d0000000"
        visible: isImage
        z: 10

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 12
            spacing: 4

            // Zoom out
            ToolButton {
                text: "−"
                font.pixelSize: 18
                onClicked: root.currentScale = Math.max(root.minScale, root.currentScale / 1.25)
            }

            // Zoom label
            Label {
                text: Math.round(root.currentScale * 100) + "%"
                color: "white"
                font.pixelSize: 12
                Layout.minimumWidth: 48
                horizontalAlignment: Text.AlignHCenter
            }

            // Zoom in
            ToolButton {
                text: "+"
                font.pixelSize: 18
                onClicked: root.currentScale = Math.min(root.maxScale, root.currentScale * 1.25)
            }

            // Fit to window
            ToolButton {
                text: "⊡"
                font.pixelSize: 16
                onClicked: root.currentScale = 1.0
                ToolTip.text: "Ajustar a ventana"
                ToolTip.visible: hovered
            }

            // Original size
            ToolButton {
                text: "1:1"
                font.pixelSize: 12
                onClicked: {
                    if (photoImage.sourceSize.width > 0) {
                        root.currentScale = photoImage.sourceSize.width / flickable.width
                    }
                }
                ToolTip.text: "Tamaño original"
                ToolTip.visible: hovered
            }

            Item { Layout.fillWidth: true }

            // Rotate CCW
            ToolButton {
                text: "↺"
                font.pixelSize: 16
                onClicked: viewerModel.rotate(-90)
                ToolTip.text: "Rotar 90° izquierda"
                ToolTip.visible: hovered
            }

            // Rotate CW
            ToolButton {
                text: "↻"
                font.pixelSize: 16
                onClicked: viewerModel.rotate(90)
                ToolTip.text: "Rotar 90° derecha"
                ToolTip.visible: hovered
            }

            // Resolution info
            Label {
                text: viewerModel.resolution.width > 0
                        ? viewerModel.resolution.width + "×" + viewerModel.resolution.height
                        : ""
                color: "#aaa"
                font.pixelSize: 11
            }
        }
    }

    // ─── Video Controls Bar ───────────────────────────────────────────────────
    Rectangle {
        id: videoControls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        color: "#d0000000"
        visible: isVideo
        z: 10

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 6
            spacing: 2

            // Seek slider
            Slider {
                Layout.fillWidth: true
                from: 0
                to: playbackController.duration > 0 ? playbackController.duration : 1
                value: playbackController.position
                onMoved: playbackController.position = value
            }

            RowLayout {
                spacing: 6

                ToolButton {
                    text: playbackController.state === "Playing" ? "⏸" : "▶"
                    font.pixelSize: 16
                    onClicked: {
                        if (playbackController.state === "Playing") playbackController.pause()
                        else playbackController.play()
                    }
                }

                ToolButton {
                    text: "⏹"
                    font.pixelSize: 16
                    onClicked: playbackController.stop()
                }

                Label {
                    text: {
                        var pos = Math.floor(playbackController.position)
                        var dur = Math.floor(playbackController.duration)
                        return String(Math.floor(pos/60)).padStart(2,"0") + ":" +
                               String(pos%60).padStart(2,"0") + " / " +
                               String(Math.floor(dur/60)).padStart(2,"0") + ":" +
                               String(dur%60).padStart(2,"0")
                    }
                    color: "white"
                    font.pixelSize: 12
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: "Vol:"
                    color: "#aaa"
                    font.pixelSize: 12
                }
                Slider {
                    from: 0; to: 100
                    value: playbackController.volume ?? 100
                    onMoved: playbackController.volume = value
                    implicitWidth: 80
                }
            }
        }
    }

    // Keyboard shortcuts
    Keys.onLeftPressed: viewerModel.previous()
    Keys.onRightPressed: viewerModel.next()
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_F) viewerModel.toggleFullscreen()
        if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal)
            root.currentScale = Math.min(root.maxScale, root.currentScale * 1.25)
        if (event.key === Qt.Key_Minus)
            root.currentScale = Math.max(root.minScale, root.currentScale / 1.25)
        if (event.key === Qt.Key_0 || event.key === Qt.Key_Home)
            root.currentScale = 1.0
        if (event.key === Qt.Key_R) viewerModel.rotate(90)
        if (event.key === Qt.Key_L) viewerModel.rotate(-90)
        if (event.key === Qt.Key_Escape) root.backRequested()
    }
    focus: true
}
