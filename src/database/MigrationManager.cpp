#include "MigrationManager.hpp"

#include <QFile>
#include <QTextStream>
#include <QString>
#include <vector>

namespace mnemis::database {

MigrationManager::MigrationManager(DatabaseConnection& conn, core::ILogger& logger)
    : m_conn(conn), m_logger(logger) {}

core::Result<int> MigrationManager::getCurrentVersion() {
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), "PRAGMA user_version;", -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        return core::Result<int>(core::Error{rc, "Failed to prepare user_version query"});
    }

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return core::Result<int>(version);
}

core::Result<void> MigrationManager::setVersion(int version) {
    std::string sql = "PRAGMA user_version = " + std::to_string(version) + ";";
    return m_conn.execute(sql);
}

core::Result<void> MigrationManager::applyMigration(int version, const std::string& sql) {
    TransactionGuard txn(m_conn);
    auto res = txn.begin();
    if (res.isError()) return res;

    res = m_conn.execute(sql);
    if (res.isError()) {
        m_logger.log(core::LogLevel::Error, "[MigrationManager] Failed to apply migration " + std::to_string(version) + ": " + res.error().message);
        return res; // Rollback happens automatically in destructor
    }

    res = setVersion(version);
    if (res.isError()) return res;

    return txn.commit();
}

core::Result<void> MigrationManager::runMigrations() {
    auto versionRes = getCurrentVersion();
    if (versionRes.isError()) return core::Result<void>(versionRes.error());

    int currentVersion = versionRes.value();
    m_logger.log(core::LogLevel::Info, "[MigrationManager] Current database version: " + std::to_string(currentVersion));

    // Hardcoded migration list for now. In a larger system, we'd scan the QDir.
    struct Migration {
        int version;
        std::string resourcePath;
    };

    std::vector<Migration> migrations = {
        {1, ":/database/migrations/V001_initial.sql"},
        {2, ":/database/migrations/V002_add_path_index.sql"},
        {3, ":/database/migrations/V003_add_playlists.sql"},
        {4, ":/database/migrations/V004_add_playback_position.sql"}
    };

    for (const auto& migration : migrations) {
        if (migration.version > currentVersion) {
            m_logger.log(core::LogLevel::Info, "[MigrationManager] Applying migration " + std::to_string(migration.version));
            
            QFile file(QString::fromStdString(migration.resourcePath));
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                return core::Result<void>(core::Error{1, "Failed to open migration file: " + migration.resourcePath});
            }

            QTextStream in(&file);
            QString sql = in.readAll();
            file.close();

            auto res = applyMigration(migration.version, sql.toStdString());
            if (res.isError()) {
                return res;
            }
            m_logger.log(core::LogLevel::Info, "[MigrationManager] Successfully applied migration " + std::to_string(migration.version));
        }
    }

    return core::Result<void>();
}

} // namespace mnemis::database
