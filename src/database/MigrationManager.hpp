#pragma once
#include "DatabaseConnection.hpp"
#include "core/Result.hpp"
#include "core/ILogger.hpp"

namespace mnemis::database {

class MigrationManager {
public:
    MigrationManager(DatabaseConnection& conn, core::ILogger& logger);
    
    core::Result<void> runMigrations();

private:
    core::Result<int> getCurrentVersion();
    core::Result<void> setVersion(int version);
    core::Result<void> applyMigration(int version, const std::string& sql);

    DatabaseConnection& m_conn;
    core::ILogger& m_logger;
};

} // namespace mnemis::database
