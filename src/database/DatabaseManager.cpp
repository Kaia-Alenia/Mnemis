#include "DatabaseManager.hpp"

namespace mnemis::database {

DatabaseManager::DatabaseManager(
    core::Logger& logger
)
    : m_logger(logger)
{
}

core::Result<void>
DatabaseManager::open(
    const std::filesystem::path& path
)
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "DatabaseManager",
            "Database path cannot be empty"
        };
    }

    std::error_code ec;

    const auto parent =
        path.parent_path();

    if (!parent.empty()) {
        std::filesystem::create_directories(
            parent,
            ec
        );

        if (ec) {
            return core::Error{
                core::ErrorCode::IoError,
                "DatabaseManager",
                "Failed to create database directory: " +
                ec.message()
            };
        }
    }

    auto openResult =
        m_database.open(
            path.string()
        );

    if (openResult.isError()) {
        return openResult.error();
    }

    m_path = path;

    MigrationManager migrations(
        m_database,
        m_logger
    );

    auto migrationResult =
        migrations.migrate();

    if (migrationResult.isError()) {
        m_database.close();
        m_path.clear();

        return migrationResult.error();
    }

    m_logger.info(
        "Persistent database opened: " +
        m_path.string()
    );

    return core::Result<void>::success();
}

void DatabaseManager::close() noexcept
{
    m_database.close();
    m_path.clear();
}

bool DatabaseManager::isOpen() const noexcept
{
    return m_database.isOpen();
}

const std::filesystem::path&
DatabaseManager::path() const noexcept
{
    return m_path;
}

int DatabaseManager::schemaVersion() const noexcept
{
    return m_database.userVersion();
}

DatabaseConnection&
DatabaseManager::connection() noexcept
{
    return m_database;
}

const DatabaseConnection&
DatabaseManager::connection() const noexcept
{
    return m_database;
}

} // namespace mnemis::database
