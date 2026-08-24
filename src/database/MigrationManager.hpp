#pragma once

#include "DatabaseConnection.hpp"
#include "core/errors/Result.hpp"
#include "core/logging/Logger.hpp"

#include <string>
#include <vector>

namespace mnemis::database {

class MigrationManager final {
public:
    MigrationManager(
        DatabaseConnection& database,
        core::Logger& logger
    );

    core::Result<void> migrate();

    int currentVersion() const noexcept;

private:
    struct Migration {
        int version;
        const char* resourcePath;
    };

    DatabaseConnection& m_database;
    core::Logger& m_logger;

    static const std::vector<Migration>& migrations();

    core::Result<std::string> readMigration(
        const char* resourcePath
    );

    core::Result<void> applyMigration(
        const Migration& migration
    );
};

} // namespace mnemis::database
