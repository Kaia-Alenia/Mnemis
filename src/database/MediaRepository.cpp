#include "MediaRepository.hpp"

#include <sqlite3.h>

namespace mnemis::database {

namespace {

core::Error databaseError(
    sqlite3* handle,
    const std::string& prefix
)
{
    const char* sqliteMessage =
        handle != nullptr
            ? sqlite3_errmsg(handle)
            : nullptr;

    return core::Error{
        core::ErrorCode::DatabaseError,
        "MediaRepository",
        prefix +
        ": " +
        (
            sqliteMessage != nullptr
                ? std::string(sqliteMessage)
                : std::string("unknown SQLite error")
        )
    };
}

core::Result<std::size_t> executeCountQuery(
    sqlite3* handle,
    const std::string& sql
)
{
    sqlite3_stmt* statement = nullptr;

    const int prepareResult =
        sqlite3_prepare_v2(
            handle,
            sql.c_str(),
            -1,
            &statement,
            nullptr
        );

    if (prepareResult != SQLITE_OK) {
        return databaseError(
            handle,
            "Failed to prepare count query"
        );
    }

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            handle,
            "Failed to execute count query"
        );
    }

    const sqlite3_int64 count =
        sqlite3_column_int64(
            statement,
            0
        );

    sqlite3_finalize(statement);

    return static_cast<std::size_t>(count);
}

} // namespace

MediaRepository::MediaRepository(
    DatabaseConnection& database
)
    : m_database(database)
{
}

core::Result<media::MediaId>
MediaRepository::upsert(
    const media::Media& media
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaRepository",
            "Database is not open"
        };
    }

    if (media.type == media::MediaType::Unknown) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "MediaRepository",
            "Cannot store media with unknown type"
        };
    }

    if (media.canonicalPath.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "MediaRepository",
            "Canonical path cannot be empty"
        };
    }

    constexpr const char* sql =
        "INSERT INTO media ("
        "canonical_path,"
        "file_name,"
        "extension,"
        "media_type,"
        "file_size,"
        "modified_time,"
        "favorite"
        ") VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(canonical_path) DO UPDATE SET "
        "file_name = excluded.file_name, "
        "extension = excluded.extension, "
        "media_type = excluded.media_type, "
        "file_size = excluded.file_size, "
        "modified_time = excluded.modified_time;";

    sqlite3_stmt* statement = nullptr;

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
            "Failed to prepare media upsert"
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        media.canonicalPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        2,
        media.fileName.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        media.extension.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        statement,
        4,
        static_cast<int>(media.type)
    );

    sqlite3_bind_int64(
        statement,
        5,
        static_cast<sqlite3_int64>(media.fileSize)
    );

    sqlite3_bind_int64(
        statement,
        6,
        static_cast<sqlite3_int64>(media.modifiedTime)
    );

    sqlite3_bind_int(
        statement,
        7,
        media.favorite ? 1 : 0
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_DONE) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to insert or update media"
        );
    }

    sqlite3_finalize(statement);

    constexpr const char* findIdSql =
        "SELECT id "
        "FROM media "
        "WHERE canonical_path = ? "
        "LIMIT 1;";

    sqlite3_stmt* findStatement = nullptr;

    const int findPrepareResult =
        sqlite3_prepare_v2(
            m_database.handle(),
            findIdSql,
            -1,
            &findStatement,
            nullptr
        );

    if (findPrepareResult != SQLITE_OK) {
        return databaseError(
            m_database.handle(),
            "Failed to prepare media ID lookup"
        );
    }

    sqlite3_bind_text(
        findStatement,
        1,
        media.canonicalPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int findStepResult =
        sqlite3_step(findStatement);

    if (findStepResult != SQLITE_ROW) {
        sqlite3_finalize(findStatement);

        return databaseError(
            m_database.handle(),
            "Failed to retrieve media ID"
        );
    }

    const auto id =
        static_cast<media::MediaId>(
            sqlite3_column_int64(
                findStatement,
                0
            )
        );

    sqlite3_finalize(findStatement);

    if (!media::isValidMediaId(id)) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "MediaRepository",
            "SQLite returned an invalid media ID"
        };
    }

    return id;
}

core::Result<std::size_t>
MediaRepository::count()
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaRepository",
            "Database is not open"
        };
    }

    return executeCountQuery(
        m_database.handle(),
        "SELECT COUNT(*) FROM media;"
    );
}

core::Result<std::size_t>
MediaRepository::countByType(
    media::MediaType type
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaRepository",
            "Database is not open"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "SELECT COUNT(*) "
        "FROM media "
        "WHERE media_type = ?;";

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
            "Failed to prepare media type count"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        static_cast<int>(type)
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to count media by type"
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

core::Result<std::size_t>
MediaRepository::countByPrefix(
    const std::string& pathPrefix
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaRepository",
            "Database is not open"
        };
    }

    if (pathPrefix.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "MediaRepository",
            "Path prefix cannot be empty"
        };
    }

    sqlite3_stmt* statement = nullptr;

    constexpr const char* sql =
        "SELECT COUNT(*) "
        "FROM media "
        "WHERE canonical_path LIKE ? ESCAPE '\\';";

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
            "Failed to prepare prefix count"
        );
    }

    std::string pattern = pathPrefix;

    if (pattern.back() != '/') {
        pattern += '/';
    }

    pattern += '%';

    sqlite3_bind_text(
        statement,
        1,
        pattern.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to count media by path prefix"
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

core::Result<media::Media>
MediaRepository::findByPath(
    const std::string& canonicalPath
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaRepository",
            "Database is not open"
        };
    }

    if (canonicalPath.empty()) {
        return core::Error{
            core::ErrorCode::InvalidArgument,
            "MediaRepository",
            "Path cannot be empty"
        };
    }

    constexpr const char* sql =
        "SELECT "
        "id,"
        "canonical_path,"
        "file_name,"
        "extension,"
        "media_type,"
        "file_size,"
        "modified_time,"
        "favorite "
        "FROM media "
        "WHERE canonical_path = ? "
        "LIMIT 1;";

    sqlite3_stmt* statement = nullptr;

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
            "Failed to prepare media lookup"
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        canonicalPath.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    const int stepResult =
        sqlite3_step(statement);

    if (stepResult == SQLITE_DONE) {
        sqlite3_finalize(statement);

        return core::Error{
            core::ErrorCode::NotFound,
            "MediaRepository",
            "Media not found: " +
            canonicalPath
        };
    }

    if (stepResult != SQLITE_ROW) {
        sqlite3_finalize(statement);

        return databaseError(
            m_database.handle(),
            "Failed to query media"
        );
    }

    media::Media result;

    result.id =
        static_cast<media::MediaId>(
            sqlite3_column_int64(statement, 0)
        );

    const auto* canonicalPathValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 1)
        );

    const auto* fileNameValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

    const auto* extensionValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

    result.canonicalPath =
        canonicalPathValue != nullptr
            ? canonicalPathValue
            : "";

    result.fileName =
        fileNameValue != nullptr
            ? fileNameValue
            : "";

    result.extension =
        extensionValue != nullptr
            ? extensionValue
            : "";

    result.type =
        static_cast<media::MediaType>(
            sqlite3_column_int(statement, 4)
        );

    result.fileSize =
        static_cast<std::int64_t>(
            sqlite3_column_int64(statement, 5)
        );

    result.modifiedTime =
        static_cast<std::int64_t>(
            sqlite3_column_int64(statement, 6)
        );

    result.favorite =
        sqlite3_column_int(statement, 7) != 0;

    sqlite3_finalize(statement);

    return result;
}

} // namespace mnemis::database
