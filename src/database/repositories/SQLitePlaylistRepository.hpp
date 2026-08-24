#pragma once

#include "core/repositories/IPlaylistRepository.hpp"
#include "database/DatabaseConnection.hpp"
#include "core/ILogger.hpp"

namespace mnemis::database::repositories {

class SQLitePlaylistRepository : public core::repositories::IPlaylistRepository {
public:
    SQLitePlaylistRepository(DatabaseConnection& conn, core::ILogger& logger);
    ~SQLitePlaylistRepository() override = default;

    core::Result<void> createPlaylist(const core::models::Playlist& playlist) override;
    core::Result<void> updatePlaylist(const core::models::Playlist& playlist) override;
    core::Result<void> deletePlaylist(const std::string& playlistId) override;
    
    core::Result<std::vector<core::models::Playlist>> getAllPlaylists() override;
    core::Result<std::optional<core::models::Playlist>> getPlaylistById(const std::string& playlistId) override;
    
    core::Result<void> addMediaToPlaylist(const std::string& playlistId, const std::string& mediaId) override;
    core::Result<void> removeMediaFromPlaylist(const std::string& playlistId, const std::string& mediaId) override;
    core::Result<void> reorderPlaylistItems(const std::string& playlistId, const std::vector<std::string>& newOrderMediaIds) override;

private:
    DatabaseConnection& m_conn;
    core::ILogger& m_logger;
};

} // namespace mnemis::database::repositories
