#include "core/logging/Logger.hpp"
#include "core/storage/AppStorage.hpp"

#include "database/DatabaseManager.hpp"
#include "library/LibraryRootRepository.hpp"

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

    return
        fs::temp_directory_path() /
        (
            "mnemis_phase11_" +
            std::to_string(value)
        );
}

}

int main()
{
    bool ok = true;

    const fs::path storageRoot =
        uniqueRoot();

    mnemis::core::AppStorage storage(
        storageRoot
    );

    auto storageResult =
        storage.ensureDirectories();

    ok &= check(
        storageResult.isSuccess(),
        "Create application storage directory"
    );

    if (!ok) {
        return 1;
    }

    ok &= check(
        storage.databasePath().filename() ==
            "mnemis.db",
        "Database filename"
    );

    mnemis::core::Logger logger;

    // --------------------------------------------------------
    // First application session
    // --------------------------------------------------------

    {
        mnemis::database::DatabaseManager database(
            logger
        );

        auto openResult =
            database.open(
                storage.databasePath()
            );

        ok &= check(
            openResult.isSuccess(),
            "Open persistent database"
        );

        if (!openResult.isSuccess()) {
            fs::remove_all(storageRoot);
            return 1;
        }

        ok &= check(
            database.isOpen(),
            "Database reports open"
        );

        ok &= check(
            database.schemaVersion() == 3,
            "Persistent database reaches schema 3"
        );

        mnemis::library::LibraryRootRepository roots(
            database.connection()
        );

        const auto rootId =
            roots.add(
                mnemis::filesystem::Path(
                    "/tmp/Mnemis/PersistentPictures"
                ),
                "Persistent Pictures"
            );

        ok &= check(
            rootId.isSuccess(),
            "Store library root in persistent database"
        );

        database.close();

        ok &= check(
            !database.isOpen(),
            "Database closes cleanly"
        );
    }

    // --------------------------------------------------------
    // Second application session
    // --------------------------------------------------------

    {
        mnemis::database::DatabaseManager database(
            logger
        );

        auto openResult =
            database.open(
                storage.databasePath()
            );

        ok &= check(
            openResult.isSuccess(),
            "Reopen persistent database"
        );

        if (!openResult.isSuccess()) {
            fs::remove_all(storageRoot);
            return 1;
        }

        ok &= check(
            database.schemaVersion() == 3,
            "Reopened database remains at schema 3"
        );

        mnemis::library::LibraryRootRepository roots(
            database.connection()
        );

        auto countResult =
            roots.count();

        ok &= check(
            countResult.isSuccess(),
            "Read persisted root count"
        );

        if (countResult.isSuccess()) {
            ok &= check(
                countResult.value() == 1,
                "Persisted library root survives restart"
            );
        }

        auto rootResult =
            roots.findByPath(
                mnemis::filesystem::Path(
                    "/tmp/Mnemis/PersistentPictures"
                )
            );

        ok &= check(
            rootResult.isSuccess(),
            "Persisted root can be found after reopen"
        );

        if (rootResult.isSuccess()) {
            ok &= check(
                rootResult.value().name ==
                    "Persistent Pictures",
                "Persisted root name survives restart"
            );
        }

        database.close();
    }

    // --------------------------------------------------------
    // Physical database file
    // --------------------------------------------------------

    ok &= check(
        fs::exists(
            storage.databasePath()
        ),
        "SQLite database file exists on disk"
    );

    std::error_code cleanupError;

    fs::remove_all(
        storageRoot,
        cleanupError
    );

    ok &= check(
        !cleanupError,
        "Cleanup persistent storage test"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Persistent storage tests passed."
        << '\n';

    return 0;
}
