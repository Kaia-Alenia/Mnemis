#include "ui/controllers/ImageViewerController.hpp"

#include <QCoreApplication>
#include <QImage>

#include <chrono>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

namespace {

bool check(
    bool condition,
    const char* message
)
{
    if (!condition) {
        std::cerr
            << "FAIL: "
            << message
            << '\n';

        return false;
    }

    return true;
}

fs::path uniqueRoot()
{
    const auto value =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    return fs::temp_directory_path() /
        (
            "mnemis_phase17_" +
            std::to_string(value)
        );
}

}

int main(
    int argc,
    char** argv
)
{
    QCoreApplication app(
        argc,
        argv
    );

    bool ok = true;

    const fs::path root =
        uniqueRoot();

    fs::create_directories(root);

    const fs::path imagePath =
        root / "pixel.png";

    QImage image(
        48,
        48,
        QImage::Format_RGBA8888
    );

    image.fill(
        Qt::transparent
    );

    for (int y = 0; y < 24; ++y) {
        for (int x = 0; x < 24; ++x) {
            image.setPixelColor(
                x,
                y,
                QColor(
                    255,
                    0,
                    0,
                    255
                )
            );
        }
    }

    ok &= check(
        image.save(
            QString::fromStdString(
                imagePath.string()
            ),
            "PNG"
        ),
        "Create test image"
    );

    mnemis::ui::ImageViewerController controller;

    controller.openMedia(
        QString::fromStdString(
            imagePath.string()
        )
    );

    ok &= check(
        controller.opened(),
        "Image opens"
    );

    ok &= check(
        !controller.animated(),
        "PNG is not animated"
    );

    ok &= check(
        controller.imageWidth() == 48 &&
        controller.imageHeight() == 48,
        "Image dimensions are detected"
    );

    // --------------------------------------------------------
    // Zoom
    // --------------------------------------------------------

    controller.setZoom(
        8.0
    );

    ok &= check(
        controller.zoom() == 8.0,
        "Integer zoom works"
    );

    controller.setZoom(
        100.0
    );

    ok &= check(
        controller.zoom() == 64.0,
        "Zoom is capped"
    );

    // --------------------------------------------------------
    // Grid
    // --------------------------------------------------------

    controller.setGridEnabled(
        true
    );

    controller.setGridCellWidth(
        48
    );

    controller.setGridCellHeight(
        48
    );

    ok &= check(
        controller.gridEnabled(),
        "Grid can be enabled"
    );

    ok &= check(
        controller.gridCellWidth() == 48 &&
        controller.gridCellHeight() == 48,
        "Grid dimensions work"
    );

    // --------------------------------------------------------
    // Pixel inspector
    // --------------------------------------------------------

    controller.setPixelInfo(
        23,
        17,
        QStringLiteral(
            "#FF0000FF"
        )
    );

    ok &= check(
        controller.pixelX() == 23 &&
        controller.pixelY() == 17,
        "Pixel coordinates work"
    );

    ok &= check(
        controller.pixelHex() ==
            "#FF0000FF",
        "Pixel hex works"
    );

    // --------------------------------------------------------
    // Sprite configuration
    // --------------------------------------------------------

    controller.setSpriteSheetMode(
        true
    );

    controller.setFrameWidth(
        48
    );

    controller.setFrameHeight(
        48
    );

    controller.setFrameColumns(
        4
    );

    controller.setFrameRows(
        3
    );

    controller.setFrameStart(
        0
    );

    controller.setFrameEnd(
        11
    );

    controller.setFrameIndex(
        7
    );

    ok &= check(
        controller.spriteSheetMode(),
        "Sprite-sheet mode works"
    );

    ok &= check(
        controller.frameWidth() == 48 &&
        controller.frameHeight() == 48,
        "Sprite frame dimensions"
    );

    ok &= check(
        controller.frameColumns() == 4 &&
        controller.frameRows() == 3,
        "Sprite grid dimensions"
    );

    ok &= check(
        controller.frameIndex() == 7,
        "Sprite frame selection"
    );

    // --------------------------------------------------------
    // Animation settings
    // --------------------------------------------------------

    controller.setFps(
        12
    );

    controller.setAnimationSpeed(
        200
    );

    controller.setLoop(
        true
    );

    ok &= check(
        controller.fps() == 12,
        "Sprite FPS"
    );

    ok &= check(
        controller.animationSpeed() == 200,
        "Animation speed"
    );

    ok &= check(
        controller.loop(),
        "Loop setting"
    );

    // --------------------------------------------------------
    // Close
    // --------------------------------------------------------

    controller.close();

    ok &= check(
        !controller.opened(),
        "Viewer closes"
    );

    ok &= check(
        controller.sourcePath().isEmpty(),
        "Viewer clears source"
    );

    std::error_code error;

    fs::remove_all(
        root,
        error
    );

    ok &= check(
        !error,
        "Cleanup"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Image viewer controller tests passed."
        << '\n';

    return 0;
}
