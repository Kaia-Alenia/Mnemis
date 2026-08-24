#include "DatabaseConnection.hpp"

namespace mnemis::database {

namespace {

core::Error makeSqliteError(
    sqlite3* handle,
    core::ErrorCode code,
    const char* category
)
{
    const char* message =
        handle != nullptr
            ? sqlite3_errmsg(handle)
            : "SQLite database handle is not available";

    return core::Error{
        code,
        category,
        message != nullptr ? message : "Unknown SQLite error"
    };
}

} // namespace

DatabaseConnection::~DatabaseConnection()
{
    close();
}

DatabaseConnection::DatabaseConnection(DatabaseConnection&& other) noexcept
{
    moveFrom(std::move(other));
}

DatabaseConnection& DatabaseConnection::operator=(
    DatabaseConnection&& other
) noexcept
{
    if (this != &other) {
        close();
        moveFrom(std::move(other));
    }

    return *this;
}

void DatabaseConnection::moveFrom(DatabaseConnection&& other) noexcept
{
    m_handle = other.m_handle;
    m_path = std::move(other.m_path);

    other.m_handle = nullptr;
    other.m_path.clear();
}

core::Result<void> DatabaseConnection::open(const std::string& path)
{
    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Database",
            "Database path cannot be empty"
        };
    }

    close();

    sqlite3* handle = nullptr;

    const int rc = sqlite3_open_v2(
        path.c_str(),
        &handle,
        SQLITE_OPEN_READWRITE |
        SQLITE_OPEN_CREATE |
        SQLITE_OPEN_FULLMUTEX,
        nullptr
    );

    if (rc != SQLITE_OK) {
        core::Error error = makeSqliteError(
            handle,
            core::ErrorCode::DatabaseError,
            "Database"
        );

        if (handle != nullptr) {
            sqlite3_close(handle);
        }

        return error;
    }

    m_handle = handle;
    m_path = path;

    const int foreignKeysRc =
        sqlite3_exec(
            m_handle,
            "PRAGMA foreign_keys = ON;",
            nullptr,
            nullptr,
            nullptr
        );

    if (foreignKeysRc != SQLITE_OK) {
        core::Error error = makeSqliteError(
            m_handle,
            core::ErrorCode::DatabaseError,
            "Database"
        );

        close();
        return error;
    }

    return core::Result<void>::success();
}

void DatabaseConnection::close() noexcept
{
    if (m_handle != nullptr) {
        sqlite3_close(m_handle);
        m_handle = nullptr;
    }

    m_path.clear();
}

bool DatabaseConnection::isOpen() const noexcept
{
    return m_handle != nullptr;
}

sqlite3* DatabaseConnection::handle() noexcept
{
    return m_handle;
}

const sqlite3* DatabaseConnection::handle() const noexcept
{
    return m_handle;
}

const std::string& DatabaseConnection::path() const noexcept
{
    return m_path;
}

core::Result<void> DatabaseConnection::execute(const std::string& sql)
{
    if (!isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "Database",
            "Database is not open"
        };
    }

    if (sql.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Database",
            "SQL statement cannot be empty"
        };
    }

    char* errorMessage = nullptr;

    const int rc = sqlite3_exec(
        m_handle,
        sql.c_str(),
        nullptr,
        nullptr,
        &errorMessage
    );

    if (rc != SQLITE_OK) {
        std::string message =
            errorMessage != nullptr
                ? errorMessage
                : sqlite3_errmsg(m_handle);

        if (errorMessage != nullptr) {
            sqlite3_free(errorMessage);
        }

        return core::Error{
            core::ErrorCode::DatabaseError,
            "Database",
            std::move(message)
        };
    }

    return core::Result<void>::success();
}

core::Result<std::int64_t>
DatabaseConnection::scalarInt64(const std::string& sql)
{
    if (!isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "Database",
            "Database is not open"
        };
    }

    sqlite3_stmt* statement = nullptr;

    int rc = sqlite3_prepare_v2(
        m_handle,
        sql.c_str(),
        -1,
        &statement,
        nullptr
    );

    if (rc != SQLITE_OK) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "Database",
            sqlite3_errmsg(m_handle)
        };
    }

    rc = sqlite3_step(statement);

    if (rc != SQLITE_ROW) {
        std::string message = sqlite3_errmsg(m_handle);
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::DatabaseError,
            "Database",
            std::move(message)
        };
    }

    const std::int64_t value =
        sqlite3_column_int64(statement, 0);

    sqlite3_finalize(statement);

    return value;
}

core::Result<void> DatabaseConnection::beginTransaction()
{
    return execute("BEGIN IMMEDIATE TRANSACTION;");
}

core::Result<void> DatabaseConnection::commit()
{
    return execute("COMMIT;");
}

core::Result<void> DatabaseConnection::rollback()
{
    if (!isOpen()) {
        return core::Result<void>::success();
    }

    return execute("ROLLBACK;");
}

int DatabaseConnection::userVersion() const noexcept
{
    if (!isOpen()) {
        return 0;
    }

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_handle,
            "PRAGMA user_version;",
            -1,
            &statement,
            nullptr) != SQLITE_OK) {
        return 0;
    }

    int version = 0;

    if (sqlite3_step(statement) == SQLITE_ROW) {
        version = sqlite3_column_int(statement, 0);
    }

    sqlite3_finalize(statement);

    return version;
}

core::Result<void>
DatabaseConnection::setUserVersion(int version)
{
    if (!isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "Database",
            "Database is not open"
        };
    }

    if (version < 0) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "Database",
            "Database version cannot be negative"
        };
    }

    return execute(
        "PRAGMA user_version = " +
        std::to_string(version) +
        ";"
    );
}

} // namespace mnemis::database
