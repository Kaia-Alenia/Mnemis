#include "core/logging/Logger.hpp"

#include "database/DatabaseConnection.hpp"
#include "database/MigrationManager.hpp"

#include "library/LibraryRootRepository.hpp"

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
        "Open database"
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
        "Apply migrations"
    );

    ok &= check(
        database.userVersion() == 3,
        "Schema version is 3"
    );

    mnemis::library::LibraryRootRepository repository(
        database
    );

    const auto pictures =
        repository.add(
            mnemis::filesystem::Path(
                "/tmp/Mnemis/Pictures"
            ),
            "Pictures"
        );

    const auto music =
        repository.add(
            mnemis::filesystem::Path(
                "/tmp/Mnemis/Music"
            ),
            "Music"
        );

    ok &= check(
        pictures.isSuccess(),
        "Add Pictures root"
    );

    ok &= check(
        music.isSuccess(),
        "Add Music root"
    );

    if (pictures.isSuccess() &&
        music.isSuccess()) {

        ok &= check(
            pictures.value() != music.value(),
            "Different roots have different IDs"
        );
    }

    auto count =
        repository.count();

    ok &= check(
        count.isSuccess() &&
        count.value() == 2,
        "Two roots exist"
    );

    auto found =
        repository.findByPath(
            mnemis::filesystem::Path(
                "/tmp/Mnemis/Pictures/../Pictures"
            )
        );

    ok &= check(
        found.isSuccess(),
        "Find root by normalized path"
    );

    if (found.isSuccess() &&
        pictures.isSuccess()) {

        ok &= check(
            found.value().id == pictures.value(),
            "Normalized path resolves same root"
        );
    }

    auto disable =
        repository.setEnabled(
            music.value(),
            false
        );

    ok &= check(
        disable.isSuccess(),
        "Disable root"
    );

    auto enabled =
        repository.list(false);

    ok &= check(
        enabled.isSuccess() &&
        enabled.value().size() == 1,
        "One enabled root remains"
    );

    auto remove =
        repository.remove(
            music.value()
        );

    ok &= check(
        remove.isSuccess(),
        "Remove root"
    );

    auto finalCount =
        repository.count();

    ok &= check(
        finalCount.isSuccess() &&
        finalCount.value() == 1,
        "Final root count is one"
    );

    if (!ok) {
        return 1;
    }

    std::cout
        << "Library root tests passed."
        << '\n';

    return 0;
}
