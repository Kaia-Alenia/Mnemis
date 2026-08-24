#include "MediaQuery.hpp"

#include <sqlite3.h>

#include <sstream>

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
        "MediaQuery",
        std::move(detail)
    };
}

core::Result<media::Media> readMedia(
    sqlite3_stmt* statement
)
{
    media::Media item;

    item.id =
        static_cast<media::MediaId>(
            sqlite3_column_int64(statement, 0)
        );

    const auto* pathValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 1)
        );

    const auto* nameValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 2)
        );

    const auto* extensionValue =
        reinterpret_cast<const char*>(
            sqlite3_column_text(statement, 3)
        );

    item.canonicalPath =
        pathValue != nullptr
            ? pathValue
            : "";

    item.fileName =
        nameValue != nullptr
            ? nameValue
            : "";

    item.extension =
        extensionValue != nullptr
            ? extensionValue
            : "";

    item.type =
        static_cast<media::MediaType>(
            sqlite3_column_int(statement, 4)
        );

    item.fileSize =
        static_cast<std::int64_t>(
            sqlite3_column_int64(statement, 5)
        );

    item.modifiedTime =
        static_cast<std::int64_t>(
            sqlite3_column_int64(statement, 6)
        );

    item.favorite =
        sqlite3_column_int(statement, 7) != 0;

    if (!item.isValid()) {
        return core::Error{
            core::ErrorCode::DatabaseError,
            "MediaQuery",
            "Query returned invalid media"
        };
    }

    return item;
}

} // namespace

MediaQuery::MediaQuery(
    database::DatabaseConnection& database
)
    : m_database(database)
{
}

const char* MediaQuery::sortColumn(
    MediaSortField field
) noexcept
{
    switch (field) {
        case MediaSortField::Name:
            return "file_name";

        case MediaSortField::Path:
            return "canonical_path";

        case MediaSortField::Size:
            return "file_size";

        case MediaSortField::ModifiedTime:
            return "modified_time";
    }

    return "file_name";
}

std::string MediaQuery::escapeLike(
    const std::string& value
)
{
    std::string escaped;

    escaped.reserve(value.size());

    for (const char character : value) {
        switch (character) {
            case '\\':
            case '%':
            case '_':
                escaped.push_back('\\');
                escaped.push_back(character);
                break;

            default:
                escaped.push_back(character);
                break;
        }
    }

    return escaped;
}

core::Result<std::vector<media::Media>>
MediaQuery::execute(
    const MediaQueryOptions& options
)
{
    return executeInternal(
        options,
        false
    );
}

core::Result<std::size_t>
MediaQuery::count(
    const MediaQueryOptions& options
)
{
    auto result =
        executeInternal(
            options,
            true
        );

    if (result.isError()) {
        return result.error();
    }

    return result.value().size();
}

core::Result<std::vector<media::Media>>
MediaQuery::listInRoot(
    const filesystem::Path& rootPath,
    bool recursive
)
{
    MediaQueryOptions options;

    options.scopePath = rootPath;
    options.recursive = recursive;

    return execute(options);
}

core::Result<std::vector<media::Media>>
MediaQuery::listInFolder(
    const filesystem::Path& folderPath,
    bool recursive
)
{
    MediaQueryOptions options;

    options.scopePath = folderPath;
    options.recursive = recursive;

    return execute(options);
}

core::Result<std::vector<media::Media>>
MediaQuery::executeInternal(
    const MediaQueryOptions& options,
    bool countOnly
)
{
    if (!m_database.isOpen()) {
        return core::Error{
            core::ErrorCode::InvalidState,
            "MediaQuery",
            "Database is not open"
        };
    }

    std::string sql;

    if (countOnly) {
        sql =
            "SELECT COUNT(*) "
            "FROM media "
            "WHERE 1 = 1 ";
    } else {
        sql =
            "SELECT "
            "id, "
            "canonical_path, "
            "file_name, "
            "extension, "
            "media_type, "
            "file_size, "
            "modified_time, "
            "favorite "
            "FROM media "
            "WHERE 1 = 1 ";
    }

    std::vector<std::string> parameters;

    if (options.scopePath.has_value()) {
        const std::string scope =
            options.scopePath
                ->normalized()
                .string();

        if (scope.empty()) {
            return core::Error{
                core::ErrorCode::InvalidArgument,
                "MediaQuery",
                "Scope path cannot be empty"
            };
        }

        if (options.recursive) {
            sql +=
                "AND ("
                "canonical_path = ? "
                "OR canonical_path LIKE ? ESCAPE '\\'"
                ") ";

            parameters.push_back(scope);

            parameters.push_back(
                escapeLike(scope + "/") + "%"
            );
        } else {
            sql +=
                "AND canonical_path LIKE ? ESCAPE '\\' "
                "AND instr("
                "substr("
                "canonical_path, "
                "length(?) + 2"
                "), "
                "'/'"
                ") = 0 ";

            parameters.push_back(
                escapeLike(scope + "/") + "%"
            );

            parameters.push_back(scope);
        }
    }

    if (!options.searchText.empty()) {
        sql +=
            "AND LOWER(file_name) LIKE LOWER(?) "
            "ESCAPE '\\' ";

        parameters.push_back(
            "%" +
            escapeLike(options.searchText) +
            "%"
        );
    }

    if (options.mediaType.has_value()) {
        sql +=
            "AND media_type = ? ";

        parameters.push_back(
            std::to_string(
                static_cast<int>(
                    options.mediaType.value()
                )
            )
        );
    }

    if (countOnly) {
        sqlite3_stmt* statement = nullptr;

        if (sqlite3_prepare_v2(
                m_database.handle(),
                sql.c_str(),
                -1,
                &statement,
                nullptr) != SQLITE_OK) {

            return databaseError(
                m_database.handle(),
                "Failed to prepare media count query"
            );
        }

        int parameterIndex = 1;

        for (const auto& parameter : parameters) {
            sqlite3_bind_text(
                statement,
                parameterIndex++,
                parameter.c_str(),
                -1,
                SQLITE_TRANSIENT
            );
        }

        const int result =
            sqlite3_step(statement);

        if (result != SQLITE_ROW) {
            sqlite3_finalize(statement);

            return databaseError(
                m_database.handle(),
                "Failed to execute media count query"
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

        std::vector<media::Media> placeholder;

        for (std::size_t index = 0;
             index < count;
             ++index) {
            placeholder.emplace_back();
        }

        return placeholder;
    }

    sql +=
        "ORDER BY " +
        std::string(
            sortColumn(options.sortField)
        ) +
        (
            options.direction ==
                SortDirection::Ascending
                ? " ASC "
                : " DESC "
        );

    if (options.limit > 0) {
        sql += "LIMIT ? ";
    }

    if (options.offset > 0) {
        sql += "OFFSET ? ";
    }

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            m_database.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr) != SQLITE_OK) {

        return databaseError(
            m_database.handle(),
            "Failed to prepare media query"
        );
    }

    int parameterIndex = 1;

    for (const auto& parameter : parameters) {
        sqlite3_bind_text(
            statement,
            parameterIndex++,
            parameter.c_str(),
            -1,
            SQLITE_TRANSIENT
        );
    }

    if (options.limit > 0) {
        sqlite3_bind_int64(
            statement,
            parameterIndex++,
            static_cast<sqlite3_int64>(
                options.limit
            )
        );
    }

    if (options.offset > 0) {
        sqlite3_bind_int64(
            statement,
            parameterIndex++,
            static_cast<sqlite3_int64>(
                options.offset
            )
        );
    }

    std::vector<media::Media> results;

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
                "Failed to execute media query"
            );
        }

        auto mediaResult =
            readMedia(statement);

        if (mediaResult.isError()) {
            sqlite3_finalize(statement);
            return mediaResult.error();
        }

        results.push_back(
            mediaResult.value()
        );
    }

    sqlite3_finalize(statement);

    return results;
}

} // namespace mnemis::library
