#include "FolderRepository.hpp"

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
        "FolderRepository",
        std::move(detail)
    };
}

core::Result<Folder> readFolder(
    sqlite3_stmt* statement
)
{
    Folder folder;

    folder.id =
        static_cast<FolderId>(
            sqlite3_column_int64(statement, 0)
        );

    folder.rootId =
        static_cast<LibraryRootId>(
            sqlite3_column_int64(statement, 1)
        );

    folder.parentId =
        static_cast<FolderId>(
            sqlite3_column_int64(statement, 2)
        );

    const auto* pathValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

    const auto* nameValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 4)
        );

    folder.path =
        filesystem::Path(
            pathValue != nullptr
                ? pathValue
                : ""
        );

    folder.name =
        nameValue != nullptr
            ? nameValue
            : "";

    folder.isRootFolder =
        sqlite3_column_int(statement, 5) != 0;

    if (!folder.isValid()) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "FolderRepository",
            "Database returned an invalid folder"
        };
    }

    return folder;
}

} // namespace

FolderRepository::FolderRepository(
    database::DatabaseConnection& database
)
    : m_database(database)
{
}

core::Result<FolderId>
FolderRepository::upsert(
    LibraryRootId rootId,
    FolderId parentId,
    const filesystem::Path& path,
    const std::string& name,
    bool isRootFolder
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    if (rootId == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "FolderRepository",
            "Invalid library root ID"
        };
    }

    if (path.empty() || name.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "FolderRepository",
            "Folder path and name are required"
        };
    }

    const std::string normalizedPath =
        path.normalized().string();

    constexpr const char* sql =
        "INSERT INTO folders "
        "(library_root_id, parent_id, path, name, is_root) "
        "VALUES (?, ?, ?, ?, ?) "
        "ON CONFLICT(library_root_id, path) DO UPDATE SET "
        "parent_id = excluded.parent_id, "
        "name = excluded.name, "
        "is_root = excluded.is_root;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder upsert"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    if (parentId == InvalidFolderId) {
        sqlite3_bind_null(statement, 2);
    } else {
        sqlite3_bind_int64(
            statement,
            2,
            static_cast<sqlite3_int64>(parentId)
        );
    }

    sqlite3_bind_text(
        statement,
        3,
        normalizedPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        4,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        statement,
        5,
        isRootFolder ? 1 : 0
    );

    if (sqlite3_step(statement) != SQLITE_DONE) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to upsert folder"
        );
    }

    sqlite3_finalize(statement);

    constexpr const char* findSql =
        "SELECT id "
        "FROM folders "
        "WHERE library_root_id = ? "
        "AND path = ? "
        "LIMIT 1;";

    sqlite3_stmt* findStatement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            findSql,
            -1,
            &findStatement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder ID lookup"
        );
    }

    sqlite3_bind_int64(
        findStatement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    sqlite3_bind_text(
        findStatement,
        2,
        normalizedPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (sqlite3_step(findStatement) != SQLITE_ROW) {
        sqlite3_finalize(findStatement);

        return databaseError(
            m_database.handle(),
            "Failed to retrieve folder ID"
        );
    }

    const FolderId id =
        static_cast<FolderId>(
            sqlite3_column_int64(
                findStatement,
                0
            )
        );

    sqlite3_finalize(findStatement);

    return id;
}

core::Result<Folder>
FolderRepository::findById(
    FolderId id
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    if (id == InvalidFolderId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "FolderRepository",
            "Invalid folder ID"
        };
    }

    constexpr const char* sql =
        "SELECT "
        "id, "
        "library_root_id, "
        "COALESCE(parent_id, 0), "
        "path, "
        "name, "
        "is_root "
        "FROM folders "
        "WHERE id = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder lookup"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(id)
    );

    const int result =
        sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::NotFound,
            "FolderRepository",
            "Folder not found"
        };
    }

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to query folder"
        );
    }

    auto folder =
        readFolder(statement);

    sqlite3_finalize(statement);

    return folder;
}

core::Result<Folder>
FolderRepository::findByPath(
    LibraryRootId rootId,
    const filesystem::Path& path
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    if (rootId == InvalidLibraryRootId ||
        path.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "FolderRepository",
            "Root ID and path are required"
        };
    }

    const std::string normalizedPath =
        path.normalized().string();

    constexpr const char* sql =
        "SELECT "
        "id, "
        "library_root_id, "
        "COALESCE(parent_id, 0), "
        "path, "
        "name, "
        "is_root "
        "FROM folders "
        "WHERE library_root_id = ? "
        "AND path = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder path lookup"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    sqlite3_bind_text(
        statement,
        2,
        normalizedPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int result =
        sqlite3_step(statement);

    if (result == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::NotFound,
            "FolderRepository",
            "Folder not found by path"
        };
    }

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to query folder by path"
        );
    }

    auto folder =
        readFolder(statement);

    sqlite3_finalize(statement);

    return folder;
}

core::Result<std::vector<Folder>>
FolderRepository::children(
    LibraryRootId rootId,
    FolderId parentId
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    if (rootId == InvalidLibraryRootId) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "FolderRepository",
            "Invalid root ID"
        };
    }

    constexpr const char* sql =
        "SELECT "
        "id, "
        "library_root_id, "
        "COALESCE(parent_id, 0), "
        "path, "
        "name, "
        "is_root "
        "FROM folders "
        "WHERE library_root_id = ? "
        "AND parent_id = ? "
        "ORDER BY name COLLATE NOCASE ASC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder children query"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    sqlite3_bind_int64(
        statement,
        2,
        static_cast<sqlite3_int64>(parentId)
    );

    std::vector<Folder> folders;

    while (true) {
        const int result =
            sqlite3_step(statement);

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            sqlite3_finalize(statement);

            return databaseError(
                m_database.handle(),
                "Failed to iterate folder children"
            );
        }

        auto folder =
            readFolder(statement);

        if (folder.isError()) {
            sqlite3_finalize(statement);
            return folder.error();
        }

        folders.push_back(
            folder.value()
        );
    }

    sqlite3_finalize(statement);

    return folders;
}

core::Result<std::vector<Folder>>
FolderRepository::listForRoot(
    LibraryRootId rootId
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    constexpr const char* sql =
        "SELECT "
        "id, "
        "library_root_id, "
        "COALESCE(parent_id, 0), "
        "path, "
        "name, "
        "is_root "
        "FROM folders "
        "WHERE library_root_id = ? "
        "ORDER BY path COLLATE NOCASE ASC;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare root folder list"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    std::vector<Folder> folders;

    while (true) {
        const int result =
            sqlite3_step(statement);

        if (result == SQLITE_DONE) {
            break;
        }

        if (result != SQLITE_ROW) {
            sqlite3_finalize(statement);

            return databaseError(
                m_database.handle(),
                "Failed to iterate root folders"
            );
        }

        auto folder =
            readFolder(statement);

        if (folder.isError()) {
            sqlite3_finalize(statement);
            return folder.error();
        }

        folders.push_back(
            folder.value()
        );
    }

    sqlite3_finalize(statement);

    return folders;
}

core::Result<std::size_t>
FolderRepository::countForRoot(
    LibraryRootId rootId
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "FolderRepository",
            "Database is not open"
        };
    }

    constexpr const char* sql =
        "SELECT COUNT(*) "
        "FROM folders "
        "WHERE library_root_id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql,
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare folder count"
        );
    }

    sqlite3_bind_int64(
        statement,
        1,
        static_cast<sqlite3_int64>(rootId)
    );

    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to count folders"
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
