#include "PlaylistController.hpp"
#include <QUuid>
#include <chrono>

namespace mnemis::ui::controllers {

PlaylistController::PlaylistController(core::repositories::IPlaylistRepository& repo, core::ILogger& logger, QObject* parent)
    : QObject(parent), m_repo(repo), m_logger(logger) {}

void PlaylistController::loadPlaylists() {
    auto result = m_repo.getAllPlaylists();
    if (!result.isSuccess()) {
        m_logger.log(core::LogLevel::Error, "Failed to load playlists: " + result.error().message);
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    QVariantList list;
    for (const auto& p : result.value()) {
        QVariantMap map;
        map["playlistId"] = QString::fromStdString(p.playlistId);
        map["name"] = QString::fromStdString(p.name);
        map["createdTime"] = QVariant::fromValue(p.createdTime);
        map["modifiedTime"] = QVariant::fromValue(p.modifiedTime);
        map["isSmart"] = p.isSmart;
        list.append(map);
    }
    emit playlistsLoaded(list);
}

void PlaylistController::createPlaylist(const QString& name) {
    core::models::Playlist p;
    p.playlistId = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    p.name = name.toStdString();
    auto now = std::chrono::system_clock::now().time_since_epoch();
    p.createdTime = std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    p.modifiedTime = p.createdTime;
    p.isSmart = false;

    auto result = m_repo.createPlaylist(p);
    if (!result.isSuccess()) {
        m_logger.log(core::LogLevel::Error, "Failed to create playlist: " + result.error().message);
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    loadPlaylists();
}

void PlaylistController::deletePlaylist(const QString& playlistId) {
    auto result = m_repo.deletePlaylist(playlistId.toStdString());
    if (!result.isSuccess()) {
        m_logger.log(core::LogLevel::Error, "Failed to delete playlist: " + result.error().message);
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    loadPlaylists();
}

void PlaylistController::addMediaToPlaylist(const QString& playlistId, const QString& mediaId) {
    auto result = m_repo.addMediaToPlaylist(playlistId.toStdString(), mediaId.toStdString());
    if (!result.isSuccess()) {
        m_logger.log(core::LogLevel::Error, "Failed to add media to playlist: " + result.error().message);
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

void PlaylistController::removeMediaFromPlaylist(const QString& playlistId, const QString& mediaId) {
    auto result = m_repo.removeMediaFromPlaylist(playlistId.toStdString(), mediaId.toStdString());
    if (!result.isSuccess()) {
        m_logger.log(core::LogLevel::Error, "Failed to remove media from playlist: " + result.error().message);
        emit errorOccurred(QString::fromStdString(result.error().message));
    }
}

} // namespace mnemis::ui::controllers
