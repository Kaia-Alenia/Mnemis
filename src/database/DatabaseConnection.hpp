#pragma once
#include "core/Result.hpp"
#include <sqlite3.h>
#include <string_view>
#include <memory>

namespace mnemis::database {

class DatabaseConnection {
public:
    DatabaseConnection();
    ~DatabaseConnection();

    // Disable copy
    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    core::Result<void> open(std::string_view path);
    void close();

    sqlite3* getHandle() const { return m_db; }
    bool isOpen() const { return m_db != nullptr; }

    core::Result<void> execute(std::string_view sql);

private:
    sqlite3* m_db = nullptr;
};

class TransactionGuard {
public:
    explicit TransactionGuard(DatabaseConnection& conn);
    ~TransactionGuard();

    core::Result<void> begin();
    core::Result<void> commit();
    void rollback();

private:
    DatabaseConnection& m_conn;
    bool m_active = false;
};

class ScopedStatement {
public:
    explicit ScopedStatement(sqlite3_stmt* stmt = nullptr);
    ~ScopedStatement();

    ScopedStatement(const ScopedStatement&) = delete;
    ScopedStatement& operator=(const ScopedStatement&) = delete;

    ScopedStatement(ScopedStatement&& other) noexcept;
    ScopedStatement& operator=(ScopedStatement&& other) noexcept;

    sqlite3_stmt* get() const { return m_stmt; }
    operator sqlite3_stmt*() const { return m_stmt; }

    void reset(sqlite3_stmt* stmt = nullptr);

private:
    sqlite3_stmt* m_stmt = nullptr;
};

} // namespace mnemis::database
