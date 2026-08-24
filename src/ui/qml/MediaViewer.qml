import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import mnemis.ui 1.0

Rectangle {
    id: root
    color: Theme.background

    signal backRequested()
    signal fullscreenToggleRequested(bool on)

    // === UI auto-hide logic ===
    property bool controlsVisible: true
    
    Timer {
        id: hideControlsTimer
        interval: 3000
        running: true
        repeat: false
        onTriggered: {
            if (isVideo || isImage || isAnimated) {
                controlsVisible = false;
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
        onPositionChanged: {
            controlsVisible = true;
            hideControlsTimer.restart();
        }
    }

    // === Image zoom/pan state ===
    property real currentScale: 1.0
    property real minScale: 0.1
    property real maxScale: 16.0

    property bool isVideo:    viewerModel.currentType === "video"
    property bool isAudio:    viewerModel.currentType === "audio"
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
        contentWidth:  Math.max(width, photoImage.width)
        contentHeight: Math.max(height, photoImage.height)
        clip: true
        interactive: currentScale > 1.0

        Image {
            id: photoImage
            width:  flickable.width * root.currentScale
            height: flickable.height * root.currentScale
            source: isImage && viewerModel.canonicalPath !== ""
                        ? ("file://" + viewerModel.canonicalPath)
                        : ""
            fillMode: Image.PreserveAspectFit
            asynchronous: true
            smooth: false
            mipmap: false
            antialiasing: false
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
                text: qsTr("Failed to load image\n") + (viewerModel.canonicalPath ?? "")
                color: Theme.danger
                font.pixelSize: 14
                horizontalAlignment: Text.AlignHCenter
                Accessible.name: text
                Accessible.role: Accessible.StaticText
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
        smooth: false
        antialiasing: false
        controller: viewerModel.animatedController
    }

    // ─── Audio Placeholder ─────────────────────────────────────────────────────
    Rectangle {
        anchors.fill: parent
        color: Theme.background
        visible: isAudio

        Label {
            anchors.centerIn: parent
            text: "🎵\n\n" + (viewerModel.title !== undefined ? viewerModel.title : qsTr("Audio"))
            color: Theme.primaryText
            font.pixelSize: 48
            horizontalAlignment: Text.AlignHCenter
            Accessible.name: qsTr("Audio Placeholder")
            Accessible.role: Accessible.StaticText
        }
    }

    // ─── Top Bar ───────────────────────────────────────────────────────────────
    Rectangle {
        id: topBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 52
        color: Theme.overlayDark
        z: 10
        opacity: root.controlsVisible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 250 } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingSm
            anchors.rightMargin: Theme.spacingSm
            spacing: Theme.spacingXs

            // Back
            ToolButton {
                text: "← " + qsTr("Back")
                Accessible.name: qsTr("Back")
                Accessible.role: Accessible.Button
                Accessible.description: qsTr("Go back to previous view")
                onClicked: {
                    console.log("[VIEWER] back clicked — type=" + viewerModel.currentType
                        + " isVideo=" + isVideo + " isAudio=" + isAudio + " isAnimated=" + isAnimated)
                    if (isVideo || isAudio) {
                        console.log("[VIEWER] stopping playback controller")
                        playbackController.stop()
                    }
                    if (isAnimated) {
                        console.log("[VIEWER] stopping animated controller")
                        viewerModel.animatedController.stop()
                    }
                    root.currentScale = 1.0
                    console.log("[VIEWER] emitting backRequested")
                    root.backRequested()
                }
            }

            // Title
            Label {
                Layout.fillWidth: true
                text: viewerModel.title !== undefined ? viewerModel.title : ""
                color: Theme.primaryText
                font.bold: true
                font.pixelSize: 13
                elide: Text.ElideMiddle
                horizontalAlignment: Text.AlignHCenter
            }

            // Favorite
            ToolButton {
                text: viewerModel.isFavorite ? "★" : "☆"
                font.pixelSize: 18
                Accessible.name: viewerModel.isFavorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                Accessible.role: Accessible.Button
                onClicked: {
                    console.log("[VIEWER] favorite clicked")
                    viewerModel.toggleFavorite()
                }
                ToolTip.text: viewerModel.isFavorite ? qsTr("Remove from favorites") : qsTr("Add to favorites")
                ToolTip.visible: hovered
            }

            // Fullscreen
            ToolButton {
                text: "⛶"
                font.pixelSize: 16
                Accessible.name: qsTr("Toggle Fullscreen")
                Accessible.role: Accessible.Button
                onClicked: {
                    console.log("[VIEWER] fullscreen clicked")
                    viewerModel.toggleFullscreen()
                }
                ToolTip.text: qsTr("Toggle Fullscreen")
                ToolTip.visible: hovered
            }

            // Prev / Next
            ToolButton {
                text: "‹"
                font.pixelSize: 20
                Accessible.name: qsTr("Previous")
                Accessible.role: Accessible.Button
                onClicked: {
                    console.log("[VIEWER] previous clicked")
                    viewerModel.previous()
                }
                ToolTip.text: qsTr("Previous")
                ToolTip.visible: hovered
            }
            ToolButton {
                text: "›"
                font.pixelSize: 20
                Accessible.name: qsTr("Next")
                Accessible.role: Accessible.Button
                onClicked: {
                    console.log("[VIEWER] next clicked")
                    viewerModel.next()
                }
                ToolTip.text: qsTr("Next")
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
        color: Theme.overlay
        visible: isImage
        z: 10
        opacity: root.controlsVisible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 250 } }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: Theme.spacingMd
            anchors.rightMargin: Theme.spacingMd
            spacing: Theme.spacingXs

            // Zoom out
            ToolButton {
                text: "−"
                font.pixelSize: Theme.fontSizeLg
                Accessible.name: qsTr("Zoom Out")
                Accessible.role: Accessible.Button
                onClicked: root.currentScale = Math.max(root.minScale, root.currentScale / 1.25)
            }

            // Zoom label
            Label {
                text: Math.round(root.currentScale * 100) + "%"
                color: Theme.primaryText
                font.pixelSize: Theme.fontSizeSm
                Layout.minimumWidth: 48
                horizontalAlignment: Text.AlignHCenter
            }

            // Zoom in
            ToolButton {
                text: "+"
                font.pixelSize: 18
                Accessible.name: qsTr("Zoom In")
                Accessible.role: Accessible.Button
                onClicked: root.currentScale = Math.min(root.maxScale, root.currentScale * 1.25)
            }

            // Fit to window
            ToolButton {
                text: "⊡"
                font.pixelSize: 16
                Accessible.name: qsTr("Fit to window")
                Accessible.role: Accessible.Button
                onClicked: root.currentScale = 1.0
                ToolTip.text: qsTr("Fit to window")
                ToolTip.visible: hovered
            }

            // Original size
            ToolButton {
                text: "1:1"
                font.pixelSize: 12
                Accessible.name: qsTr("Original size")
                Accessible.role: Accessible.Button
                onClicked: {
                    if (photoImage.sourceSize.width > 0) {
                        root.currentScale = photoImage.sourceSize.width / flickable.width
                    }
                }
                ToolTip.text: qsTr("Original size")
                ToolTip.visible: hovered
            }

            Item { Layout.fillWidth: true }

            // Rotate CCW
            ToolButton {
                text: "↺"
                font.pixelSize: 16
                Accessible.name: qsTr("Rotate counter-clockwise")
                Accessible.role: Accessible.Button
                onClicked: viewerModel.rotate(-90)
                ToolTip.text: qsTr("Rotate counter-clockwise")
                ToolTip.visible: hovered
            }

            // Rotate CW
            ToolButton {
                text: "↻"
                font.pixelSize: 16
                Accessible.name: qsTr("Rotate clockwise")
                Accessible.role: Accessible.Button
                onClicked: viewerModel.rotate(90)
                ToolTip.text: qsTr("Rotate clockwise")
                ToolTip.visible: hovered
            }

            // Resolution info
            Label {
                text: viewerModel.resolution.width > 0
                        ? viewerModel.resolution.width + "×" + viewerModel.resolution.height
                        : ""
                color: Theme.secondaryText
                font.pixelSize: 11
            }
        }
    }

    // ─── Video/Audio Controls Bar ─────────────────────────────────────────────
    Rectangle {
        id: mediaControls
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 56
        color: Theme.overlay
        visible: isVideo || isAudio
        z: 10
        opacity: root.controlsVisible ? 1.0 : 0.0
        Behavior on opacity { NumberAnimation { duration: 250 } }

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
                spacing: Theme.spacingSm

                ToolButton {
                    text: playbackController.state === "Playing" ? "⏸" : "▶"
                    font.pixelSize: 16
                    Accessible.name: playbackController.state === "Playing" ? qsTr("Pause") : qsTr("Play")
                    Accessible.role: Accessible.Button
                    onClicked: {
                        if (playbackController.state === "Playing") playbackController.pause()
                        else playbackController.play()
                    }
                }

                ToolButton {
                    text: "⏹"
                    font.pixelSize: 16
                    Accessible.name: qsTr("Stop")
                    Accessible.role: Accessible.Button
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
                    color: Theme.primaryText
                    font.pixelSize: Theme.fontSizeSm
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: qsTr("Vol:")
                    color: Theme.secondaryText
                    font.pixelSize: Theme.fontSizeSm
                }
                Slider {
                    from: 0; to: 100
                    value: playbackController.volume ?? 100
                    onMoved: playbackController.volume = value
                    implicitWidth: 80
                    Accessible.name: qsTr("Volume")
                    Accessible.role: Accessible.Slider
                }

                Label {
                    text: qsTr("Speed: ") + playbackController.playbackRate.toFixed(1) + "x"
                    color: Theme.secondaryText
                    font.pixelSize: Theme.fontSizeSm
                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (playbackController.playbackRate < 2.0)
                                playbackController.playbackRate += 0.25;
                            else
                                playbackController.playbackRate = 0.25;
                        }
                    }
                    Accessible.name: qsTr("Playback Speed")
                    Accessible.role: Accessible.StaticText
                }

                Label {
                    text: qsTr("Audio:")
                    color: Theme.secondaryText
                    font.pixelSize: 12
                    visible: playbackController.audioTracks && playbackController.audioTracks.length > 0
                }
                ComboBox {
                    id: audioTrackCombo
                    model: playbackController.audioTracks
                    textRole: "title"
                    valueRole: "id"
                    implicitWidth: 100
                    visible: playbackController.audioTracks && playbackController.audioTracks.length > 0
                    onActivated: (index) => {
                        var trackId = model[index].id;
                        playbackController.setAudioTrack(trackId);
                    }
                    Connections {
                        target: playbackController
                        function onTracksChanged() {
                            if (!audioTrackCombo.model) return;
                            for (var i = 0; i < audioTrackCombo.model.length; ++i) {
                                if (audioTrackCombo.model[i].selected) {
                                    audioTrackCombo.currentIndex = i;
                                    break;
                                }
                            }
                        }
                    }
                }

                Label {
                    text: qsTr("Subs:")
                    color: Theme.secondaryText
                    font.pixelSize: 12
                    visible: playbackController.subtitleTracks && playbackController.subtitleTracks.length > 0
                }
                ComboBox {
                    id: subTrackCombo
                    model: playbackController.subtitleTracks
                    textRole: "title"
                    valueRole: "id"
                    implicitWidth: 100
                    visible: playbackController.subtitleTracks && playbackController.subtitleTracks.length > 0
                    onActivated: (index) => {
                        var trackId = model[index].id;
                        playbackController.setSubtitleTrack(trackId);
                    }
                    Connections {
                        target: playbackController
                        function onTracksChanged() {
                            if (!subTrackCombo.model) return;
                            for (var i = 0; i < subTrackCombo.model.length; ++i) {
                                if (subTrackCombo.model[i].selected) {
                                    subTrackCombo.currentIndex = i;
                                    break;
                                }
                            }
                        }
                    }
                }

            }
        }
    }

    // Keyboard shortcuts
    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Left) {
            console.log("[VIEWER] previous via left arrow")
            viewerModel.previous()
            event.accepted = true
        } else if (event.key === Qt.Key_Right) {
            console.log("[VIEWER] next via right arrow")
            viewerModel.next()
            event.accepted = true
        } else if (event.key === Qt.Key_F) viewerModel.toggleFullscreen()
        else if (event.key === Qt.Key_Plus || event.key === Qt.Key_Equal)
            root.currentScale = Math.min(root.maxScale, root.currentScale * 1.25)
        else if (event.key === Qt.Key_Minus)
            root.currentScale = Math.max(root.minScale, root.currentScale / 1.25)
        else if (event.key === Qt.Key_0 || event.key === Qt.Key_Home)
            root.currentScale = 1.0
        else if (event.key === Qt.Key_R) viewerModel.rotate(90)
        else if (event.key === Qt.Key_L) viewerModel.rotate(-90)
        else if (event.key === Qt.Key_Escape) root.backRequested()
        else if (event.key === Qt.Key_Space) {
            if (isVideo || isAudio) {
                if (playbackController.state === "Playing") playbackController.pause()
                else playbackController.play()
                event.accepted = true
            }
        }
    }
    focus: true
    Component.onCompleted: forceActiveFocus()
}
