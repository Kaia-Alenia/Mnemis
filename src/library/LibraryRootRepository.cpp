#include "LibraryRootRepository.hpp"

#include <sqlite3.h>

namespace mnemis::library {

namespace {

core::Error databaseError(
    sqlite3* handle,
    const std::string& message
)
{
    std::string detail = message;

    if (handle != nullptr) {
        const char* sqliteMessage =
            sqlite3_errmsg(handle);

        if (sqliteMessage != nullptr) {
            detail += ": ";
            detail += sqliteMessage;
        }
    }

    return core::Error{
        core::ErrorCode::DatabaseError,
        "LibraryRootRepository",
        std::move(detail)
    };
}

core::Result<LibraryRoot>
readRoot(
    sqlite3_stmt* statement
)
{
    LibraryRoot root;

    root.id =
        static_cast<LibraryRootId>(
            sqlite3_column_int64(
                statement,
                0
            )
        );

    const auto* pathValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                statement,
                1
            )
        );

    const auto* nameValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(
                statement,
                2
            )
        );

    root.path =
        filesystem::Path(
            pathValue != nullptr
                ? pathValue
                : ""
        );

    root.name =
        nameValue != nullptr
            ? nameValue
            : "";

    root.enabled =
        sqlite3_column_int(
            statement,
            3
        ) != 0;

    if (!root.isValid()) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "LibraryRootRepository",
            "Database returned an invalid library root"
        };
    }

    return root;
}

} // namespace

LibraryRootRepository::LibraryRootRepository(
    database::DatabaseConnection& database
)
    : m_database(database)
{
}

core::Result<LibraryRootId>
LibraryRootRepository::add(
    const filesystem::Path& path,
    const std::string& name
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Library root path cannot be empty"
        };
    }

    if (name.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Library root name cannot be empty"
        };
    }

    const auto normalizedPath =
        path.normalized().string();

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "INSERT INTO library_roots "
        "(path, name, enabled) "
        "VALUES (?, ?, 1) "
        "ON CONFLICT(path) DO UPDATE SET "
        "name = excluded.name, "
        "enabled = 1;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root insertion"
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        normalizedPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        2,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to insert library root"
        );
    }

    sqlite3_finalize(statement);

    return findByPath(
        filesystem::Path(normalizedPath)
    ).value().id;
}

core::Result<void>
LibraryRootRepository::remove(
    LibraryRootId id
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    if (id == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Invalid library root ID"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "DELETE FROM library_roots "
        "WHERE id = ?;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root removal"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(id)
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to remove library root"
        );
    }

    sqlite3_finalize(statement);

    return core::Result<void>::success();
}

core::Result<void>
LibraryRootRepository::setEnabled(
    LibraryRootId id,
    bool enabled
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    if (id == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Invalid library root ID"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "UPDATE library_roots "
        "SET enabled = ? "
        "WHERE id = ?;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root update"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        enabled ? 1 : 0
    );

    sqlite3_bind_int64(
        statement,
        2,
        static_cast<sqlite3_int64>(id)
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to update library root"
        );
    }

    sqlite3_finalize(statement);

    return core::Result<void>::success();
}

core::Result<LibraryRoot>
LibraryRootRepository::findById(
    LibraryRootId id
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    if (id == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Invalid library root ID"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "SELECT "
        "id, "
        "path, "
        "name, "
        "enabled "
        "FROM library_roots "
        "WHERE id = ? "
        "LIMIT 1;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root lookup"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(id)
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::NotFound,
            "LibraryRootRepository",
            "Library root not found"
        };
    }

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to query library root"
        );
    }

    auto result =
        readRoot(statement);

    sqlite3_finalize(statement);

    return result;
}

core::Result<LibraryRoot>
LibraryRootRepository::findByPath(
    const filesystem::Path& path
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    if (path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "LibraryRootRepository",
            "Library root path cannot be empty"
        };
    }

    const auto normalizedPath =
        path.normalized().string();

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "SELECT "
        "id, "
        "path, "
        "name, "
        "enabled "
        "FROM library_roots "
        "WHERE path = ? "
        "LIMIT 1;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root path lookup"
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        normalizedPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::NotFound,
            "LibraryRootRepository",
            "Library root not found"
        };
    }

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to query library root by path"
        );
    }

    auto result =
        readRoot(statement);

    sqlite3_finalize(statement);

    return result;
}

core::Result<std::vector<LibraryRoot>>
LibraryRootRepository::list(
    bool includeDisabled
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* allSql =
        "SELECT "
        "id, "
        "path, "
        "name, "
        "enabled "
        "FROM library_roots "
        "ORDER BY name COLLATE NOCASE ASC;";

    constexpr const char* enabledSql =
        "SELECT "
        "id, "
        "path, "
        "name, "
        "enabled "
        "FROM library_roots "
        "WHERE enabled = 1 "
        "ORDER BY name COLLATE NOCASE ASC;";

    const char* sql =
        includeDisabled
            ? allSql
            : enabledSql;

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare library root list"
        );
    }

    std::vector<LibraryRoot> roots;

    while (true) {
        const int stepResult =
            sqlite3_step(statement);

        if (stepResult == SQLITE_DONE) {
            break;
        }

        if (stepResult != SQLITE_ROW) {
            sqlite3_finalize(statement);

            return databaseError(
                m_database.handle(),
                "Failed to iterate library roots"
            );
        }

        auto rootResult =
            readRoot(statement);

        if (rootResult.isError()) {
            sqlite3_finalize(statement);
            return rootResult.error();
        }

        roots.push_back(
            rootResult.value()
        );
    }

    sqlite3_finalize(statement);

    return roots;
}

core::Result<std::size_t>
LibraryRootRepository::count()
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "LibraryRootRepository",
            "Database is not open"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "SELECT COUNT(*) "
        "FROM library_roots;";

    const int prepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare root count"
        );
    }

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to count library roots"
        );
    }

    const auto count =
        static_cast<std::size_t>(
            sqlite3_column_int64(
                statement,
                0
            )
        );

    sqlite3_finalize(statement);

    return count;
}

} // namespace mnemis::library
