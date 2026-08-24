#include "media/thumbnails/ThumbnailService.hpp"

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

fs::path uniqueDirectory()
{
    const auto value =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    return fs::temp_directory_path() /
        (
            "mnemis_phase15_" +
            std::to_string(value)
        );
}

}

int main(
    int argc,
    char** argv
)
{
    QCoreApplication application(
        argc,
        argv
    );

    bool ok = true;

    const fs::path root =
        uniqueDirectory();

    const fs::path imagePath =
        root / "pixel_art.png";

    const fs::path cachePath =
        root / "cache";

    std::error_code error;

    fs::create_directories(
        root,
        error
    );

    if (error) {
        return 1;
    }

    // --------------------------------------------------------
    // Create deterministic pixel-art style image
    // --------------------------------------------------------

    QImage source(
        32,
        32,
        QImage::Format_RGBA8888
    );

    source.fill(
        Qt::transparent
    );

    for (int y = 8; y < 24; ++y) {
        for (int x = 8; x < 24; ++x) {
            source.setPixelColor(
                x,
                y,
                QColor(
                    255,
                    255,
                    255,
                    255
                )
            );
        }
    }

    ok &= check(
        source.save(
            QString::fromStdString(
                imagePath.string()
            ),
            "PNG"
        ),
        "Create source image"
    );

    // --------------------------------------------------------
    // Media object
    // --------------------------------------------------------

    mnemis::media::Media media;

    media.id = 1;

    media.type =
        mnemis::media::MediaType::Image;

    media.canonicalPath =
        imagePath.string();

    media.fileName =
        "pixel_art.png";

    media.extension =
        ".png";

    media.fileSize =
        static_cast<std::int64_t>(
            fs::file_size(
                imagePath
            )
        );

    media.modifiedTime = 1;

    // --------------------------------------------------------
    // Service
    // --------------------------------------------------------

    mnemis::media::ThumbnailService thumbnails(
        cachePath
    );

    auto firstResult =
        thumbnails.thumbnailFor(
            media,
            128,
            128
        );

    ok &= check(
        firstResult.isSuccess(),
        "Generate first thumbnail"
    );

    if (firstResult.isSuccess()) {

        const fs::path generated =
            firstResult.value();

        ok &= check(
            fs::exists(generated),
            "Generated thumbnail exists"
        );

        QImage thumbnail(
            QString::fromStdString(
                generated.string()
            )
        );

        ok &= check(
            !thumbnail.isNull(),
            "Generated thumbnail can be decoded"
        );

        if (!thumbnail.isNull()) {
            ok &= check(
                thumbnail.width() == 128 &&
                thumbnail.height() == 128,
                "Thumbnail preserves requested bounds"
            );

            ok &= check(
                thumbnail.pixelColor(
                    64,
                    64
                ).alpha() > 0,
                "Thumbnail retains image content"
            );
        }

        // Second request must reuse cache.
        auto secondResult =
            thumbnails.thumbnailFor(
                media,
                128,
                128
            );

        ok &= check(
            secondResult.isSuccess(),
            "Second thumbnail request succeeds"
        );

        if (secondResult.isSuccess()) {
            ok &= check(
                secondResult.value() ==
                    generated,
                "Second request reuses cached thumbnail"
            );
        }
    }

    // --------------------------------------------------------
    // Different size must use another cache entry
    // --------------------------------------------------------

    auto differentSize =
        thumbnails.thumbnailFor(
            media,
            64,
            64
        );

    ok &= check(
        differentSize.isSuccess(),
        "Different thumbnail size succeeds"
    );

    if (differentSize.isSuccess() &&
        firstResult.isSuccess()) {

        ok &= check(
            differentSize.value() !=
                firstResult.value(),
            "Different size creates separate cache key"
        );
    }

    // --------------------------------------------------------
    // Unsupported type
    // --------------------------------------------------------

    mnemis::media::Media audio =
        media;

    audio.type =
        mnemis::media::MediaType::Audio;

    auto audioResult =
        thumbnails.thumbnailFor(
            audio,
            128,
            128
        );

    ok &= check(
        audioResult.isError(),
        "Audio thumbnail is rejected in image-only phase"
    );

    // --------------------------------------------------------
    // Invalid dimensions
    // --------------------------------------------------------

    auto invalidSize =
        thumbnails.thumbnailFor(
            media,
            0,
            128
        );

    ok &= check(
        invalidSize.isError(),
        "Invalid thumbnail dimensions are rejected"
    );

    // --------------------------------------------------------
    // Clear cache
    // --------------------------------------------------------

    auto clearResult =
        thumbnails.clearCache();

    ok &= check(
        clearResult.isSuccess(),
        "Clear thumbnail cache"
    );

    ok &= check(
        !fs::exists(cachePath),
        "Thumbnail cache directory removed"
    );

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    fs::remove_all(
        root,
        error
    );

    ok &= check(
        !error,
        "Cleanup thumbnail test"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Thumbnail service tests passed."
        << '\n';

    return 0;
}
