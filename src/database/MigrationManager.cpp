#include "MigrationManager.hpp"

#include <QFile>

namespace mnemis::database {

MigrationManager::MigrationManager(
    DatabaseConnection& database,
    core::Logger& logger
)
    : m_database(database)
    , m_logger(logger)
{
}

const std::vector<MigrationManager::Migration>&
MigrationManager::migrations()
{
    static const std::vector<MigrationManager::Migration> list = {
        {
            1,
            ":/database/migrations/V001_initial.sql"
        },
        {
            2,
            ":/database/migrations/V002_add_library_roots.sql"
        },
        {
            3,
            ":/database/migrations/V003_add_library_folders.sql"
        }
    };

    return list;
}

core::Result<std::string>
MigrationManager::readMigration(
    const char* resourcePath
)
{
    if (resourcePath == nullptr ||
        *resourcePath == '\0') {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Migration",
            "Migration resource path is empty"
        };
    }

    QFile file(
        QString::fromUtf8(resourcePath)
    );

    if (!file.open(
            QIODevice::ReadOnly |
            QIODevice::Text)) {

        return core::Error{
            core::ErrorCode::NotFound,
            "Migration",
            "Could not open migration resource: " +
            std::string(resourcePath)
        };
    }

    const QByteArray data =
        file.readAll();

    if (data.isEmpty()) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "Migration",
            "Migration file is empty: " +
            std::string(resourcePath)
        };
    }

    return data.toStdString();
}

core::Result<void>
MigrationManager::applyMigration(
    const Migration& migration
)
{
    m_logger.info(
        "Applying database migration " +
        std::to_string(migration.version)
    );

    auto sqlResult =
        readMigration(
            migration.resourcePath
        );

    if (sqlResult.isError()) {
        return sqlResult.error();
    }

    auto beginResult =
        m_database.beginTransaction();

    if (beginResult.isError()) {
        return beginResult.error();
    }

    auto executeResult =
        m_database.execute(
            sqlResult.value()
        );

    if (executeResult.isError()) {
        auto rollbackResult =
            m_database.rollback();

        if (rollbackResult.isError()) {
            m_logger.error(
                "Database rollback failed: " +
                rollbackResult.error().message()
            );
        }

        return executeResult.error();
    }

    auto versionResult =
        m_database.setUserVersion(
            migration.version
        );

    if (versionResult.isError()) {
        auto rollbackResult =
            m_database.rollback();

        if (rollbackResult.isError()) {
            m_logger.error(
                "Database rollback failed: " +
                rollbackResult.error().message()
            );
        }

        return versionResult.error();
    }

    auto commitResult =
        m_database.commit();

    if (commitResult.isError()) {
        auto rollbackResult =
            m_database.rollback();

        if (rollbackResult.isError()) {
            m_logger.error(
                "Database rollback failed: " +
                rollbackResult.error().message()
            );
        }

        return commitResult.error();
    }

    m_logger.info(
        "Database migration " +
        std::to_string(migration.version) +
        " applied"
    );

    return core::Result<void>::success();
}

core::Result<void>
MigrationManager::migrate()
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "Migration",
            "Database must be open before migrations run"
        };
    }

    const int installedVersion =
        m_database.userVersion();

    m_logger.info(
        "Database version: " +
        std::to_string(installedVersion)
    );

    for (const Migration& migration :
         migrations()) {

        if (migration.version <=
            installedVersion) {
            continue;
        }

        auto result =
            applyMigration(migration);

        if (result.isError()) {
            return result.error();
        }
    }

    return core::Result<void>::success();
}

int MigrationManager::currentVersion() const noexcept
{
    return m_database.userVersion();
}

} // namespace mnemis::database
