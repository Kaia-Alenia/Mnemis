#pragma once

#include "DatabaseConnection.hpp"
#include "MigrationManager.hpp"
#include "core/errors/Result.hpp"
#include "core/logging/Logger.hpp"

#include <filesystem>

namespace mnemis::database {

class DatabaseManager final {
public:
    explicit DatabaseManager(
        core::Logger& logger
    );

    core::Result<void> open(
        const std::filesystem::path& path
    );

    void close() noexcept;

    bool isOpen() const noexcept;

    const std::filesystem::path& path() const noexcept;

    int schemaVersion() const noexcept;

    DatabaseConnection& connection() noexcept;
    const DatabaseConnection& connection() const noexcept;

private:
    core::Logger& m_logger;
    DatabaseConnection m_database;
    std::filesystem::path m_path;
};

} // namespace mnemis::database
