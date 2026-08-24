#pragma once

#include "core/errors/Result.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <string>

namespace mnemis::database {

class DatabaseConnection final {
public:
    DatabaseConnection() = default;

    ~DatabaseConnection();

    DatabaseConnection(const DatabaseConnection&) = delete;
    DatabaseConnection& operator=(const DatabaseConnection&) = delete;

    DatabaseConnection(DatabaseConnection&& other) noexcept;
    DatabaseConnection& operator=(DatabaseConnection&& other) noexcept;

    core::Result<void> open(const std::string& path);
    void close() noexcept;

    bool isOpen() const noexcept;

    sqlite3* handle() noexcept;
    const sqlite3* handle() const noexcept;

    const std::string& path() const noexcept;

    core::Result<void> execute(const std::string& sql);

    core::Result<std::int64_t> scalarInt64(const std::string& sql);

    core::Result<void> beginTransaction();
    core::Result<void> commit();
    core::Result<void> rollback();

    int userVersion() const noexcept;
    core::Result<void> setUserVersion(int version);

private:
    sqlite3* m_handle = nullptr;
    std::string m_path;

    void moveFrom(DatabaseConnection&& other) noexcept;
};

} // namespace mnemis::database
