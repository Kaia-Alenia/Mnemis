#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"

#include "filesystem/models/Path.hpp"

#include "library/service/LibraryService.hpp"
#include "media/thumbnails/ThumbnailService.hpp"

#include "ui/controllers/LibraryController.hpp"

#include <QCoreApplication>

#include <chrono>
#include <filesystem>
#include <fstream>
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
    const auto now =
        std::chrono::steady_clock::now()
            .time_since_epoch()
            .count();

    return fs::temp_directory_path() /
        (
            "mnemis_phase14_" +
            std::to_string(now)
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
        uniqueRoot();

    std::error_code ec;

    fs::create_directories(
        root / "Pictures" / "Sprites",
        ec
    );

    fs::create_directories(
        root / "Music",
        ec
    );

    if (ec) {
        return 1;
    }

    std::ofstream(
        root / "root.png"
    ) << "image";

    std::ofstream(
        root / "Pictures" / "picture.png"
    ) << "image";

    std::ofstream(
        root / "Pictures" / "Sprites" / "sprite.png"
    ) << "image";

    std::ofstream(
        root / "Music" / "theme.mp3"
    ) << "audio";

    // ========================================================
    // Database
    // ========================================================

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

    // ========================================================
    // Service / controller
    // ========================================================

    mnemis::library::LibraryService service(
        database,
        logger
    );

    mnemis::media::ThumbnailService thumbnails(
        fs::temp_directory_path() / "mnemis_test_thumbnails"
    );

    mnemis::ui::LibraryController controller(
        service,
        thumbnails
    );

    auto added =
        controller.addRoot(
            QString::fromStdString(
                root.string()
            ),
            QStringLiteral(
                "Test Library"
            )
        );

    ok &= check(
        added,
        "Add library root"
    );

    ok &= check(
        controller.roots().size() == 1,
        "One root exposed"
    );

    ok &= check(
        controller.selectedRootId() != 0,
        "Root selected"
    );

    // ========================================================
    // Before indexing
    // ========================================================

    ok &= check(
        controller.atRoot(),
        "Controller starts at root"
    );

    // ========================================================
    // Index
    // ========================================================

    auto indexed =
        controller.indexSelectedRoot();

    ok &= check(
        indexed,
        "Index selected root"
    );

    ok &= check(
        controller.media().size() == 1,
        "Root direct view contains one media item"
    );

    // ========================================================
    // Root folder navigation
    // ========================================================

    auto rootFolders =
        controller.folders();

    ok &= check(
        rootFolders.size() == 2,
        "Root exposes Pictures and Music"
    );

    qulonglong picturesId = 0;

    for (const QVariant& value :
         rootFolders) {

        const QVariantMap map =
            value.toMap();

        if (map.value(
                QStringLiteral("name")
            ).toString() == "Pictures") {

            picturesId =
                map.value(
                    QStringLiteral("id")
                ).toULongLong();

            break;
        }
    }

    ok &= check(
        picturesId != 0,
        "Pictures folder found"
    );

    // ========================================================
    // Open Pictures
    // ========================================================

    controller.openFolder(
        picturesId
    );

    ok &= check(
        !controller.atRoot(),
        "Controller leaves root"
    );

    ok &= check(
        controller.currentFolderId() ==
            picturesId,
        "Pictures selected"
    );

    ok &= check(
        controller.currentFolderName() ==
            "Pictures",
        "Current folder name is Pictures"
    );

    ok &= check(
        controller.folders().size() == 1,
        "Pictures exposes Sprites"
    );

    ok &= check(
        controller.media().size() == 1,
        "Pictures direct view contains one media"
    );

    // ========================================================
    // Open Sprites
    // ========================================================

    qulonglong spritesId = 0;

    for (const QVariant& value :
         controller.folders()) {

        const QVariantMap map =
            value.toMap();

        if (map.value(
                QStringLiteral("name")
            ).toString() == "Sprites") {

            spritesId =
                map.value(
                    QStringLiteral("id")
                ).toULongLong();

            break;
        }
    }

    ok &= check(
        spritesId != 0,
        "Sprites folder found"
    );

    controller.openFolder(
        spritesId
    );

    ok &= check(
        controller.currentFolderName() ==
            "Sprites",
        "Current folder is Sprites"
    );

    ok &= check(
        controller.folders().isEmpty(),
        "Sprites has no child folders"
    );

    ok &= check(
        controller.media().size() == 1,
        "Sprites contains one media"
    );

    // ========================================================
    // Go up
    // ========================================================

    controller.goUp();

    ok &= check(
        controller.currentFolderName() ==
            "Pictures",
        "Go up returns to Pictures"
    );

    ok &= check(
        controller.media().size() == 1,
        "Pictures media restored after going up"
    );

    controller.goUp();

    ok &= check(
        controller.atRoot(),
        "Go up returns to root"
    );

    ok &= check(
        controller.media().size() == 1,
        "Root media restored"
    );

    // ========================================================
    // Select root again
    // ========================================================

    const qulonglong rootId =
        controller.selectedRootId();

    controller.selectRoot(
        rootId
    );

    ok &= check(
        controller.atRoot(),
        "Selecting root resets browser to root"
    );

    // ========================================================
    // Cleanup
    // ========================================================

    fs::remove_all(
        root,
        ec
    );

    ok &= check(
        !ec,
        "Cleanup"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Library browser controller tests passed."
        << '\n';

    return 0;
}
