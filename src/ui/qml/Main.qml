import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root

    visible: true

    width: 1280
    height: 800

    minimumWidth: 900
    minimumHeight: 600

    title: "Mnemis"

    color: "#111111"

    property bool viewerOpen: false
    property bool spritePlaying: false
    property real panX: 0
    property real panY: 0

    Rectangle {
        anchors.fill: parent

        color: "#111111"

        ColumnLayout {
            anchors.fill: parent

            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 64

                color: "#181818"

                RowLayout {
                    anchors.fill: parent

                    anchors.leftMargin: 20
                    anchors.rightMargin: 20

                    spacing: 16

                    Label {
                        text: "Mnemis"

                        color: "white"

                        font.pixelSize: 22
                        font.bold: true
                    }

                    Rectangle {
                        Layout.fillWidth: true

                        Layout.preferredHeight: 38

                        radius: 8

                        color: "#242424"

                        TextField {
                            anchors.fill: parent

                            anchors.leftMargin: 12
                            anchors.rightMargin: 12

                            placeholderText:
                                "Search your library..."

                            color: "white"

                            placeholderTextColor:
                                "#777777"

                            background: null

                            enabled: false
                        }
                    }

                    Button {
                        text: "Refresh"

                        onClicked:
                            libraryController.refreshBrowser()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true

                spacing: 0

                Rectangle {
                    Layout.preferredWidth: 280
                    Layout.fillHeight: true

                    color: "#151515"

                    ColumnLayout {
                        anchors.fill: parent

                        anchors.margins: 14

                        spacing: 10

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text: "LIBRARY"

                                color: "#999999"

                                font.pixelSize: 12
                                font.bold: true

                                Layout.fillWidth: true
                            }

                            Button {
                                text: "+"

                                width: 34
                                height: 30

                                onClicked:
                                    addRootDialog.open()
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1

                            color: "#292929"
                        }

                        ListView {
                            id: rootList

                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            clip: true
                            spacing: 4

                            model:
                                libraryController.roots

                            delegate: Rectangle {
                                required property var modelData

                                width:
                                    rootList.width

                                height: 54

                                radius: 8

                                color:
                                    libraryController.selectedRootId ===
                                    modelData.id
                                    ? "#2a2a2a"
                                    : "transparent"

                                MouseArea {
                                    anchors.fill: parent

                                    onClicked:
                                        libraryController.selectRoot(
                                            modelData.id
                                        )
                                }

                                ColumnLayout {
                                    anchors.fill: parent

                                    anchors.leftMargin: 12
                                    anchors.rightMargin: 12

                                    spacing: 2

                                    Label {
                                        Layout.fillWidth: true

                                        text:
                                            modelData.name

                                        color: "white"

                                        font.pixelSize: 14

                                        elide:
                                            Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true

                                        text:
                                            modelData.path

                                        color: "#777777"

                                        font.pixelSize: 11

                                        elide:
                                            Text.ElideMiddle
                                    }
                                }
                            }

                            Label {
                                anchors.centerIn: parent

                                visible:
                                    libraryController.roots.length === 0

                                text:
                                    "No library roots yet"

                                color: "#666666"
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Button {
                                Layout.fillWidth: true

                                text: "Index"

                                enabled:
                                    libraryController.selectedRootId !== 0

                                onClicked:
                                    libraryController.indexSelectedRoot()
                            }

                            Button {
                                text: "Remove"

                                enabled:
                                    libraryController.selectedRootId !== 0

                                onClicked:
                                    libraryController.removeSelectedRoot()
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    color: "#111111"

                    ColumnLayout {
                        anchors.fill: parent

                        anchors.margins: 24

                        spacing: 14

                        RowLayout {
                            Layout.fillWidth: true

                            spacing: 10

                            Button {
                                text: "←"

                                enabled:
                                    !libraryController.atRoot

                                onClicked:
                                    libraryController.goUp()
                            }

                            Button {
                                text: "Home"

                                enabled:
                                    libraryController.selectedRootId !== 0

                                onClicked:
                                    libraryController.goToRoot()
                            }

                            Label {
                                text: "›"

                                color: "#555555"
                            }

                            Label {
                                Layout.fillWidth: true

                                text:
                                    libraryController.currentFolderName === ""
                                    ? "Library"
                                    : libraryController.currentFolderName

                                color: "white"

                                font.pixelSize: 26
                                font.bold: true

                                elide:
                                    Text.ElideRight
                            }
                        }

                        Label {
                            Layout.fillWidth: true

                            text:
                                libraryController.currentFolderPath

                            color: "#666666"

                            font.pixelSize: 11

                            elide:
                                Text.ElideMiddle
                        }

                        Label {
                            visible:
                                libraryController.errorMessage.length > 0

                            Layout.fillWidth: true

                            text:
                                libraryController.errorMessage

                            color: "#ff7777"

                            wrapMode:
                                Text.Wrap
                        }

                        Label {
                            visible:
                                libraryController.folders.length > 0

                            text: "Folders"

                            color: "#AAAAAA"

                            font.pixelSize: 14
                            font.bold: true
                        }

                        GridView {
                            id: folderGrid

                            visible:
                                libraryController.folders.length > 0

                            Layout.fillWidth: true

                            Layout.preferredHeight:
                                Math.min(
                                    220,
                                    Math.max(
                                        1,
                                        Math.ceil(
                                            libraryController.folders.length / 4
                                        )
                                    ) * 72
                                )

                            cellWidth: 210
                            cellHeight: 64

                            clip: true

                            model:
                                libraryController.folders

                            delegate: Rectangle {
                                required property var modelData

                                width: 196
                                height: 54

                                radius: 8

                                color: "#191919"

                                border.color:
                                    "#292929"

                                MouseArea {
                                    anchors.fill: parent

                                    onClicked:
                                        libraryController.openFolder(
                                            modelData.id
                                        )
                                }

                                RowLayout {
                                    anchors.fill: parent

                                    anchors.margins: 10

                                    spacing: 10

                                    Label {
                                        text: "▰"

                                        color: "#d9a84d"

                                        font.pixelSize: 18
                                    }

                                    Label {
                                        Layout.fillWidth: true

                                        text:
                                            modelData.name

                                        color: "white"

                                        font.pixelSize: 13

                                        elide:
                                            Text.ElideRight
                                    }
                                }
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Label {
                                text:
                                    "Media"

                                color: "#AAAAAA"

                                font.pixelSize: 14
                                font.bold: true

                                Layout.fillWidth: true
                            }

                            Label {
                                text:
                                    libraryController.media.length +
                                    " item(s)"

                                color: "#666666"

                                font.pixelSize: 11
                            }
                        }

                        GridView {
                            id: mediaGrid

                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            clip: true

                            cellWidth: 230
                            cellHeight: 190

                            model:
                                libraryController.media

                            delegate: Rectangle {
                                required property var modelData

                                width: 214
                                height: 174

                                radius: 8

                                color: "#191919"

                                border.color:
                                    "#292929"

                                MouseArea {
                                    anchors.fill: parent

                                    onClicked: {
                                        imageViewer.openMedia(
                                            modelData.path
                                        )

                                        root.panX = 0
                                        root.panY = 0
                                        gifPlaybackTimer.frame = 0
                                        root.viewerOpen = true

                                        imageViewer.zoom =
                                            Math.max(
                                                0.05,
                                                Math.min(
                                                    (
                                                        viewer.width - 32
                                                    ) /
                                                    imageViewer.imageWidth,
                                                    (
                                                        viewer.height -
                                                        64 -
                                                        120 -
                                                        32
                                                    ) /
                                                    imageViewer.imageHeight
                                                )
                                            )
                                    }
                                }

                                ColumnLayout {
                                    anchors.fill: parent

                                    anchors.margins: 8

                                    spacing: 6

                                    Rectangle {
                                        Layout.fillWidth: true
                                        Layout.preferredHeight: 130

                                        radius: 6

                                        color: "#101010"

                                        clip: true

                                        Image {
                                            anchors.fill: parent

                                            anchors.margins: 2

                                            source:
                                                modelData.thumbnailUrl

                                            visible:
                                                modelData.thumbnailUrl.length > 0

                                            fillMode:
                                                Image.PreserveAspectFit

                                            smooth: false

                                            mipmap: false

                                            asynchronous: true
                                        }

                                        Label {
                                            anchors.centerIn: parent

                                            visible:
                                                modelData.thumbnailUrl.length === 0

                                            text:
                                                modelData.type.toUpperCase()

                                            color: "#555555"

                                            font.pixelSize: 11
                                        }
                                    }

                                    Label {
                                        Layout.fillWidth: true

                                        text:
                                            modelData.name

                                        color: "white"

                                        font.pixelSize: 13

                                        elide:
                                            Text.ElideRight
                                    }

                                    Label {
                                        Layout.fillWidth: true

                                        text:
                                            modelData.animated
                                            ? "ANIMATED"
                                            : modelData.type.toUpperCase()

                                        color: "#777777"

                                        font.pixelSize: 10

                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // ========================================================
    // IMAGE / ANIMATION VIEWER
    // ========================================================

    Rectangle {
        id: viewer

        anchors.fill: parent

        visible:
            root.viewerOpen &&
            imageViewer.opened

        z: 100

        color: "#111111"

        // ----------------------------------------------------
        // Top toolbar
        // ----------------------------------------------------

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            height: 64

            color: "#181818"

            RowLayout {
                anchors.fill: parent

                anchors.leftMargin: 14
                anchors.rightMargin: 14

                spacing: 8

                Button {
                    text: "Close"

                    onClicked: {
                        imageViewer.close()
                        root.panX = 0
                        root.panY = 0
                        root.viewerOpen = false
                        root.spritePlaying = false
                    }
                }

                Rectangle {
                    width: 1
                    height: 28

                    color: "#333333"
                }

                Label {
                    text:
                        imageViewer.spriteSheetMode
                        ? "Sprite Sheet"
                        : imageViewer.animated
                          ? "Animated Image"
                          : "Image"

                    color: "white"

                    font.pixelSize: 15
                    font.bold: true
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: "−"

                    onClicked:
                        imageViewer.zoom =
                            Math.max(
                                0.05,
                                imageViewer.zoom / 2
                            )
                }

                Label {
                    text:
                        Math.round(
                            imageViewer.zoom * 100
                        ) + "%"

                    color: "#CCCCCC"

                    width: 70

                    horizontalAlignment:
                        Text.AlignHCenter
                }

                Button {
                    text: "+"

                    onClicked:
                        imageViewer.zoom =
                            Math.min(
                                64,
                                imageViewer.zoom * 2
                            )
                }

                Button {
                    text: "Fit"

                    onClicked: {
                        var width =
                            imageViewer.spriteSheetMode
                            ? imageViewer.frameWidth
                            : imageViewer.imageWidth

                        var height =
                            imageViewer.spriteSheetMode
                            ? imageViewer.frameHeight
                            : imageViewer.imageHeight

                        if (
                            width > 0 &&
                            height > 0
                        ) {
                            imageViewer.zoom =
                                Math.max(
                                    0.05,
                                    Math.min(
                                        (
                                            viewer.width - 32
                                        ) / width,
                                        (
                                            viewer.height -
                                            64 -
                                            120 -
                                            32
                                        ) / height
                                    )
                                )

                            root.panX = 0
                            root.panY = 0
                        }
                    }
                }

                Button {
                    text: "Grid"

                    checkable: true

                    checked:
                        imageViewer.gridEnabled

                    onToggled:
                        imageViewer.gridEnabled =
                            checked
                }
            }
        }

        // ----------------------------------------------------
        // Main viewer area
        // ----------------------------------------------------

        Rectangle {
            id: imageViewport

            anchors.left: parent.left
            anchors.right: parent.right

            anchors.top:
                parent.top

            anchors.bottom:
                parent.bottom

            anchors.topMargin: 64

            color: "#111111"

            clip: true

            // Checkerboard transparency background
            Canvas {
                anchors.fill: parent

                visible:
                    imageViewer.opened

                onPaint: {
                    var context =
                        getContext("2d")

                    var size = 16

                    context.fillStyle =
                        "#1a1a1a"

                    context.fillRect(
                        0,
                        0,
                        width,
                        height
                    )

                    context.fillStyle =
                        "#222222"

                    for (
                        var y = 0;
                        y < height;
                        y += size
                    ) {
                        for (
                            var x = 0;
                            x < width;
                            x += size
                        ) {
                            if (
                                (
                                    x / size +
                                    y / size
                                ) % 2 === 0
                            ) {
                                context.fillRect(
                                    x,
                                    y,
                                    size,
                                    size
                                )
                            }
                        }
                    }
                }

                Component.onCompleted:
                    requestPaint()
            }

            Item {
                id: viewContent

                property real contentWidth:
                    (
                        imageViewer.spriteSheetMode
                        ? imageViewer.frameWidth
                        : imageViewer.imageWidth
                    ) * imageViewer.zoom

                property real contentHeight:
                    (
                        imageViewer.spriteSheetMode
                        ? imageViewer.frameHeight
                        : imageViewer.imageHeight
                    ) * imageViewer.zoom

                width: Math.max(1, contentWidth)
                height: Math.max(1, contentHeight)

                x:
                    (parent.width - width) / 2 + root.panX

                y:
                    (Math.max(1, parent.height - 120) - height) / 2 + root.panY

                // ------------------------------------------------
                // Static image viewer
                // ------------------------------------------------

                Image {
                    id: staticImage

                    anchors.centerIn: parent

                    visible:
                        imageViewer.opened &&
                        !imageViewer.animated &&
                        !imageViewer.spriteSheetMode

                    source:
                        imageViewer.sourceUrl

                    width:
                        imageViewer.imageWidth *
                        imageViewer.zoom

                    height:
                        imageViewer.imageHeight *
                        imageViewer.zoom

                    fillMode:
                        Image.Stretch

                    smooth: false

                    mipmap: false

                    asynchronous: true

                    cache: true

                    sourceClipRect:
                        Qt.rect(
                            0,
                            0,
                            imageViewer.imageWidth,
                            imageViewer.imageHeight
                        )
                }

                // ------------------------------------------------
                // Animated image viewer
                // ------------------------------------------------

                AnimatedImage {
                    id: animatedImage

                    anchors.centerIn: parent

                    visible:
                        imageViewer.opened &&
                        imageViewer.animated &&
                        !imageViewer.spriteSheetMode

                    source:
                        imageViewer.sourceUrl

                    width:
                        imageViewer.imageWidth *
                        imageViewer.zoom

                    height:
                        imageViewer.imageHeight *
                        imageViewer.zoom

                    fillMode:
                        Image.Stretch

                    smooth: false

                    mipmap: false

                    asynchronous: true

                    playing: false

                    currentFrame:
                        gifPlaybackTimer.frame

                    cache: false
                }

                // ------------------------------------------------
                // Sprite sheet viewer
                // ------------------------------------------------

                Image {
                    id: spriteImage

                    anchors.centerIn: parent

                    visible:
                        imageViewer.opened &&
                        imageViewer.spriteSheetMode

                    source:
                        imageViewer.sourceUrl

                    width:
                        imageViewer.frameWidth *
                        imageViewer.zoom

                    height:
                        imageViewer.frameHeight *
                        imageViewer.zoom

                    fillMode:
                        Image.Stretch

                    smooth: false

                    mipmap: false

                    asynchronous: true

                    sourceClipRect:
                        Qt.rect(
                            (
                                imageViewer.frameIndex %
                                imageViewer.frameColumns
                            ) *
                            imageViewer.frameWidth,

                            Math.floor(
                                imageViewer.frameIndex /
                                imageViewer.frameColumns
                            ) *
                            imageViewer.frameHeight,

                            imageViewer.frameWidth,
                            imageViewer.frameHeight
                        )
                }

                // ------------------------------------------------
                            // ------------------------------------------------
            // Pixel grid overlay
            // ------------------------------------------------

            Repeater {
                model:
                    imageViewer.gridEnabled
                    ? Math.max(
                        1,
                        Math.ceil(
                            (
                                imageViewer.spriteSheetMode
                                ? imageViewer.frameWidth
                                : imageViewer.imageWidth
                            ) /
                            Math.max(1, imageViewer.gridCellWidth)
                        )
                    )
                    : 0

                delegate: Rectangle {
                    required property int index

                    x:
                        index *
                        imageViewer.gridCellWidth *
                        imageViewer.zoom

                    y: 0
                    width: 1
                    height: viewContent.height
                    color: "#88ffffff"
                }
            }

            Repeater {
                model:
                    imageViewer.gridEnabled
                    ? Math.max(
                        1,
                        Math.ceil(
                            (
                                imageViewer.spriteSheetMode
                                ? imageViewer.frameHeight
                                : imageViewer.imageHeight
                            ) /
                            Math.max(1, imageViewer.gridCellHeight)
                        )
                    )
                    : 0

                delegate: Rectangle {
                    required property int index

                    x: 0
                    y:
                        index *
                        imageViewer.gridCellHeight *
                        imageViewer.zoom

                    width: viewContent.width
                    height: 1
                    color: "#88ffffff"
                }
            }

// Pixel inspector
            // ------------------------------------------------

            

        }

        // ----------------------------------------------------
        // Viewer navigation
        // ----------------------------------------------------

        MouseArea {
            anchors.fill: imageViewport

            hoverEnabled: true
            enabled: imageViewer.opened

            property bool panning: false
            property real lastX: 0
            property real lastY: 0

            onPressed: function(mouse) {
                if (mouse.button === Qt.LeftButton) {
                    panning = true
                    lastX = mouse.x
                    lastY = mouse.y
                }
            }

            onReleased: {
                panning = false
            }

            onCanceled: {
                panning = false
            }

            onWheel: function(wheel) {
                imageViewer.zoom =
                    wheel.angleDelta.y > 0
                    ? Math.min(
                        64,
                        imageViewer.zoom * 2
                    )
                    : Math.max(
                        0.05,
                        imageViewer.zoom / 2
                    )

                wheel.accepted = true
            }

            onPositionChanged: function(mouse) {

                if (panning) {
                    root.panX +=
                        mouse.x - lastX

                    root.panY +=
                        mouse.y - lastY

                    lastX = mouse.x
                    lastY = mouse.y

                    return
                }

                var imageWidth =
                    imageViewer.spriteSheetMode
                    ? imageViewer.frameWidth
                    : imageViewer.imageWidth

                var imageHeight =
                    imageViewer.spriteSheetMode
                    ? imageViewer.frameHeight
                    : imageViewer.imageHeight

                if (
                    imageWidth <= 0 ||
                    imageHeight <= 0
                ) {
                    return
                }

                var px =
                    Math.floor(
                        (
                            mouse.x -
                            viewContent.x
                        ) /
                        imageViewer.zoom
                    )

                var py =
                    Math.floor(
                        (
                            mouse.y -
                            viewContent.y
                        ) /
                        imageViewer.zoom
                    )

                if (
                    px >= 0 &&
                    py >= 0 &&
                    px < imageWidth &&
                    py < imageHeight
                ) {
                    imageViewer.setPixelInfo(
                        px,
                        py,
                        ""
                    )
                }
            }
        }

        // ----------------------------------------------------
        // Bottom controls
        // ----------------------------------------------------

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom

            height: 120

            color: "#181818"

            ColumnLayout {
                anchors.fill: parent

                anchors.margins: 12

                spacing: 8

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        text:
                            "Image: " +
                            imageViewer.imageWidth +
                            " × " +
                            imageViewer.imageHeight

                        color: "#AAAAAA"
                    }

                    Label {
                        text:
                            imageViewer.pixelX < 0
                            ? "Pixel: —"
                            :
                            "Pixel: " +
                            imageViewer.pixelX +
                            ", " +
                            imageViewer.pixelY

                        color: "#AAAAAA"
                    }

                    Label {
                        text:
                            "RGBA: " +
                            imageViewer.pixelRGBA

                        color: "#AAAAAA"
                    }

                    Label {
                        text:
                            "HEX: " +
                            imageViewer.pixelHex

                        color: "#AAAAAA"
                    }

                    Item {
                        Layout.fillWidth: true
                    }

                    CheckBox {
                        text: "Sprite Sheet"

                        checked:
                            imageViewer.spriteSheetMode

                        onToggled: {
                            imageViewer.spriteSheetMode =
                                checked

                            if (checked) {
                                imageViewer.frameStart = 0

                                imageViewer.frameEnd =
                                    Math.max(
                                        0,
                                        (
                                            imageViewer.frameColumns *
                                            imageViewer.frameRows
                                        ) - 1
                                    )

                                imageViewer.frameIndex = 0

                                spritePlaybackMode.currentIndex =
                                    0
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    visible:
                        imageViewer.gridEnabled

                    Label {
                        text: "Grid"

                        color: "#AAAAAA"
                    }

                    Label {
                        text: "W"

                        color: "#777777"
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.gridCellWidth

                        onValueModified:
                            imageViewer.gridCellWidth =
                                value
                    editable: true
                    }

                    Label {
                        text: "H"

                        color: "#777777"
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.gridCellHeight

                        onValueModified:
                            imageViewer.gridCellHeight =
                                value
                    editable: true
                    }
                }


                RowLayout {
                    Layout.fillWidth: true

                    visible:
                        imageViewer.spriteSheetMode

                    Label {
                        text: "Playback"

                        color: "#AAAAAA"
                    }

                    ComboBox {
                        id: spritePlaybackMode

                        model: [
                            "Complete Sheet",
                            "Single Row"
                        ]

                        currentIndex: 0

                        onCurrentIndexChanged: {
                            if (currentIndex === 0) {
                                imageViewer.frameStart = 0

                                imageViewer.frameEnd =
                                    Math.max(
                                        0,
                                        (
                                            imageViewer.frameColumns *
                                            imageViewer.frameRows
                                        ) - 1
                                    )

                                imageViewer.frameIndex = 0
                            } else {
                                var start =
                                    (
                                        spriteRow.value - 1
                                    ) *
                                    imageViewer.frameColumns

                                imageViewer.frameStart =
                                    start

                                imageViewer.frameEnd =
                                    Math.max(
                                        start,
                                        start +
                                        imageViewer.frameColumns -
                                        1
                                    )

                                imageViewer.frameIndex =
                                    start
                            }
                        }
                    }

                    Label {
                        visible:
                            spritePlaybackMode.currentIndex === 1

                        text: "Row"

                        color: "#777777"
                    }

                    SpinBox {
                        id: spriteRow

                        visible:
                            spritePlaybackMode.currentIndex === 1

                        from: 1

                        to:
                            Math.max(
                                1,
                                imageViewer.frameRows
                            )

                        value: 1

                        onValueModified: {
                            var start =
                                (
                                    value - 1
                                ) *
                                imageViewer.frameColumns

                            imageViewer.frameStart =
                                start

                            imageViewer.frameEnd =
                                Math.max(
                                    start,
                                    start +
                                    imageViewer.frameColumns -
                                    1
                                )

                            imageViewer.frameIndex =
                                start
                        }
                    editable: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    visible:
                        imageViewer.spriteSheetMode

                    Label {
                        text: "Frame"

                        color: "#AAAAAA"
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.frameWidth

                        onValueModified:
                            imageViewer.frameWidth =
                                value
                    editable: true
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.frameHeight

                        onValueModified:
                            imageViewer.frameHeight =
                                value
                    editable: true
                    }

                    Label {
                        text: "Columns"

                        color: "#AAAAAA"
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.frameColumns

                        onValueModified: {
                            imageViewer.frameColumns =
                                value

                            if (
                                spritePlaybackMode.currentIndex === 0
                            ) {
                                imageViewer.frameStart = 0

                                imageViewer.frameEnd =
                                    Math.max(
                                        0,
                                        (
                                            imageViewer.frameColumns *
                                            imageViewer.frameRows
                                        ) - 1
                                    )

                                imageViewer.frameIndex = 0
                            } else {
                                var start =
                                    (
                                        spriteRow.value - 1
                                    ) *
                                    imageViewer.frameColumns

                                imageViewer.frameStart =
                                    start

                                imageViewer.frameEnd =
                                    Math.max(
                                        start,
                                        start +
                                        imageViewer.frameColumns -
                                        1
                                    )

                                imageViewer.frameIndex =
                                    start
                            }
                        }
                    editable: true
                    }

                    Label {
                        text: "Rows"

                        color: "#AAAAAA"
                    }

                    SpinBox {
                        from: 1
                        to: 4096

                        value:
                            imageViewer.frameRows

                        onValueModified: {
                            imageViewer.frameRows =
                                value

                            spriteRow.value =
                                Math.min(
                                    spriteRow.value,
                                    imageViewer.frameRows
                                )

                            if (
                                spritePlaybackMode.currentIndex === 0
                            ) {
                                imageViewer.frameStart = 0

                                imageViewer.frameEnd =
                                    Math.max(
                                        0,
                                        (
                                            imageViewer.frameColumns *
                                            imageViewer.frameRows
                                        ) - 1
                                    )

                                imageViewer.frameIndex = 0
                            } else {
                                var start =
                                    (
                                        spriteRow.value - 1
                                    ) *
                                    imageViewer.frameColumns

                                imageViewer.frameStart =
                                    start

                                imageViewer.frameEnd =
                                    Math.max(
                                        start,
                                        start +
                                        imageViewer.frameColumns -
                                        1
                                    )

                                imageViewer.frameIndex =
                                    start
                            }
                        }
                    editable: true
                    }

                    Label {
                        text: "FPS"

                        color: "#AAAAAA"
                    }

                    SpinBox {
                        from: 1
                        to: 240

                        value:
                            imageViewer.fps

                        onValueModified:
                            imageViewer.fps =
                                value
                    editable: true
                    }

                    Button {
                        text:
                            root.spritePlaying
                            ? "Pause"
                            : "Play"

                        onClicked:
                            root.spritePlaying =
                                !root.spritePlaying
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    visible:
                        imageViewer.animated &&
                        !imageViewer.spriteSheetMode

                    Label {
                        text: "GIF speed"

                        color: "#AAAAAA"
                    }

                    Slider {
                        Layout.fillWidth: true

                        from: 25
                        to: 400

                        value:
                            imageViewer.animationSpeed

                        onMoved:
                            imageViewer.animationSpeed =
                                Math.round(value)
                    }

                    Label {
                        text:
                            imageViewer.animationSpeed +
                            "%"

                        color: "#AAAAAA"

                        width: 50
                    }

                    CheckBox {
                        text: "Loop"

                        checked:
                            imageViewer.loop

                        onToggled:
                            imageViewer.loop =
                                checked
                    }
                }
            }
        }
    }

    }
    
    // ========================================================
    // GIF playback timer
    // ========================================================

    Timer {
        id: gifPlaybackTimer

        property int frame: 0

        interval:
            imageViewer.animated &&
            animatedImage.frameCount > 0
            ? Math.max(
                1,
                Math.round(
                    (
                        imageViewer.currentAnimationDelay > 0
                        ? imageViewer.currentAnimationDelay
                        : 83
                    ) /
                    Math.max(
                        0.1,
                        imageViewer.animationSpeed / 100.0
                    )
                )
            )
            : 83

        repeat: true

        running:
            root.viewerOpen &&
            imageViewer.opened &&
            imageViewer.animated &&
            !imageViewer.spriteSheetMode &&
            animatedImage.frameCount > 1

        onTriggered: {
            if (
                animatedImage.frameCount <= 0
            ) {
                frame = 0
                return
            }

            frame =
                (
                    frame + 1
                ) %
                animatedImage.frameCount
        }

        onRunningChanged: {
            if (!running) {
                frame = 0
            }
        }
    }

    // ========================================================
    // Sprite animation timer
    // ========================================================

    Timer {
        id: spriteTimer

        interval:
            imageViewer.fps > 0
            ? 1000 / imageViewer.fps
            : 83

        repeat: true

        running:
            root.viewerOpen &&
            imageViewer.spriteSheetMode &&
            root.spritePlaying

        onTriggered: {

            if (
                imageViewer.frameIndex <
                imageViewer.frameEnd
            ) {
                imageViewer.frameIndex += 1

                return
            }

            if (imageViewer.loop) {
                imageViewer.frameIndex =
                    imageViewer.frameStart
            } else {
                root.spritePlaying = false
            }
        }
    }

    // ========================================================
    // Add root dialog
    // ========================================================

    Dialog {
        id: addRootDialog

        modal: true

        width: 520

        title: "Add Library"

        standardButtons:
            Dialog.Ok | Dialog.Cancel

        anchors.centerIn: Overlay.overlay

        ColumnLayout {
            anchors.fill: parent

            spacing: 12

            Label {
                text: "Folder path"

                color: "#AAAAAA"
            }

            TextField {
                id: pathField

                Layout.fillWidth: true

                placeholderText:
                    "/home/user/Pictures"
            }

            Label {
                text: "Library name"

                color: "#AAAAAA"
            }

            TextField {
                id: nameField

                Layout.fillWidth: true

                placeholderText:
                    "Pictures"
            }
        }

        onAccepted: {
            if (
                libraryController.addRoot(
                    pathField.text,
                    nameField.text
                )
            ) {
                pathField.clear()
                nameField.clear()
            }
        }

        onRejected: {
            pathField.clear()
            nameField.clear()
        }
    }
}
