#include "SQLiteMediaRepository.hpp"
#include "core/events/ILibraryEventBus.hpp"

#include <sqlite3.h>
#include <cstring>

namespace mnemis::database::repositories {

using namespace core::models;

SQLiteMediaRepository::SQLiteMediaRepository(DatabaseConnection& conn) : m_conn(conn), m_eventBus(nullptr) {}

void SQLiteMediaRepository::setEventBus(std::shared_ptr<core::events::ILibraryEventBus> eventBus) {
    m_eventBus = std::move(eventBus);
}

static void bindStringOptional(sqlite3_stmt* stmt, int index, const std::optional<std::string>& val) {
    if (val.has_value()) {
        sqlite3_bind_text(stmt, index, val->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

static void bindIntOptional(sqlite3_stmt* stmt, int index, const std::optional<uint32_t>& val) {
    if (val.has_value()) {
        sqlite3_bind_int(stmt, index, static_cast<int>(val.value()));
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

static void bindInt64Optional(sqlite3_stmt* stmt, int index, const std::optional<int64_t>& val) {
    if (val.has_value()) {
        sqlite3_bind_int64(stmt, index, val.value());
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

static void bindDoubleOptional(sqlite3_stmt* stmt, int index, const std::optional<double>& val) {
    if (val.has_value()) {
        sqlite3_bind_double(stmt, index, val.value());
    } else {
        sqlite3_bind_null(stmt, index);
    }
}

static std::optional<std::string> getColumnStringOptional(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, col));
    return text ? std::optional<std::string>(text) : std::nullopt;
}

static std::optional<uint32_t> getColumnUInt32Optional(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return static_cast<uint32_t>(sqlite3_column_int(stmt, col));
}

static std::optional<int64_t> getColumnInt64Optional(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return sqlite3_column_int64(stmt, col);
}

static std::optional<double> getColumnDoubleOptional(sqlite3_stmt* stmt, int col) {
    if (sqlite3_column_type(stmt, col) == SQLITE_NULL) {
        return std::nullopt;
    }
    return sqlite3_column_double(stmt, col);
}

core::Result<void> SQLiteMediaRepository::add(const MediaItem& item) {
    if (!m_conn.isOpen()) return core::Result<void>(core::Error{1, "Database not open"});

    const char* sql = R"(
        INSERT INTO media (
            media_id, path, canonical_path, file_name, extension, mime_type, media_type,
            file_size, modified_time, created_time, width, height, duration, frame_rate,
            audio_channels, audio_sample_rate, title, artist, album, album_artist, genre,
            track_number, disc_number, year, has_thumbnail, thumbnail_version, favorite,
            last_played, play_count, index_state, error_state
        ) VALUES (
            ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?
        )
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    sqlite3_bind_text(stmt, 1, item.mediaId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, item.path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.canonicalPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, item.fileName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, item.extension.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, item.mimeType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, static_cast<int>(item.mediaType));
    sqlite3_bind_int64(stmt, 8, item.fileSize);
    sqlite3_bind_int64(stmt, 9, item.modifiedTime);
    sqlite3_bind_int64(stmt, 10, item.createdTime);

    bindIntOptional(stmt, 11, item.width);
    bindIntOptional(stmt, 12, item.height);
    bindDoubleOptional(stmt, 13, item.duration);
    bindDoubleOptional(stmt, 14, item.frameRate);
    bindIntOptional(stmt, 15, item.audioChannels);
    bindIntOptional(stmt, 16, item.audioSampleRate);
    
    bindStringOptional(stmt, 17, item.title);
    bindStringOptional(stmt, 18, item.artist);
    bindStringOptional(stmt, 19, item.album);
    bindStringOptional(stmt, 20, item.albumArtist);
    bindStringOptional(stmt, 21, item.genre);
    
    bindIntOptional(stmt, 22, item.trackNumber);
    bindIntOptional(stmt, 23, item.discNumber);
    bindIntOptional(stmt, 24, item.year);

    sqlite3_bind_int(stmt, 25, item.hasThumbnail ? 1 : 0);
    sqlite3_bind_int(stmt, 26, item.thumbnailVersion);
    sqlite3_bind_int(stmt, 27, item.favorite ? 1 : 0);
    bindInt64Optional(stmt, 28, item.lastPlayed);
    sqlite3_bind_int(stmt, 29, item.playCount);
    sqlite3_bind_int(stmt, 30, static_cast<int>(item.indexState));
    bindStringOptional(stmt, 31, item.errorState);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});
    }

    if (m_eventBus) {
        core::events::LibraryEvent event;
        event.type = core::events::LibraryEvent::Type::Added;
        event.mediaId = item.mediaId;
        m_eventBus->publish(event);
    }

    return core::Result<void>();
}

core::Result<void> SQLiteMediaRepository::update(const MediaItem& item) {
    if (!m_conn.isOpen()) return core::Result<void>(core::Error{1, "Database not open"});

    const char* sql = R"(
        UPDATE media SET
            path = ?, canonical_path = ?, file_name = ?, extension = ?, mime_type = ?, media_type = ?,
            file_size = ?, modified_time = ?, created_time = ?, width = ?, height = ?, duration = ?, frame_rate = ?,
            audio_channels = ?, audio_sample_rate = ?, title = ?, artist = ?, album = ?, album_artist = ?, genre = ?,
            track_number = ?, disc_number = ?, year = ?, has_thumbnail = ?, thumbnail_version = ?, favorite = ?,
            last_played = ?, play_count = ?, index_state = ?, error_state = ?
        WHERE media_id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    sqlite3_bind_text(stmt, 1, item.path.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, item.canonicalPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, item.fileName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, item.extension.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, item.mimeType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 6, static_cast<int>(item.mediaType));
    sqlite3_bind_int64(stmt, 7, item.fileSize);
    sqlite3_bind_int64(stmt, 8, item.modifiedTime);
    sqlite3_bind_int64(stmt, 9, item.createdTime);

    bindIntOptional(stmt, 10, item.width);
    bindIntOptional(stmt, 11, item.height);
    bindDoubleOptional(stmt, 12, item.duration);
    bindDoubleOptional(stmt, 13, item.frameRate);
    bindIntOptional(stmt, 14, item.audioChannels);
    bindIntOptional(stmt, 15, item.audioSampleRate);
    
    bindStringOptional(stmt, 16, item.title);
    bindStringOptional(stmt, 17, item.artist);
    bindStringOptional(stmt, 18, item.album);
    bindStringOptional(stmt, 19, item.albumArtist);
    bindStringOptional(stmt, 20, item.genre);
    
    bindIntOptional(stmt, 21, item.trackNumber);
    bindIntOptional(stmt, 22, item.discNumber);
    bindIntOptional(stmt, 23, item.year);

    sqlite3_bind_int(stmt, 24, item.hasThumbnail ? 1 : 0);
    sqlite3_bind_int(stmt, 25, item.thumbnailVersion);
    sqlite3_bind_int(stmt, 26, item.favorite ? 1 : 0);
    bindInt64Optional(stmt, 27, item.lastPlayed);
    sqlite3_bind_int(stmt, 28, item.playCount);
    sqlite3_bind_int(stmt, 29, static_cast<int>(item.indexState));
    bindStringOptional(stmt, 30, item.errorState);

    sqlite3_bind_text(stmt, 31, item.mediaId.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});
    }

    if (m_eventBus) {
        core::events::LibraryEvent event;
        event.type = core::events::LibraryEvent::Type::Updated;
        event.mediaId = item.mediaId;
        m_eventBus->publish(event);
    }

    return core::Result<void>();
}

core::Result<void> SQLiteMediaRepository::remove(const std::string& mediaId) {
    if (!m_conn.isOpen()) return core::Result<void>(core::Error{1, "Database not open"});

    const char* sql = "DELETE FROM media WHERE media_id = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    sqlite3_bind_text(stmt, 1, mediaId.c_str(), -1, SQLITE_TRANSIENT);

    rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});
    }

    if (m_eventBus) {
        core::events::LibraryEvent event;
        event.type = core::events::LibraryEvent::Type::Removed;
        event.mediaId = mediaId;
        m_eventBus->publish(event);
    }

    return core::Result<void>();
}

static MediaItem populateItemFromRow(sqlite3_stmt* stmt) {
    MediaItem item;
    item.mediaId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    item.path = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    item.canonicalPath = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    item.fileName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
    item.extension = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
    item.mimeType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
    item.mediaType = static_cast<MediaType>(sqlite3_column_int(stmt, 6));
    item.fileSize = sqlite3_column_int64(stmt, 7);
    item.modifiedTime = sqlite3_column_int64(stmt, 8);
    item.createdTime = sqlite3_column_int64(stmt, 9);
    
    item.width = getColumnUInt32Optional(stmt, 10);
    item.height = getColumnUInt32Optional(stmt, 11);
    item.duration = getColumnDoubleOptional(stmt, 12);
    item.frameRate = getColumnDoubleOptional(stmt, 13);
    item.audioChannels = getColumnUInt32Optional(stmt, 14);
    item.audioSampleRate = getColumnUInt32Optional(stmt, 15);
    
    item.title = getColumnStringOptional(stmt, 16);
    item.artist = getColumnStringOptional(stmt, 17);
    item.album = getColumnStringOptional(stmt, 18);
    item.albumArtist = getColumnStringOptional(stmt, 19);
    item.genre = getColumnStringOptional(stmt, 20);
    
    item.trackNumber = getColumnUInt32Optional(stmt, 21);
    item.discNumber = getColumnUInt32Optional(stmt, 22);
    item.year = getColumnUInt32Optional(stmt, 23);
    
    item.hasThumbnail = sqlite3_column_int(stmt, 24) != 0;
    item.thumbnailVersion = sqlite3_column_int(stmt, 25);
    item.favorite = sqlite3_column_int(stmt, 26) != 0;
    item.lastPlayed = getColumnInt64Optional(stmt, 27);
    item.playCount = sqlite3_column_int(stmt, 28);
    item.indexState = static_cast<IndexState>(sqlite3_column_int(stmt, 29));
    item.errorState = getColumnStringOptional(stmt, 30);

    return item;
}

core::Result<std::optional<MediaItem>> SQLiteMediaRepository::getById(const std::string& mediaId) {
    if (!m_conn.isOpen()) return core::Result<std::optional<MediaItem>>(core::Error{1, "Database not open"});

    const char* sql = R"(
        SELECT media_id, path, canonical_path, file_name, extension, mime_type, media_type,
               file_size, modified_time, created_time, width, height, duration, frame_rate,
               audio_channels, audio_sample_rate, title, artist, album, album_artist, genre,
               track_number, disc_number, year, has_thumbnail, thumbnail_version, favorite,
               last_played, play_count, index_state, error_state
        FROM media WHERE media_id = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<std::optional<MediaItem>>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    sqlite3_bind_text(stmt, 1, mediaId.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<MediaItem> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = populateItemFromRow(stmt);
    }
    
    sqlite3_finalize(stmt);
    return core::Result<std::optional<MediaItem>>(std::move(result));
}

core::Result<std::vector<MediaItem>> SQLiteMediaRepository::list(int page, int pageSize, const core::repositories::QueryOptions& options) {
    if (!m_conn.isOpen()) return core::Result<std::vector<MediaItem>>(core::Error{1, "Database not open"});

    std::string sql = R"(
        SELECT media_id, path, canonical_path, file_name, extension, mime_type, media_type,
               file_size, modified_time, created_time, width, height, duration, frame_rate,
               audio_channels, audio_sample_rate, title, artist, album, album_artist, genre,
               track_number, disc_number, year, has_thumbnail, thumbnail_version, favorite,
               last_played, play_count, index_state, error_state
        FROM media
    )";

    std::vector<std::string> conditions;
    if (options.filterText.has_value() && !options.filterText->empty()) {
        conditions.push_back("(file_name LIKE ? OR title LIKE ? OR artist LIKE ? OR album LIKE ?)");
    }
    if (options.filterMediaType.has_value()) {
        conditions.push_back("media_type = ?");
    }
    if (options.filterFavorite.has_value() && options.filterFavorite.value()) {
        conditions.push_back("favorite = 1");
    }

    if (!conditions.empty()) {
        sql += " WHERE " + conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            sql += " AND " + conditions[i];
        }
    }

    // Sort column validation to prevent SQL injection
    std::string sortCol = "created_time"; // Default
    if (options.sortBy == "file_name" || options.sortBy == "title" || 
        options.sortBy == "created_time" || options.sortBy == "modified_time" || 
        options.sortBy == "file_size" || options.sortBy == "duration") {
        sortCol = options.sortBy;
    }

    sql += " ORDER BY " + sortCol + (options.ascending ? " ASC" : " DESC");
    sql += " LIMIT ? OFFSET ?";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<std::vector<MediaItem>>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    int bindIdx = 1;
    std::string likeText;
    if (options.filterText.has_value() && !options.filterText->empty()) {
        likeText = "%" + options.filterText.value() + "%";
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (options.filterMediaType.has_value()) {
        sqlite3_bind_int(stmt, bindIdx++, static_cast<int>(options.filterMediaType.value()));
    }

    int limit = pageSize > 0 ? pageSize : 100;
    int offset = page > 0 ? (page - 1) * limit : 0;

    sqlite3_bind_int(stmt, bindIdx++, limit);
    sqlite3_bind_int(stmt, bindIdx++, offset);

    std::vector<MediaItem> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.push_back(populateItemFromRow(stmt));
    }
    
    sqlite3_finalize(stmt);
    return core::Result<std::vector<MediaItem>>(std::move(results));
}

core::Result<int> SQLiteMediaRepository::count(const core::repositories::QueryOptions& options) {
    if (!m_conn.isOpen()) return core::Result<int>(core::Error{1, "Database not open"});

    std::string sql = "SELECT COUNT(*) FROM media";

    std::vector<std::string> conditions;
    if (options.filterText.has_value() && !options.filterText->empty()) {
        conditions.push_back("(file_name LIKE ? OR title LIKE ? OR artist LIKE ? OR album LIKE ?)");
    }
    if (options.filterMediaType.has_value()) {
        conditions.push_back("media_type = ?");
    }
    if (options.filterFavorite.has_value() && options.filterFavorite.value()) {
        conditions.push_back("favorite = 1");
    }

    if (!conditions.empty()) {
        sql += " WHERE " + conditions[0];
        for (size_t i = 1; i < conditions.size(); ++i) {
            sql += " AND " + conditions[i];
        }
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<int>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    int bindIdx = 1;
    std::string likeText;
    if (options.filterText.has_value() && !options.filterText->empty()) {
        likeText = "%" + options.filterText.value() + "%";
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, likeText.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (options.filterMediaType.has_value()) {
        sqlite3_bind_int(stmt, bindIdx++, static_cast<int>(options.filterMediaType.value()));
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    
    sqlite3_finalize(stmt);
    return core::Result<int>(count);
}

core::Result<std::optional<MediaItem>> SQLiteMediaRepository::getByCanonicalPath(const std::string& canonicalPath) {
    if (!m_conn.isOpen()) return core::Result<std::optional<MediaItem>>(core::Error{1, "Database not open"});

    const char* sql = R"(
        SELECT media_id, path, canonical_path, file_name, extension, mime_type, media_type,
               file_size, modified_time, created_time, width, height, duration, frame_rate,
               audio_channels, audio_sample_rate, title, artist, album, album_artist, genre,
               track_number, disc_number, year, has_thumbnail, thumbnail_version, favorite,
               last_played, play_count, index_state, error_state
        FROM media WHERE canonical_path = ?
    )";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return core::Result<std::optional<MediaItem>>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});

    sqlite3_bind_text(stmt, 1, canonicalPath.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<MediaItem> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = populateItemFromRow(stmt);
    }
    
    sqlite3_finalize(stmt);
    return core::Result<std::optional<MediaItem>>(std::move(result));
}

core::Result<void> SQLiteMediaRepository::saveBatch(const std::vector<MediaItem>& items) {
    if (!m_conn.isOpen()) return core::Result<void>(core::Error{1, "Database not open"});

    // Start transaction
    char* errMsg = nullptr;
    if (sqlite3_exec(m_conn.getHandle(), "BEGIN TRANSACTION", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr = errMsg ? errMsg : "Unknown error";
        if (errMsg) sqlite3_free(errMsg);
        return core::Result<void>(core::Error{2, "Failed to begin transaction: " + errStr});
    }

    core::Result<void> batchResult;

    for (const auto& item : items) {
        auto existingRes = getByCanonicalPath(item.canonicalPath);
        if (!existingRes.isSuccess()) {
            batchResult = core::Result<void>(existingRes.error());
            break;
        }

        core::Result<void> opResult;
        if (existingRes.value().has_value()) {
            opResult = update(item);
        } else {
            opResult = add(item);
        }

        if (!opResult.isSuccess()) {
            batchResult = opResult;
            break;
        }
    }

    if (!batchResult.isSuccess()) {
        sqlite3_exec(m_conn.getHandle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return batchResult;
    }

    if (sqlite3_exec(m_conn.getHandle(), "COMMIT", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr = errMsg ? errMsg : "Unknown error";
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_exec(m_conn.getHandle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return core::Result<void>(core::Error{3, "Failed to commit transaction: " + errStr});
    }

    if (m_eventBus) {
        // Because saveBatch delegates to add/update, events are already emitted by add/update.
        // For absolute strictness we should suppress them until commit, but since SQLiteDatabaseService 
        // doesn't support concurrent multi-threaded writes on the same connection easily, 
        // emitting them inside add/update during the batch is acceptable for now.
        // We can emit a BatchAdded to notify of a mass change.
        std::vector<std::string> ids;
        for (const auto& item : items) {
            ids.push_back(item.mediaId);
        }
        core::events::LibraryEvent event;
        event.type = core::events::LibraryEvent::Type::BatchAdded;
        event.mediaIds = std::move(ids);
        m_eventBus->publish(event);
    }

    return core::Result<void>();
}

core::Result<void> SQLiteMediaRepository::removeBatch(const std::vector<std::string>& mediaIds) {
    if (!m_conn.isOpen()) return core::Result<void>(core::Error{1, "Database not open"});
    
    if (mediaIds.empty()) return core::Result<void>();

    // Start transaction
    char* errMsg = nullptr;
    if (sqlite3_exec(m_conn.getHandle(), "BEGIN TRANSACTION", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr = errMsg ? errMsg : "Unknown error";
        if (errMsg) sqlite3_free(errMsg);
        return core::Result<void>(core::Error{2, "Failed to begin transaction: " + errStr});
    }

    core::Result<void> batchResult;
    const char* sql = "DELETE FROM media WHERE media_id = ?";
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        batchResult = core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});
    } else {
        for (const auto& id : mediaIds) {
            sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
            
            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE) {
                batchResult = core::Result<void>(core::Error{rc, sqlite3_errmsg(m_conn.getHandle())});
                break;
            }
            sqlite3_reset(stmt);
        }
        sqlite3_finalize(stmt);
    }

    if (!batchResult.isSuccess()) {
        sqlite3_exec(m_conn.getHandle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return batchResult;
    }

    if (sqlite3_exec(m_conn.getHandle(), "COMMIT", nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string errStr = errMsg ? errMsg : "Unknown error";
        if (errMsg) sqlite3_free(errMsg);
        sqlite3_exec(m_conn.getHandle(), "ROLLBACK", nullptr, nullptr, nullptr);
        return core::Result<void>(core::Error{3, "Failed to commit transaction: " + errStr});
    }

    if (m_eventBus) {
        core::events::LibraryEvent event;
        event.type = core::events::LibraryEvent::Type::BatchRemoved;
        event.mediaIds = mediaIds;
        m_eventBus->publish(event);
    }

    return core::Result<void>();
}

} // namespace mnemis::database::repositories
