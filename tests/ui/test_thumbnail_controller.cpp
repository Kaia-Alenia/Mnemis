#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"

#include "filesystem/models/Path.hpp"

#include "library/service/LibraryService.hpp"

#include "media/thumbnails/ThumbnailService.hpp"

#include "ui/controllers/LibraryController.hpp"

#include <QCoreApplication>
#include <QImage>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

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
            "mnemis_phase16_" +
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

    const fs::path cache =
        root / "thumbnail-cache";

    const fs::path imagePath =
        root / "pixel.png";

    std::error_code ec;

    fs::create_directories(
        root,
        ec
    );

    if (ec) {
        return 1;
    }

    // --------------------------------------------------------
    // Create image
    // --------------------------------------------------------

    QImage image(
        32,
        32,
        QImage::Format_RGBA8888
    );

    image.fill(
        Qt::transparent
    );

    for (int y = 8; y < 24; ++y) {
        for (int x = 8; x < 24; ++x) {
            image.setPixelColor(
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
        image.save(
            QString::fromStdString(
                imagePath.string()
            ),
            "PNG"
        ),
        "Create test image"
    );

    // --------------------------------------------------------
    // Database
    // --------------------------------------------------------

    mnemis::database::DatabaseConnection database;

    auto openResult =
        database.open(":memory:");

    ok &= check(
        openResult.isSuccess(),
        "Open database"
    );

    mnemis::core::Logger logger;

    mnemis::database::MigrationManager migrations(
        database,
        logger
    );

    auto migrationResult =
        migrations.migrate();

    ok &= check(
        migrationResult.isSuccess(),
        "Apply migrations"
    );

    if (!ok) {
        fs::remove_all(root, ec);
        return 1;
    }

    // --------------------------------------------------------
    // Services
    // --------------------------------------------------------

    mnemis::library::LibraryService library(
        database,
        logger
    );

    mnemis::media::ThumbnailService thumbnails(
        cache
    );

    mnemis::ui::LibraryController controller(
        library,
        thumbnails
    );

    auto rootResult =
        controller.addRoot(
            QString::fromStdString(
                root.string()
            ),
            QStringLiteral(
                "Images"
            )
        );

    ok &= check(
        rootResult,
        "Controller creates library root"
    );

    ok &= check(
        controller.indexSelectedRoot(),
        "Controller indexes image"
    );

    ok &= check(
        controller.media().size() == 1,
        "Controller exposes one media item"
    );

    // --------------------------------------------------------
    // Thumbnail URL
    // --------------------------------------------------------

    if (!controller.media().isEmpty()) {

        const QVariantMap media =
            controller.media()
                .first()
                .toMap();

        const QString thumbnailUrl =
            media.value(
                QStringLiteral(
                    "thumbnailUrl"
                )
            ).toString();

        ok &= check(
            !thumbnailUrl.isEmpty(),
            "Controller exposes thumbnail URL"
        );

        if (!thumbnailUrl.isEmpty()) {

            const QUrl url(
                thumbnailUrl
            );

            ok &= check(
                url.isLocalFile(),
                "Thumbnail URL is a local file URL"
            );

            const fs::path thumbnailPath =
                url.toLocalFile()
                    .toStdString();

            ok &= check(
                fs::exists(thumbnailPath),
                "Thumbnail file exists"
            );

            QImage thumbnail(
                url.toLocalFile()
            );

            ok &= check(
                !thumbnail.isNull(),
                "Thumbnail can be decoded"
            );

            if (!thumbnail.isNull()) {
                ok &= check(
                    thumbnail.width() == 256 &&
                    thumbnail.height() == 256,
                    "Controller thumbnail uses 256x256 bounds"
                );
            }
        }
    }

    // --------------------------------------------------------
    // Cache reuse
    // --------------------------------------------------------

    auto secondMedia =
        controller.media();

    ok &= check(
        secondMedia.size() == 1,
        "Media remains available"
    );

    const int cacheFilesBefore =
        [&cache]() -> int {
            if (!fs::exists(cache)) {
                return 0;
            }

            int count = 0;

            for (const auto& entry :
                 fs::directory_iterator(cache)) {

                if (entry.is_regular_file()) {
                    ++count;
                }
            }

            return count;
        }();

    controller.refreshMedia();

    const int cacheFilesAfter =
        [&cache]() -> int {
            if (!fs::exists(cache)) {
                return 0;
            }

            int count = 0;

            for (const auto& entry :
                 fs::directory_iterator(cache)) {

                if (entry.is_regular_file()) {
                    ++count;
                }
            }

            return count;
        }();

    ok &= check(
        cacheFilesBefore == 1 &&
        cacheFilesAfter == 1,
        "Thumbnail cache is reused"
    );

    // --------------------------------------------------------
    // Cleanup
    // --------------------------------------------------------

    fs::remove_all(
        root,
        ec
    );

    ok &= check(
        !ec,
        "Cleanup thumbnail controller test"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Thumbnail controller integration tests passed."
        << '\n';

    return 0;
}
