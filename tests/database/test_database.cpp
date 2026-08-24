#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"
#include "core/logging/Logger.hpp"

#include <iostream>

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

}

int main()
{
    bool ok = true;

    mnemis::database::DatabaseConnection database;

    auto openResult =
        database.open(":memory:");

    ok &= check(
        openResult.isSuccess(),
        "Opening in-memory database"
    );

    ok &= check(
        database.isOpen(),
        "Database reports open"
    );

    ok &= check(
        database.userVersion() == 0,
        "Initial database version is 0"
    );

    if (!ok) {
        return 1;
    }

    mnemis::core::Logger logger;

    mnemis::database::MigrationManager migrations(
        database,
        logger
    );

    auto migrationResult =
        migrations.migrate();

    ok &= check(
        migrationResult.isSuccess(),
        "Initial migration succeeds"
    );

    ok &= check(
        database.userVersion() == 3,
        "Database version becomes 3"
    );

    auto tableCheck =
        database.scalarInt64(
            "SELECT COUNT(*) "
            "FROM sqlite_master "
            "WHERE type = 'table' "
            "AND name = 'media';"
        );

    ok &= check(
        tableCheck.isSuccess(),
        "Media table query succeeds"
    );

    if (tableCheck.isSuccess()) {
        ok &= check(
            tableCheck.value() == 1,
            "Media table exists"
        );
    }

    auto rootTableCheck =
        database.scalarInt64(
            "SELECT COUNT(*) "
            "FROM sqlite_master "
            "WHERE type = 'table' "
            "AND name = 'library_roots';"
        );

    ok &= check(
        rootTableCheck.isSuccess(),
        "Library roots table query succeeds"
    );

    if (rootTableCheck.isSuccess()) {
        ok &= check(
            rootTableCheck.value() == 1,
            "Library roots table exists"
        );
    }

    auto folderTableCheck =
        database.scalarInt64(
            "SELECT COUNT(*) "
            "FROM sqlite_master "
            "WHERE type = 'table' "
            "AND name = 'folders';"
        );

    ok &= check(
        folderTableCheck.isSuccess(),
        "Folders table query succeeds"
    );

    if (folderTableCheck.isSuccess()) {
        ok &= check(
            folderTableCheck.value() == 1,
            "Folders table exists"
        );
    }

    auto secondMigration =
        migrations.migrate();

    ok &= check(
        secondMigration.isSuccess(),
        "Second migration run succeeds"
    );

    ok &= check(
        database.userVersion() == 3,
        "Migration is idempotent"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Database foundation tests passed."
        << '\n';

    return 0;
}
