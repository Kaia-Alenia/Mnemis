#include "DatabaseConnection.hpp"


namespace mnemis::database {

DatabaseConnection::DatabaseConnection() = default;

DatabaseConnection::~DatabaseConnection() {
    close();
}

core::Result<void> DatabaseConnection::open(std::string_view path) {
    if (m_db) {
        return core::Result<void>(core::Error{2, "Database already open"});
    }

    int rc = sqlite3_open_v2(path.data(), &m_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr);
    if (rc != SQLITE_OK) {
        std::string errMsg = sqlite3_errmsg(m_db);
        close();
        return core::Result<void>(core::Error{rc, "Failed to open database: " + errMsg});
    }

    // Enable foreign keys by default
    execute("PRAGMA foreign_keys = ON;");

    return core::Result<void>();
}

void DatabaseConnection::close() {
    if (m_db) {
        sqlite3_close_v2(m_db);
        m_db = nullptr;
    }
}

core::Result<void> DatabaseConnection::execute(std::string_view sql) {
    if (!m_db) {
        return core::Result<void>(core::Error{1, "Database not open"});
    }

    char* errMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.data(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::string msg = errMsg ? errMsg : "Unknown SQL execution error";
        sqlite3_free(errMsg);
        return core::Result<void>(core::Error{rc, msg});
    }

    return core::Result<void>();
}

// --- TransactionGuard ---

TransactionGuard::TransactionGuard(DatabaseConnection& conn) : m_conn(conn) {}

TransactionGuard::~TransactionGuard() {
    if (m_active) {
        rollback();
    }
}

core::Result<void> TransactionGuard::begin() {
    if (m_active) {
        return core::Result<void>(core::Error{1, "Transaction already active"});
    }
    auto res = m_conn.execute("BEGIN TRANSACTION;");
    if (res.isSuccess()) {
        m_active = true;
    }
    return res;
}

core::Result<void> TransactionGuard::commit() {
    if (!m_active) {
        return core::Result<void>(core::Error{1, "No active transaction to commit"});
    }
    auto res = m_conn.execute("COMMIT;");
    if (res.isSuccess()) {
        m_active = false;
    }
    return res;
}

void TransactionGuard::rollback() {
    if (m_active) {
        m_conn.execute("ROLLBACK;");
        m_active = false;
    }
}

} // namespace mnemis::database
