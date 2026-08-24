#include "SQLitePlaylistRepository.hpp"
#include <sqlite3.h>
#include <chrono>

namespace mnemis::database::repositories {

SQLitePlaylistRepository::SQLitePlaylistRepository(DatabaseConnection& conn, core::ILogger& logger)
    : m_conn(conn), m_logger(logger) {}

core::Result<void> SQLitePlaylistRepository::createPlaylist(const core::models::Playlist& playlist) {
    const char* sql = "INSERT INTO playlists (playlist_id, name, created_time, modified_time, is_smart, query_json) VALUES (?, ?, ?, ?, ?, ?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return core::Result<void>(core::Error{1, "Failed to prepare createPlaylist statement"});
    }

    sqlite3_bind_text(stmt, 1, playlist.playlistId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, playlist.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 3, playlist.createdTime);
    sqlite3_bind_int64(stmt, 4, playlist.modifiedTime);
    sqlite3_bind_int(stmt, 5, playlist.isSmart ? 1 : 0);
    sqlite3_bind_text(stmt, 6, playlist.queryJson.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        return core::Result<void>(core::Error{rc, "Failed to execute createPlaylist"});
    }
    return core::Result<void>();
}

core::Result<void> SQLitePlaylistRepository::updatePlaylist(const core::models::Playlist& playlist) {
    const char* sql = "UPDATE playlists SET name = ?, modified_time = ?, is_smart = ?, query_json = ? WHERE playlist_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return core::Result<void>(core::Error{1, "Failed to prepare updatePlaylist"});
    }
    sqlite3_bind_text(stmt, 1, playlist.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, playlist.modifiedTime);
    sqlite3_bind_int(stmt, 3, playlist.isSmart ? 1 : 0);
    sqlite3_bind_text(stmt, 4, playlist.queryJson.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, playlist.playlistId.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) return core::Result<void>(core::Error{rc, "Failed to execute updatePlaylist"});
    return core::Result<void>();
}

core::Result<void> SQLitePlaylistRepository::deletePlaylist(const std::string& playlistId) {
    const char* sql = "DELETE FROM playlists WHERE playlist_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return core::Result<void>(core::Error{1, "Prepare failed"});
    sqlite3_bind_text(stmt, 1, playlistId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return core::Result<void>();
}

core::Result<std::vector<core::models::Playlist>> SQLitePlaylistRepository::getAllPlaylists() {
    const char* sql = "SELECT playlist_id, name, created_time, modified_time, is_smart, query_json FROM playlists";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return core::Result<std::vector<core::models::Playlist>>(core::Error{1, "Failed to prepare getAllPlaylists"});
    }

    std::vector<core::models::Playlist> playlists;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        core::models::Playlist p;
        p.playlistId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.createdTime = sqlite3_column_int64(stmt, 2);
        p.modifiedTime = sqlite3_column_int64(stmt, 3);
        p.isSmart = sqlite3_column_int(stmt, 4) != 0;
        const char* q = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (q) p.queryJson = q;
        playlists.push_back(p);
    }
    sqlite3_finalize(stmt);
    return core::Result<std::vector<core::models::Playlist>>(playlists);
}

core::Result<std::optional<core::models::Playlist>> SQLitePlaylistRepository::getPlaylistById(const std::string& playlistId) {
    const char* sql = "SELECT playlist_id, name, created_time, modified_time, is_smart, query_json FROM playlists WHERE playlist_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return core::Result<std::optional<core::models::Playlist>>(core::Error{1, "Failed to prepare getPlaylistById"});
    }
    sqlite3_bind_text(stmt, 1, playlistId.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<core::models::Playlist> result = std::nullopt;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        core::models::Playlist p;
        p.playlistId = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.createdTime = sqlite3_column_int64(stmt, 2);
        p.modifiedTime = sqlite3_column_int64(stmt, 3);
        p.isSmart = sqlite3_column_int(stmt, 4) != 0;
        const char* q = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        if (q) p.queryJson = q;
        result = p;
    }
    sqlite3_finalize(stmt);
    return core::Result<std::optional<core::models::Playlist>>(result);
}

core::Result<void> SQLitePlaylistRepository::addMediaToPlaylist(const std::string& playlistId, const std::string& mediaId) {
    const char* sql = "INSERT INTO playlist_items (playlist_id, media_id, position, added_time) VALUES (?, ?, (SELECT COALESCE(MAX(position),0)+1 FROM playlist_items WHERE playlist_id=?), ?)";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return core::Result<void>(core::Error{1, "Prepare failed"});
    
    auto now = std::chrono::system_clock::now().time_since_epoch();
    int64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();

    sqlite3_bind_text(stmt, 1, playlistId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, mediaId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, playlistId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timestamp);
    
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return core::Result<void>();
}

core::Result<void> SQLitePlaylistRepository::removeMediaFromPlaylist(const std::string& playlistId, const std::string& mediaId) {
    const char* sql = "DELETE FROM playlist_items WHERE playlist_id = ? AND media_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return core::Result<void>(core::Error{1, "Prepare failed"});
    sqlite3_bind_text(stmt, 1, playlistId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, mediaId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return core::Result<void>();
}

core::Result<void> SQLitePlaylistRepository::reorderPlaylistItems(const std::string& playlistId, const std::vector<std::string>& newOrderMediaIds) {
    const char* sql = "UPDATE playlist_items SET position = ? WHERE playlist_id = ? AND media_id = ?";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_conn.getHandle(), sql, -1, &stmt, nullptr) != SQLITE_OK) return core::Result<void>(core::Error{1, "Prepare failed"});
    
    int position = 1;
    for (const auto& mediaId : newOrderMediaIds) {
        sqlite3_bind_int(stmt, 1, position++);
        sqlite3_bind_text(stmt, 2, playlistId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, mediaId.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    return core::Result<void>();
}

} // namespace mnemis::database::repositories
