#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "core/logging/Logger.hpp"
#include "library/service/LibraryService.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;

static bool check(bool condition, const char* message)
{
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        return false;
    }

    return true;
}

int main()
{
    bool ok = true;

    const auto root =
        fs::temp_directory_path() / "mnemis_phase12_test";

    std::error_code ec;

    fs::remove_all(root, ec);

    fs::create_directories(
        root / "Pictures",
        ec
    );

    std::ofstream(
        root / "Pictures" / "image.png"
    ) << "image";

    std::ofstream(
        root / "music.mp3"
    ) << "audio";

    mnemis::database::DatabaseConnection database;

    auto open =
        database.open(":memory:");

    ok &= check(
        open.isSuccess(),
        "Open database"
    );

    mnemis::core::Logger logger;

    mnemis::database::MigrationManager migration(
        database,
        logger
    );

    auto migrated =
        migration.migrate();

    ok &= check(
        migrated.isSuccess(),
        "Migrate database"
    );

    mnemis::library::LibraryService service(
        database,
        logger
    );

    auto rootId =
        service.addRoot(
            mnemis::filesystem::Path(
                root.string()
            ),
            "Test"
        );

    ok &= check(
        rootId.isSuccess(),
        "Add root"
    );

    if (rootId.isSuccess()) {

        auto roots =
            service.getRoots();

        ok &= check(
            roots.isSuccess() &&
            roots.value().size() == 1,
            "List root"
        );

        auto index =
            service.indexRoot(
                rootId.value()
            );

        ok &= check(
            index.isSuccess(),
            "Index root"
        );

        auto media =
            service.mediaInRoot(
                rootId.value(),
                true
            );

        ok &= check(
            media.isSuccess(),
            "Query root media"
        );

        if (media.isSuccess()) {
            ok &= check(
                media.value().size() == 2,
                "Two media files found"
            );
        }

        auto outside =
            service.mediaInFolder(
                rootId.value(),
                mnemis::filesystem::Path(
                    "/tmp/outside"
                ),
                true
            );

        ok &= check(
            outside.isError(),
            "Reject path outside root"
        );
    }

    fs::remove_all(root, ec);

    ok &= check(
        !ec,
        "Cleanup"
    );

    return ok ? 0 : 1;
}
