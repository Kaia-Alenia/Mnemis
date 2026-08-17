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

} // namespace mnemis::database
